/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <ghost.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <libdevice/manager.hpp>
#include <libeth/ethdriver.hpp>
#include <libpci/driver.hpp>

#define ETH_LOG(fmt, ...) klog("ethdriver: " fmt, ##__VA_ARGS__)

namespace
{
constexpr uint16_t INTEL_VENDOR_ID = 0x8086;
constexpr uint16_t INTEL_E1000_DEVICE_ID = 0x100E;
constexpr uint32_t E1000_MMIO_SIZE = 0x20000;

constexpr uint32_t E1000_RX_DESCRIPTOR_COUNT = 32;
constexpr uint32_t E1000_TX_DESCRIPTOR_COUNT = 16;
constexpr uint32_t E1000_RX_BUFFER_SIZE = 2048;

// Register offsets
constexpr uint32_t E1000_REG_CTRL = 0x0000;
constexpr uint32_t E1000_REG_STATUS = 0x0008;
constexpr uint32_t E1000_REG_EERD = 0x0014;
constexpr uint32_t E1000_REG_IMS = 0x00D0;
constexpr uint32_t E1000_REG_IMC = 0x00D8;
constexpr uint32_t E1000_REG_RCTL = 0x0100;
constexpr uint32_t E1000_REG_TCTL = 0x0400;
constexpr uint32_t E1000_REG_TIPG = 0x0410;
constexpr uint32_t E1000_REG_RDBAL = 0x2800;
constexpr uint32_t E1000_REG_RDBAH = 0x2804;
constexpr uint32_t E1000_REG_RDLEN = 0x2808;
constexpr uint32_t E1000_REG_RDH = 0x2810;
constexpr uint32_t E1000_REG_RDT = 0x2818;
constexpr uint32_t E1000_REG_TDBAL = 0x3800;
constexpr uint32_t E1000_REG_TDBAH = 0x3804;
constexpr uint32_t E1000_REG_TDLEN = 0x3808;
constexpr uint32_t E1000_REG_TDH = 0x3810;
constexpr uint32_t E1000_REG_TDT = 0x3818;
constexpr uint32_t E1000_REG_RAL0 = 0x5400;
constexpr uint32_t E1000_REG_RAH0 = 0x5404;

// STATUS bits
constexpr uint32_t E1000_STATUS_LU = (1u << 1);

// CTRL bits
constexpr uint32_t E1000_CTRL_RST = (1u << 26);
constexpr uint32_t E1000_CTRL_ASDE = (1u << 5);
constexpr uint32_t E1000_CTRL_SLU = (1u << 6);

// RCTL bits
constexpr uint32_t E1000_RCTL_EN = (1u << 1);
constexpr uint32_t E1000_RCTL_SBP = (1u << 2);
constexpr uint32_t E1000_RCTL_UPE = (1u << 3);
constexpr uint32_t E1000_RCTL_MPE = (1u << 4);
constexpr uint32_t E1000_RCTL_BAM = (1u << 15);
constexpr uint32_t E1000_RCTL_SECRC = (1u << 26);
constexpr uint32_t E1000_RCTL_BSIZE_2048 = 0;

// TCTL bits
constexpr uint32_t E1000_TCTL_EN = (1u << 1);
constexpr uint32_t E1000_TCTL_PSP = (1u << 3);
constexpr uint32_t E1000_TCTL_CT_SHIFT = 4;
constexpr uint32_t E1000_TCTL_COLD_SHIFT = 12;
constexpr uint32_t E1000_TCTL_RTLC = (1u << 24);

// EEPROM bits
constexpr uint32_t E1000_EERD_START = (1u << 0);
constexpr uint32_t E1000_EERD_DONE = (1u << 4);
constexpr uint32_t E1000_EERD_ADDR_SHIFT = 8;
constexpr uint32_t E1000_EERD_DATA_SHIFT = 16;

// Descriptor flags
constexpr uint8_t E1000_RX_STATUS_DD = (1u << 0);
constexpr uint8_t E1000_TX_STATUS_DD = (1u << 0);
constexpr uint8_t E1000_TX_CMD_EOP = (1u << 0);
constexpr uint8_t E1000_TX_CMD_IFCS = (1u << 1);
constexpr uint8_t E1000_TX_CMD_RS = (1u << 3);

struct e1000_rx_desc
{
	uint64_t address;
	uint16_t length;
	uint16_t checksum;
	uint8_t status;
	uint8_t errors;
	uint16_t special;
} __attribute__((packed));

struct e1000_tx_desc
{
	uint64_t address;
	uint16_t length;
	uint8_t cso;
	uint8_t command;
	uint8_t status;
	uint8_t css;
	uint16_t special;
} __attribute__((packed));

struct e1000_buffer
{
	void* virt;
	uint64_t phys;
};

struct ethdriver_context
{
	g_pci_device_address deviceAddress = 0;
	volatile uint8_t* mmio = nullptr;
	volatile e1000_rx_desc* rxDescriptors = nullptr;
	volatile e1000_tx_desc* txDescriptors = nullptr;
	e1000_buffer rxBuffers[E1000_RX_DESCRIPTOR_COUNT]{};
	e1000_buffer txBuffers[E1000_TX_DESCRIPTOR_COUNT]{};
	uint64_t rxDescriptorPhys = 0;
	uint64_t txDescriptorPhys = 0;
	uint32_t rxIndex = 0;
	uint32_t txTail = 0;
	g_fd rxPipeWrite = -1;
	g_fd rxPipeRead = -1;
	g_fd txPipeWrite = -1;
	g_fd txPipeRead = -1;
	g_tid rxPartner = G_TID_NONE;
	uint8_t mac[6]{};
	g_device_id deviceId = 0;
	bool linkReady = false;
} g_ctx;

inline volatile uint32_t* e1000Reg(uint32_t reg)
{
	return reinterpret_cast<volatile uint32_t*>(g_ctx.mmio + reg);
}

uint32_t e1000ReadReg(uint32_t reg)
{
	return *e1000Reg(reg);
}

void e1000WriteReg(uint32_t reg, uint32_t value)
{
	*e1000Reg(reg) = value;
}

bool identifyDevice()
{
	ETH_LOG("probing PCI bus for E1000 controller");
	int count = 0;
	g_pci_device_data* devices = nullptr;
	if(!pciDriverListDevices(&count, &devices))
	{
		ETH_LOG("failed to list PCI devices");
		return false;
	}

	bool found = false;
	for(int i = 0; i < count; i++)
	{
		if(devices[i].classCode != PCI_BASE_CLASS_NETWORK ||
		   devices[i].subclassCode != PCI_02_SUBCLASS_ETHERNET)
			continue;

		uint32_t vendorId = 0;
		if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_VENDOR_ID, 2, &vendorId))
			continue;

