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
#include <cstdio>

#include <cstring>
#include <libac97/ac97.hpp>
#include <libdevice/manager.hpp>
#include <libpci/driver.hpp>

#define AC97_LOG(fmt, ...) klog("ac97driver: " fmt, ##__VA_ARGS__)

namespace
{
struct dma_buffer
{
	void* virt = nullptr;
	uint64_t phys = 0;
};

struct ac97_context
{
	g_pci_device_address device = 0;
	uint16_t mixerBase = 0;
	uint16_t busMasterBase = 0;
	volatile ac97_buffer_descriptor* bdl = nullptr;
	uint64_t bdlPhys = 0;
	dma_buffer buffers[AC97_BDL_ENTRY_COUNT]{};
	uint8_t lvi = 0;
	g_fd driverPipe = G_FD_NONE;
	g_device_id deviceId = 0;
	size_t streamBytes = 0;
	size_t zeroDescriptors = 0;
} g_ctx;

size_t fillDescriptor(uint8_t index);

bool findController()
{
	int count = 0;
	g_pci_device_data* devices = nullptr;
	if(!pciDriverListDevices(&count, &devices))
	{
		AC97_LOG("failed to list PCI devices");
		return false;
	}

	for(int i = 0; i < count; ++i)
	{
		if(devices[i].classCode != PCI_BASE_CLASS_MULTIMEDIA ||
		   devices[i].subclassCode != PCI_04_SUBCLASS_MULTIMEDIA_AUDIO)
		{
			continue;
		}

		uint32_t vendorId = 0;
		uint32_t deviceId = 0;
		if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_VENDOR_ID, 2, &vendorId) ||
		   !pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_DEVICE_ID, 2, &deviceId))
		{
			continue;
		}

		if(vendorId == 0x8086 && deviceId == 0x2415)
		{
			g_ctx.device = devices[i].deviceAddress;
			pciDriverFreeDeviceList(devices);
			AC97_LOG("found AC97 controller at %x", g_ctx.device);
			return true;
		}
	}

	pciDriverFreeDeviceList(devices);
	return false;
}

bool mapResources()
{
	g_address bar0 = 0;
	g_address bar1 = 0;
	if(!pciDriverReadBAR(g_ctx.device, 0, &bar0) || !pciDriverReadBAR(g_ctx.device, 1, &bar1))
	{
		AC97_LOG("failed to read BARs");
		return false;
	}

	pciDriverEnableResourceAccess(g_ctx.device, true);
	g_ctx.mixerBase = static_cast<uint16_t>(bar0 & ~0x1);
	g_ctx.busMasterBase = static_cast<uint16_t>(bar1 & ~0x1);
	AC97_LOG("mixer IO base=0x%x, bus master base=0x%x", g_ctx.mixerBase, g_ctx.busMasterBase);
	return true;
}

inline void writeMixer(uint16_t reg, uint16_t value)
{
	g_io_port_write_word(g_ctx.mixerBase + reg, value);
}

inline uint16_t readMixer(uint16_t reg)
{
	return g_io_port_read_word(g_ctx.mixerBase + reg);
}

void resetCodec()
{
	g_io_port_write_dword(g_ctx.busMasterBase + AC97_BM_REG_GLOBAL_CONTROL, AC97_GLOB_CNT_COLD);
	g_sleep(10);
	g_io_port_write_dword(g_ctx.busMasterBase + AC97_BM_REG_GLOBAL_CONTROL, 0);
	g_sleep(10);

	writeMixer(AC97_REG_POWER_CONTROL, AC97_POWER_EAPD);
	g_sleep(5);
}

void configureMixer()
{
	writeMixer(AC97_REG_MASTER_VOLUME, 0x0808);
	writeMixer(AC97_REG_PCM_OUT_VOLUME, 0x0808);
	writeMixer(AC97_REG_FRONT_DAC_RATE, AC97_DEFAULT_SAMPLE_RATE);
}

bool initializeDma()
{
	void* bdlPhys = nullptr;
	auto* bdlVirt =
	    reinterpret_cast<ac97_buffer_descriptor*>(g_alloc_mem_p(sizeof(ac97_buffer_descriptor) * AC97_BDL_ENTRY_COUNT,
	                                                            &bdlPhys));
	if(!bdlVirt)
	{
		AC97_LOG("failed to allocate descriptor list");
		return false;
	}

	std::memset(bdlVirt, 0, sizeof(ac97_buffer_descriptor) * AC97_BDL_ENTRY_COUNT);
	g_ctx.bdl = bdlVirt;
	g_ctx.bdlPhys = reinterpret_cast<uint64_t>(bdlPhys);

	for(size_t i = 0; i < AC97_BDL_ENTRY_COUNT; ++i)
	{
		void* bufPhys = nullptr;
		void* bufVirt = g_alloc_mem_p(AC97_DMA_BUFFER_SIZE, &bufPhys);
		if(!bufVirt)
		{
			AC97_LOG("failed to allocate DMA buffer %zu", i);
			return false;
		}

		std::memset(bufVirt, 0, AC97_DMA_BUFFER_SIZE);
		g_ctx.buffers[i].virt = bufVirt;
		g_ctx.buffers[i].phys = reinterpret_cast<uint64_t>(bufPhys);
		g_ctx.bdl[i].buffer = static_cast<uint32_t>(g_ctx.buffers[i].phys);
		g_ctx.bdl[i].length = AC97_DMA_BUFFER_SIZE & 0xFFFE;
		g_ctx.bdl[i].control = AC97_BDL_IOC;
	}

	g_io_port_write_dword(g_ctx.busMasterBase + AC97_BM_REG_PO_BDBAR, static_cast<uint32_t>(g_ctx.bdlPhys));

	for(uint32_t i = 0; i < AC97_BDL_ENTRY_COUNT; ++i)
	{
		fillDescriptor(i);
	}
	g_ctx.lvi = AC97_BDL_ENTRY_COUNT - 1;
	g_io_port_write_byte(g_ctx.busMasterBase + AC97_BM_REG_PO_LVI, g_ctx.lvi);

	g_io_port_write_word(g_ctx.busMasterBase + AC97_BM_REG_PO_SR,
	                     AC97_PO_SR_DCH | AC97_PO_SR_CELV | AC97_PO_SR_LVBCI | AC97_PO_SR_BCIS | AC97_PO_SR_FIFOE);

	g_io_port_write_word(g_ctx.busMasterBase + AC97_BM_REG_PO_PICB, AC97_DMA_BUFFER_SIZE / 2);

	uint8_t control = g_io_port_read_byte(g_ctx.busMasterBase + AC97_BM_REG_PO_CR);
	control |= AC97_PO_CR_RUN;
	g_io_port_write_byte(g_ctx.busMasterBase + AC97_BM_REG_PO_CR, control);
	AC97_LOG("DMA initialized");
	return true;
}

