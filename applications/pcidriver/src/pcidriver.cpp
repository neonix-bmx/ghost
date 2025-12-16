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

#include "pcidriver.hpp"

#include <cstdio>
#include <cstring>
#include <libpci/pci.hpp>
#include <libpci/driver.hpp>
#include <ghost.h>
#include <ghost/messages.h>
#include <malloc.h>

g_user_mutex configSpaceLock = g_mutex_initialize();
g_pci_device* deviceList = nullptr;
int deviceCount = 0;
g_user_mutex deviceListLock = g_mutex_initialize();

int main()
{
	klog("pcidriver: starting up");
	if(!g_task_register_name(G_PCI_DRIVER_NAME))
	{
		klog("pcidriver: failed to register task name '%s'", G_PCI_DRIVER_NAME);
		return -1;
	}
	klog("pcidriver: registered as %s", G_PCI_DRIVER_NAME);

	pciDriverScanBus();

	klog("pcidriver: ready to serve requests");
	pciDriverServeRequests();
}

void pciDriverServeRequests()
{
	uint8_t message[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	while(true)
	{
		g_message_receive_status status = g_receive_message(message, sizeof(message));
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;

		auto msgHeader = reinterpret_cast<g_message_header*>(message);
		auto header = reinterpret_cast<g_pci_request_header*>(G_MESSAGE_CONTENT(message));

		switch(header->command)
		{
		case G_PCI_LIST_DEVICES:
			pciDriverHandleListDevices(msgHeader->sender, msgHeader->transaction);
			break;
		case G_PCI_READ_CONFIG:
			pciDriverHandleReadConfig(msgHeader->sender, msgHeader->transaction,
			                         reinterpret_cast<g_pci_read_config_request*>(header));
			break;
		case G_PCI_WRITE_CONFIG:
			pciDriverHandleWriteConfig(msgHeader->sender, msgHeader->transaction,
			                          reinterpret_cast<g_pci_write_config_request*>(header));
			break;
		case G_PCI_ENABLE_RESOURCE_ACCESS:
			pciDriverHandleEnableResourceAccess(msgHeader->sender, msgHeader->transaction,
			                                   reinterpret_cast<g_pci_enable_resource_access_request*>(header));
			break;
		case G_PCI_READ_BAR:
			pciDriverHandleReadBar(msgHeader->sender, msgHeader->transaction,
			                      reinterpret_cast<g_pci_read_bar_request*>(header));
			break;
		case G_PCI_READ_BAR_SIZE:
			pciDriverHandleReadBarSize(msgHeader->sender, msgHeader->transaction,
			                        reinterpret_cast<g_pci_read_bar_size_request*>(header));
			break;
		default:
			klog("pcidriver: received unknown command %i", header->command);
			break;
		}
	}
}

void pciDriverHandleListDevices(g_tid sender, g_message_transaction tx)
{
	g_mutex_acquire(deviceListLock);
	klog("pcidriver: handling list-devices request");

	g_pci_list_devices_count_response response{};
	response.numDevices = deviceCount;
	klog("pcidriver: announced %d devices", deviceCount);

	size_t dataSize = deviceCount * sizeof(g_pci_device_data);
	auto data = (g_pci_device_data*) malloc(dataSize ? dataSize : 1);
	int pos = 0;
	g_pci_device* current = deviceList;
	while(current)
	{
		auto& entry = data[pos++];
		entry.deviceAddress = G_PCI_DEVICE_ADDRESS_BUILD(current->bus, current->device, current->function);
		entry.vendorId = current->vendorId;
		entry.deviceId = current->deviceId;
		entry.classCode = current->classCode;
		entry.subclassCode = current->subclassCode;
		entry.progIf = current->progIf;

		current = current->next;
	}

	g_mutex_release(deviceListLock);
	if(dataSize)
		klog("pcidriver: sending %zu bytes of device data", dataSize);
	else
		klog("pcidriver: device list empty");

	size_t payloadSize = sizeof(response) + dataSize;
	uint8_t* payload = (uint8_t*) malloc(payloadSize);
	std::memcpy(payload, &response, sizeof(response));
	if(dataSize)
		std::memcpy(payload + sizeof(response), data, dataSize);
	free(data);
	g_send_message_t(sender, payload, payloadSize, tx);
	free(payload);
}

void pciDriverHandleReadConfig(g_tid sender, g_message_transaction tx, g_pci_read_config_request* request)
{
	g_pci_read_config_response response{};
	uint8_t bus = G_PCI_DEVICE_ADDRESS_BUS(request->deviceAddress);
	uint8_t device = G_PCI_DEVICE_ADDRESS_DEVICE(request->deviceAddress);
	uint8_t function = G_PCI_DEVICE_ADDRESS_FUNCTION(request->deviceAddress);

	if(request->bytes == 1)
	{
		response.value = pciConfigReadByteAt(bus, device, function, request->offset);
		response.successful = true;
	}
	else if(request->bytes == 2)
	{
		response.value = pciConfigReadWordAt(bus, device, function, request->offset);
		response.successful = true;
	}
	else if(request->bytes == 4)
	{
		response.value = pciConfigReadDwordAt(bus, device, function, request->offset);
		response.successful = true;
	}
	else
	{
		klog("failed to read %i bytes from offset %i", request->bytes, request->offset);
		response.successful = false;
	}

	g_send_message_t(sender, &response, sizeof(response), tx);
}

void pciDriverHandleWriteConfig(g_tid sender, g_message_transaction tx, g_pci_write_config_request* request)
{
	g_pci_write_config_response response{};
	uint8_t bus = G_PCI_DEVICE_ADDRESS_BUS(request->deviceAddress);
	uint8_t device = G_PCI_DEVICE_ADDRESS_DEVICE(request->deviceAddress);
	uint8_t function = G_PCI_DEVICE_ADDRESS_FUNCTION(request->deviceAddress);

	if(request->bytes == 1)
	{
		pciConfigWriteByteAt(bus, device, function, request->offset, request->value);
		response.successful = true;
	}
	else if(request->bytes == 2)
	{
		pciConfigWriteWordAt(bus, device, function, request->offset, request->value);
		response.successful = true;
	}
	else if(request->bytes == 4)
	{
		pciConfigWriteDwordAt(bus, device, function, request->offset, request->value);
		response.successful = true;
	}
	else
	{
		klog("failed to write %i bytes to offset %i", request->bytes, request->offset);
		response.successful = false;
	}

	g_send_message_t(sender, &response, sizeof(response), tx);
}

void pciDriverHandleEnableResourceAccess(g_tid sender, g_message_transaction tx, g_pci_enable_resource_access_request* request)
{
	g_pci_enable_resource_access_response response{};
	pciEnableResourceAccessAddress(request->deviceAddress, request->enabled);
	response.successful = true;

	g_send_message_t(sender, &response, sizeof(response), tx);
}

void pciDriverHandleReadBar(g_tid sender, g_message_transaction tx, g_pci_read_bar_request* request)
{
	g_pci_read_bar_response response{};
	uint8_t bus = G_PCI_DEVICE_ADDRESS_BUS(request->deviceAddress);
	uint8_t device = G_PCI_DEVICE_ADDRESS_DEVICE(request->deviceAddress);
	uint8_t function = G_PCI_DEVICE_ADDRESS_FUNCTION(request->deviceAddress);
	response.value = pciConfigGetBARAt(bus, device, function, request->bar);
	response.successful = true;

	g_send_message_t(sender, &response, sizeof(response), tx);
}

void pciDriverHandleReadBarSize(g_tid sender, g_message_transaction tx, g_pci_read_bar_size_request* request)
{
	g_pci_read_bar_size_response response{};
	uint8_t bus = G_PCI_DEVICE_ADDRESS_BUS(request->deviceAddress);
	uint8_t device = G_PCI_DEVICE_ADDRESS_DEVICE(request->deviceAddress);
	uint8_t function = G_PCI_DEVICE_ADDRESS_FUNCTION(request->deviceAddress);
	response.value = pciConfigGetBARSizeAt(bus, device, function, request->bar);
	response.successful = true;

	g_send_message_t(sender, &response, sizeof(response), tx);
}

void pciDriverScanBus()
{
	int total = 0;
	for(uint16_t bus = 0; bus < 2 /* TODO: How much should we really scan? PCI_NUM_BUSES */; bus++)
	{
		for(uint8_t dev = 0; dev < PCI_NUM_DEVICES; dev++)
		{
			for(uint8_t fun = 0; fun < PCI_NUM_FUNCTIONS; fun++)
			{
				uint32_t header = pciConfigReadDwordAt(bus, dev, fun, PCI_CONFIG_OFF_CLASS);
				uint8_t classCode = (header >> 24) & 0xFF;
				uint8_t subClass = (header >> 16) & 0xFF;
				uint8_t progIf = (header >> 8) & 0xFF;

				if(classCode != 0xFF)
				{
					uint32_t idHeader = pciConfigReadDwordAt(bus, dev, fun, PCI_CONFIG_OFF_VENDOR_ID);
					uint16_t vendor = idHeader & 0xFFFF;
					uint16_t deviceId = (idHeader >> 16) & 0xFFFF;
					klog("pcidriver: found %02x:%02x.%u vendor=%04x device=%04x class=%02x/%02x/%02x",
					     bus, dev, fun, vendor, deviceId, classCode, subClass, progIf);

					g_mutex_acquire(deviceListLock);
					auto device = new g_pci_device();
					device->bus = bus;
					device->device = dev;
					device->function = fun;
					device->vendorId = vendor;
					device->deviceId = deviceId;
					device->classCode = classCode;
					device->subclassCode = subClass;
					device->progIf = progIf;
					device->next = deviceList;
					deviceList = device;
					deviceCount++;
					g_mutex_release(deviceListLock);

					++total;
				}
			}
		}
	}

	klog("PCI driver identified %i devices", total);
}

void pciEnableResourceAccessAddress(g_pci_device_address address, bool enabled)
{
	uint8_t bus = G_PCI_DEVICE_ADDRESS_BUS(address);
	uint8_t device = G_PCI_DEVICE_ADDRESS_DEVICE(address);
	uint8_t function = G_PCI_DEVICE_ADDRESS_FUNCTION(address);

	uint16_t command = pciConfigReadWordAt(bus, device, function, PCI_CONFIG_OFF_COMMAND);
	const uint8_t flags = 0x0007;

	if(enabled)
		command |= flags;
	else
		command &= ~flags;

	pciConfigWriteWordAt(bus, device, function, PCI_CONFIG_OFF_COMMAND, command);
}

void pciEnableResourceAccess(g_pci_device* dev, bool enabled)
{
	if(!dev)
		return;
	pciEnableResourceAccessAddress(G_PCI_DEVICE_ADDRESS_BUILD(dev->bus, dev->device, dev->function), enabled);
}

uint32_t pciConfigGetBAR(g_pci_device* dev, int bar)
{
	if(!dev)
		return 0;
	return pciConfigGetBARAt(dev->bus, dev->device, dev->function, bar);
}

uint32_t pciConfigGetBARAt(uint8_t bus, uint8_t device, uint8_t function, int bar)
{
	uint8_t offset = PCI_CONFIG_OFF_BAR0 + (0x4 * bar);

	uint32_t barVal = pciConfigReadDwordAt(bus, device, function, offset);
	uint32_t mask = (barVal & PCI_CONFIG_BAR_IO) ? 0x3 : 0xF;
	return barVal & ~mask;
}

uint32_t pciConfigGetBARSize(g_pci_device* dev, int bar)
{
	if(!dev)
		return 0;
	return pciConfigGetBARSizeAt(dev->bus, dev->device, dev->function, bar);
}

uint32_t pciConfigGetBARSizeAt(uint8_t bus, uint8_t device, uint8_t function, int bar)
{
	uint8_t offset = PCI_CONFIG_OFF_BAR0 + (0x4 * bar);
	uint32_t barVal = pciConfigReadDwordAt(bus, device, function, offset);

	pciConfigWriteDwordAt(bus, device, function, offset, 0xFFFFFFFF);
	uint32_t barSize = ~(pciConfigReadDwordAt(bus, device, function, offset) & 0xFFFFFFF0) + 1;;

	pciConfigWriteDwordAt(bus, device, function, offset, barVal);
	return barSize;
}

uint8_t pciConfigReadByte(g_pci_device* dev, uint8_t offset)
{
	if(!dev)
		return 0;
	return pciConfigReadByteAt(dev->bus, dev->device, dev->function, offset);
}

uint8_t pciConfigReadByteAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t result = pciConfigReadDwordAt(bus, device, function, offset);
	return (result >> ((offset & 3) * 8)) & 0xFF;
}