		uint32_t deviceId = 0;
		if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_DEVICE_ID, 2, &deviceId))
			continue;

		if(vendorId == INTEL_VENDOR_ID && deviceId == INTEL_E1000_DEVICE_ID)
		{
			g_ctx.deviceAddress = devices[i].deviceAddress;
			ETH_LOG("found controller at PCI address 0x%x", g_ctx.deviceAddress);
			found = true;
			break;
		}
	}

	pciDriverFreeDeviceList(devices);
	return found;
}

bool mapMmio()
{
	g_address bar = 0;
	if(!pciDriverReadBAR(g_ctx.deviceAddress, 0, &bar))
	{
		ETH_LOG("failed to read BAR0");
		return false;
	}

	pciDriverEnableResourceAccess(g_ctx.deviceAddress, true);
	g_ctx.mmio = reinterpret_cast<volatile uint8_t*>(g_map_mmio((void*) bar, E1000_MMIO_SIZE));
	if(!g_ctx.mmio)
	{
		ETH_LOG("failed to map MMIO region");
		return false;
	}
	ETH_LOG("MMIO mapped at physical 0x%x", (uint32_t) bar);
	return true;
}

void reset()
{
	e1000WriteReg(E1000_REG_IMC, 0xFFFFFFFF);
	e1000WriteReg(E1000_REG_CTRL, e1000ReadReg(E1000_REG_CTRL) | E1000_CTRL_RST);
	g_sleep(10);
}