size_t fillDescriptor(uint8_t index)
{
	auto* dst = reinterpret_cast<uint8_t*>(g_ctx.buffers[index].virt);
	size_t remaining = AC97_DMA_BUFFER_SIZE;
	size_t written = 0;

	while(remaining > 0)
	{
		g_fs_read_status status = G_FS_READ_SUCCESSFUL;
		int32_t read = g_read_s(g_ctx.driverPipe, dst + written, remaining, &status);
		if(read <= 0)
		{
			if(status == G_FS_READ_BUSY)
			{
				g_sleep(1);
				continue;
			}

			++g_ctx.zeroDescriptors;
			AC97_LOG("descriptor %u: pipe empty/EOF, padding remaining %zu bytes (zero desc=%zu)",
			         index, remaining, g_ctx.zeroDescriptors);
			std::memset(dst + written, 0, remaining);
			written += remaining;
			break;
		}

		written += read;
		remaining -= read;
	}

	g_ctx.streamBytes += written;

	if(written % 4)
	{
		size_t pad = 4 - (written % 4);
		std::memset(dst + written, 0, pad);
		written += pad;
	}

	g_ctx.bdl[index].buffer = static_cast<uint32_t>(g_ctx.buffers[index].phys);
	uint16_t byteLength = static_cast<uint16_t>(written & 0xFFFE);
	if(byteLength == 0)
		byteLength = 2;
	g_ctx.bdl[index].length = byteLength;
	g_ctx.bdl[index].control = AC97_BDL_IOC;
	AC97_LOG("filled descriptor %u with %zu bytes (total streamed=%zu)", index, written, g_ctx.streamBytes);
	return written;
}

void feederLoop()
{
	while(true)
	{
		uint8_t civ = g_io_port_read_byte(g_ctx.busMasterBase + AC97_BM_REG_PO_CIV);
		uint8_t next = (g_ctx.lvi + 1) % AC97_BDL_ENTRY_COUNT;

		if(next == civ)
		{
			g_sleep(2);
			continue;
		}

		auto written = fillDescriptor(next);
		g_ctx.lvi = next;
		g_io_port_write_byte(g_ctx.busMasterBase + AC97_BM_REG_PO_LVI, g_ctx.lvi);
		uint16_t status = g_io_port_read_word(g_ctx.busMasterBase + AC97_BM_REG_PO_SR);
		g_io_port_write_word(g_ctx.busMasterBase + AC97_BM_REG_PO_SR,
		                     AC97_PO_SR_BCIS | AC97_PO_SR_LVBCI | AC97_PO_SR_FIFOE);
		if(status & AC97_PO_SR_FIFOE)
		{
			AC97_LOG("FIFO underrun detected (status=0x%x)", status);
		}
		AC97_LOG("advanced LVI=%u CIV=%u status=0x%x lastFill=%zu", g_ctx.lvi, civ, status, written);
	}
}

bool preparePcmPipe()
{
	g_fd publishFd = G_FD_NONE;
	if(g_pipe_b(&publishFd, &g_ctx.driverPipe, false) != G_FS_PIPE_SUCCESSFUL)
	{
		AC97_LOG("failed to create PCM pipe");
		return false;
	}

	auto status = g_fs_publish_pipe("ac97", publishFd, false);
	if(status != G_FS_PUBLISH_PIPE_SUCCESS)
	{
		AC97_LOG("failed to publish PCM pipe (status=%d)", status);
		return false;
	}

	g_close(publishFd);
	AC97_LOG("pcm pipe ready at /dev/ac97 driver=%d (non-blocking)", g_ctx.driverPipe);
	return true;
}

bool initializeDriver()
{
	if(!findController())
		return false;
	if(!mapResources())
		return false;
	if(!preparePcmPipe())
		return false;

	resetCodec();
	configureMixer();
	if(!initializeDma())
		return false;

	return true;
}

} // namespace

int main()
{
	AC97_LOG("starting");
	if(!g_task_register_name(G_AC97_DRIVER_NAME))
	{
		AC97_LOG("failed to register task name");
		return -1;
	}

	if(!initializeDriver())
	{
		AC97_LOG("initialization failed");
		return -1;
	}

	if(!deviceManagerRegisterDevice(G_DEVICE_TYPE_AUDIO, g_get_tid(), &g_ctx.deviceId))
	{
		AC97_LOG("failed to register device");
	}
	else
	{
		AC97_LOG("registered audio device id %u", g_ctx.deviceId);
	}

	AC97_LOG("ready: waiting for PCM data via /dev/ac97");
	feederLoop();
	return 0;
}