uint16_t pciConfigReadWord(g_pci_device* dev, uint8_t offset)
{
	if(!dev)
		return 0;
	return pciConfigReadWordAt(dev->bus, dev->device, dev->function, offset);
}

uint16_t pciConfigReadWordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t result = pciConfigReadDwordAt(bus, device, function, offset);
	return (result >> ((offset & 2) * 8)) & 0xFFFF;
}

uint32_t pciConfigReadDword(g_pci_device* dev, uint8_t offset)
{
	return pciConfigReadDwordAt(dev->bus, dev->device, dev->function, offset);
}

uint32_t pciConfigReadDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(bus, device, function, offset));
	auto result = g_io_port_read_dword(PCI_CONFIG_PORT_DATA);
	g_mutex_release(configSpaceLock);
	return result;
}

void pciConfigWriteByte(g_pci_device* dev, uint8_t offset, uint8_t value)
{
	if(!dev)
		return;
	pciConfigWriteByteAt(dev->bus, dev->device, dev->function, offset, value);
}

void pciConfigWriteByteAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value)
{
	uint32_t current = pciConfigReadDwordAt(bus, device, function, offset & ~3);
	uint8_t shift = (offset & 3) * 8;
	current &= ~(0xFF << shift);
	current |= (value << shift);
	pciConfigWriteDwordAt(bus, device, function, offset & ~3, current);
}

void pciConfigWriteWord(g_pci_device* dev, uint8_t offset, uint16_t value)
{
	if(!dev)
		return;
	pciConfigWriteWordAt(dev->bus, dev->device, dev->function, offset, value);
}

void pciConfigWriteWordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value)
{
	uint32_t current = pciConfigReadDwordAt(bus, device, function, offset & ~3);
	uint8_t shift = (offset & 2) * 8;
	current &= ~(0xFFFF << shift);
	current |= (value << shift);
	pciConfigWriteDwordAt(bus, device, function, offset & ~3, current);
}

void pciConfigWriteDword(g_pci_device* dev, uint8_t offset, uint32_t value)
{
	if(!dev)
		return;
	pciConfigWriteDwordAt(dev->bus, dev->device, dev->function, offset, value);
}

void pciConfigWriteDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(bus, device, function, offset));
	g_io_port_write_dword(PCI_CONFIG_PORT_DATA, value);
	g_mutex_release(configSpaceLock);
}