bool configureLink()
{
	uint32_t ctrl = e1000ReadReg(E1000_REG_CTRL);
	ctrl |= E1000_CTRL_ASDE | E1000_CTRL_SLU;
	e1000WriteReg(E1000_REG_CTRL, ctrl);

	for(int attempt = 0; attempt < 200; ++attempt)
	{
		uint32_t status = e1000ReadReg(E1000_REG_STATUS);
		if(status & E1000_STATUS_LU)
		{
			g_ctx.linkReady = true;
			return true;
		}
		g_sleep(5);
	}

	ETH_LOG("link did not come up");
	g_ctx.linkReady = false;
	return false;
}

void monitorLink()
{
	bool last = g_ctx.linkReady;
	for(;;)
	{
		bool up = (e1000ReadReg(E1000_REG_STATUS) & E1000_STATUS_LU) != 0;
		if(up != last)
		{
			g_ctx.linkReady = up;
			ETH_LOG("link state changed: %s", up ? "up" : "down");
			last = up;
		}
		g_sleep(500);
	}
}

bool initRx()
{
	void* descPhys = nullptr;
	void* descVirt = g_alloc_mem_p(sizeof(e1000_rx_desc) * E1000_RX_DESCRIPTOR_COUNT, &descPhys);
	if(!descVirt)
		return false;

	std::memset(descVirt, 0, sizeof(e1000_rx_desc) * E1000_RX_DESCRIPTOR_COUNT);
	g_ctx.rxDescriptors = reinterpret_cast<volatile e1000_rx_desc*>(descVirt);
	g_ctx.rxDescriptorPhys = reinterpret_cast<uint64_t>(descPhys);

	for(uint32_t i = 0; i < E1000_RX_DESCRIPTOR_COUNT; i++)
	{
		void* bufPhys = nullptr;
		void* bufVirt = g_alloc_mem_p(E1000_RX_BUFFER_SIZE, &bufPhys);
		if(!bufVirt)
			return false;

		g_ctx.rxBuffers[i].virt = bufVirt;
		g_ctx.rxBuffers[i].phys = reinterpret_cast<uint64_t>(bufPhys);
		g_ctx.rxDescriptors[i].address = g_ctx.rxBuffers[i].phys;
		g_ctx.rxDescriptors[i].status = 0;
	}

	e1000WriteReg(E1000_REG_RDBAL, (uint32_t) g_ctx.rxDescriptorPhys);
	e1000WriteReg(E1000_REG_RDBAH, (uint32_t) (g_ctx.rxDescriptorPhys >> 32));
	e1000WriteReg(E1000_REG_RDLEN, E1000_RX_DESCRIPTOR_COUNT * sizeof(e1000_rx_desc));
	e1000WriteReg(E1000_REG_RDH, 0);
	e1000WriteReg(E1000_REG_RDT, E1000_RX_DESCRIPTOR_COUNT - 1);

	uint32_t rctl = E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_UPE | E1000_RCTL_MPE |
	                E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_BSIZE_2048;
	e1000WriteReg(E1000_REG_RCTL, rctl);
	return true;
}

bool initTx()
{
	void* descPhys = nullptr;
	void* descVirt = g_alloc_mem_p(sizeof(e1000_tx_desc) * E1000_TX_DESCRIPTOR_COUNT, &descPhys);
	if(!descVirt)
		return false;

	std::memset(descVirt, 0, sizeof(e1000_tx_desc) * E1000_TX_DESCRIPTOR_COUNT);
	g_ctx.txDescriptors = reinterpret_cast<volatile e1000_tx_desc*>(descVirt);
	g_ctx.txDescriptorPhys = reinterpret_cast<uint64_t>(descPhys);

	for(uint32_t i = 0; i < E1000_TX_DESCRIPTOR_COUNT; i++)
	{
		void* bufPhys = nullptr;
		void* bufVirt = g_alloc_mem_p(G_ETH_FRAME_DATA_SIZE, &bufPhys);
		if(!bufVirt)
			return false;

		g_ctx.txBuffers[i].virt = bufVirt;
		g_ctx.txBuffers[i].phys = reinterpret_cast<uint64_t>(bufPhys);
		g_ctx.txDescriptors[i].address = g_ctx.txBuffers[i].phys;
		g_ctx.txDescriptors[i].status = E1000_TX_STATUS_DD;
	}

	e1000WriteReg(E1000_REG_TDBAL, (uint32_t) g_ctx.txDescriptorPhys);
	e1000WriteReg(E1000_REG_TDBAH, (uint32_t) (g_ctx.txDescriptorPhys >> 32));
	e1000WriteReg(E1000_REG_TDLEN, E1000_TX_DESCRIPTOR_COUNT * sizeof(e1000_tx_desc));
	e1000WriteReg(E1000_REG_TDH, 0);
	e1000WriteReg(E1000_REG_TDT, 0);

	uint32_t tctl = E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_RTLC;
	tctl |= (0x10 << E1000_TCTL_CT_SHIFT);
	tctl |= (0x40 << E1000_TCTL_COLD_SHIFT);
	e1000WriteReg(E1000_REG_TCTL, tctl);
	e1000WriteReg(E1000_REG_TIPG, 0x0060200A);
	return true;
}

bool readMac()
{
	ETH_LOG("reading MAC address");
	uint32_t ral = e1000ReadReg(E1000_REG_RAL0);
	uint32_t rah = e1000ReadReg(E1000_REG_RAH0);
	if(rah & (1u << 31))
	{
		g_ctx.mac[0] = ral & 0xFF;
		g_ctx.mac[1] = (ral >> 8) & 0xFF;
		g_ctx.mac[2] = (ral >> 16) & 0xFF;
		g_ctx.mac[3] = (ral >> 24) & 0xFF;
		g_ctx.mac[4] = rah & 0xFF;
		g_ctx.mac[5] = (rah >> 8) & 0xFF;
		return true;
	}

	for(uint32_t i = 0; i < 3; i++)
	{
		e1000WriteReg(E1000_REG_EERD, E1000_EERD_START | (i << E1000_EERD_ADDR_SHIFT));
		while(!(e1000ReadReg(E1000_REG_EERD) & E1000_EERD_DONE))
		{
			g_yield();
		}

		uint32_t value = e1000ReadReg(E1000_REG_EERD);
		uint16_t data = (uint16_t) (value >> E1000_EERD_DATA_SHIFT);
		g_ctx.mac[i * 2] = data & 0xFF;
		g_ctx.mac[i * 2 + 1] = (data >> 8) & 0xFF;
	}
	return true;
}

bool transmit(const uint8_t* data, uint16_t length)
{
	if(length == 0)
		return false;

	if(length > G_ETH_FRAME_DATA_SIZE)
		length = G_ETH_FRAME_DATA_SIZE;

	while(true)
	{
		uint32_t index = g_ctx.txTail;
		volatile e1000_tx_desc& desc = g_ctx.txDescriptors[index];
		if(desc.status & E1000_TX_STATUS_DD)
		{
			std::memcpy(g_ctx.txBuffers[index].virt, data, length);
			desc.length = length;
			desc.command = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
			desc.status = 0;

			g_ctx.txTail = (index + 1) % E1000_TX_DESCRIPTOR_COUNT;
			e1000WriteReg(E1000_REG_TDT, g_ctx.txTail);
			return true;
		}
		g_sleep(1);
	}
}

void rxLoop()
{
	ETH_LOG("RX loop started");
	while(true)
	{
		volatile e1000_rx_desc& desc = g_ctx.rxDescriptors[g_ctx.rxIndex];
		if(desc.status & E1000_RX_STATUS_DD)
		{
			uint16_t length = desc.length;
			if(length > G_ETH_FRAME_DATA_SIZE)
				length = G_ETH_FRAME_DATA_SIZE;

			g_eth_frame frame{};
			frame.length = length;
			std::memcpy(frame.data, g_ctx.rxBuffers[g_ctx.rxIndex].virt, length);

			if(g_write(g_ctx.rxPipeWrite, &frame, sizeof(frame)) == sizeof(frame))
			{
				if(g_ctx.rxPartner != G_TID_NONE)
				{
					g_yield_t(g_ctx.rxPartner);
				}
			}

			desc.status = 0;
			e1000WriteReg(E1000_REG_RDT, g_ctx.rxIndex);
			g_ctx.rxIndex = (g_ctx.rxIndex + 1) % E1000_RX_DESCRIPTOR_COUNT;
		}
		else
		{
			g_sleep(2);
		}
	}
}

void txLoop()
{
	ETH_LOG("TX loop started");
	g_eth_frame frame;
	while(true)
	{
		int32_t read = g_read(g_ctx.txPipeRead, &frame, sizeof(frame));
		if(read != sizeof(frame))
		{
			g_sleep(2);
			continue;
		}
		transmit(frame.data, frame.length);
	}
}

void handleInitialize(g_tid sender, g_message_transaction transaction, g_eth_initialize_request* request)
{
	g_ctx.rxPartner = request->rxPartnerTask;

	g_pid targetPid = g_get_pid_for_tid(sender);
	g_pid sourcePid = g_get_pid();

	g_eth_initialize_response response{};
	response.linkUp = g_ctx.linkReady ? 1 : 0;
	response.status = G_ETH_STATUS_SUCCESS;
	response.rxPipe = g_clone_fd(g_ctx.rxPipeRead, sourcePid, targetPid);
	response.txPipe = g_clone_fd(g_ctx.txPipeWrite, sourcePid, targetPid);
	if(response.rxPipe < 0 || response.txPipe < 0)
	{
		response.status = G_ETH_STATUS_FAILURE;
	}
	std::memcpy(response.mac, g_ctx.mac, sizeof(g_ctx.mac));

	g_send_message_t(sender, &response, sizeof(response), transaction);
}

void messageLoop()
{
	size_t bufLen = sizeof(g_message_header) + sizeof(g_eth_initialize_request);
	uint8_t buf[bufLen];

	while(true)
	{
		auto status = g_receive_message(buf, bufLen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;

		auto header = (g_message_header*) buf;
		auto request = (g_eth_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_ETH_COMMAND_INITIALIZE)
		{
			handleInitialize(header->sender, header->transaction,
			                (g_eth_initialize_request*) request);
		}
	}
}

bool initializeDriver()
{
	ETH_LOG("initializing driver context");
	if(!identifyDevice())
	{
		ETH_LOG("e1000 controller not found");
		return false;
	}

	if(g_pipe_b(&g_ctx.rxPipeWrite, &g_ctx.rxPipeRead, false) != G_FS_PIPE_SUCCESSFUL)
	{
		ETH_LOG("failed to create RX pipe");
		return false;
	}

	if(g_pipe(&g_ctx.txPipeWrite, &g_ctx.txPipeRead) != G_FS_PIPE_SUCCESSFUL)
	{
		ETH_LOG("failed to create TX pipe");
		return false;
	}

	if(!mapMmio())
		return false;

	reset();
	if(!configureLink())
	{
		ETH_LOG("continuing initialization without active link");
	}

	if(!initRx() || !initTx())
	{
		ETH_LOG("failed to initialize descriptors");
		return false;
	}

	if(!readMac())
	{
		ETH_LOG("failed to obtain MAC, continuing with zeros");
	}
	else
	{
		ETH_LOG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		        g_ctx.mac[0], g_ctx.mac[1], g_ctx.mac[2],
		        g_ctx.mac[3], g_ctx.mac[4], g_ctx.mac[5]);
	}
	e1000WriteReg(E1000_REG_IMC, 0xFFFFFFFF);
	return true;
}

} // namespace

int main()
{
	ETH_LOG("main start");
	if(!g_task_register_name(G_ETH_DRIVER_NAME))
	{
		ETH_LOG("failed to register task name");
		return -1;
	}

	if(!initializeDriver())
	{
		ETH_LOG("initialization failed");
		return -1;
	}

	if(!deviceManagerRegisterDevice(G_DEVICE_TYPE_NETWORK, g_get_tid(), &g_ctx.deviceId))
	{
		ETH_LOG("failed to register device with manager");
	}
	else
	{
		ETH_LOG("registered device id %u", g_ctx.deviceId);
	}

	g_tid linkTask = g_create_task((void*) monitorLink);
	g_tid rxTask = g_create_task((void*) rxLoop);
	g_tid txTask = g_create_task((void*) txLoop);
	(void) rxTask;
	(void) txTask;
	(void) linkTask;

	messageLoop();
	return 0;
}
