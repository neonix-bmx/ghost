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

#include "libpci/driver.hpp"
#include <cstdio>
#include <cstring>
#include <ghost/malloc.h>
#include <ghost/mutex.h>
#include <ghost/messages.h>

namespace
{
g_tid g_pciDriverTid = 0;
g_user_mutex g_pciRequestLock = g_mutex_initialize_r(true);

bool pciEnsureDriver()
{
	if(g_pciDriverTid)
		return true;

	g_pciDriverTid = g_task_await_by_name(G_PCI_DRIVER_NAME);
	return g_pciDriverTid != 0;
}

bool pciSendRequest(const void* request, size_t requestLength, uint8_t* responseBuffer, size_t bufferSize, size_t* outResponseLength)
{
	if(!pciEnsureDriver())
		return false;

	g_message_transaction tx = g_get_message_tx_id();
	if(g_send_message_t(g_pciDriverTid, const_cast<void*>(request), requestLength, tx) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		klog("libpci: failed to send PCI request");
		return false;
	}

	uint8_t message[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	g_message_receive_status status = g_receive_message_t(message, sizeof(message), tx);
	if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		klog("libpci: failed to receive PCI response (%i)", status);
		return false;
	}

	g_message_header* header = reinterpret_cast<g_message_header*>(message);
	if(header->length > bufferSize)
	{
		klog("libpci: PCI response too large (%zu > %zu)", header->length, bufferSize);
		return false;
	}
		std::memcpy(responseBuffer, G_MESSAGE_CONTENT(message), header->length);
	*outResponseLength = header->length;
	return true;
}

} // namespace

bool pciDriverListDevices(int* outCount, g_pci_device_data** outData)
{
	g_mutex_acquire(g_pciRequestLock);
	bool success = false;
	g_pci_device_data* devices = nullptr;
	const char* failureReason = nullptr;
	int receivedDevices = 0;

	do
	{
		uint8_t responseBuffer[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
		size_t responseLength = 0;

		g_pci_list_devices_request request{};
		request.header.command = G_PCI_LIST_DEVICES;
		klog("libpci: pciDriverListDevices sending request");
		if(!pciSendRequest(&request, sizeof(request), responseBuffer, sizeof(responseBuffer), &responseLength))
		{
			failureReason = "message";
			break;
		}

		if(responseLength < sizeof(g_pci_list_devices_count_response))
		{
			failureReason = "short-response";
			break;
		}

		auto response = reinterpret_cast<g_pci_list_devices_count_response*>(responseBuffer);
		receivedDevices = response->numDevices;
		klog("libpci: pciDriverListDevices expects %d devices", receivedDevices);
		size_t dataSize = receivedDevices * sizeof(g_pci_device_data);
		if(responseLength != sizeof(g_pci_list_devices_count_response) + dataSize)
		{
			failureReason = "size-mismatch";
			break;
		}

		devices = (g_pci_device_data*) malloc(dataSize ? dataSize : 1);
		if(dataSize)
			std::memcpy(devices, responseBuffer + sizeof(g_pci_list_devices_count_response), dataSize);
		if(dataSize)
			klog("libpci: pciDriverListDevices received %zu bytes", dataSize);
		else
			klog("libpci: pciDriverListDevices received empty list");

		*outCount = receivedDevices;
		*outData = devices;
		devices = nullptr;
		success = true;
	} while(false);

	if(devices)
		free(devices);
	g_mutex_release(g_pciRequestLock);
	if(!success && failureReason)
		klog("libpci: pciDriverListDevices failed (%s, devices=%d)", failureReason, receivedDevices);
	return success;
}

void pciDriverFreeDeviceList(g_pci_device_data* deviceList)
{
	free(deviceList);
}

bool pciDriverReadConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t* outValue)
{
	g_mutex_acquire(g_pciRequestLock);
	bool success = false;
	uint8_t responseBuffer[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	size_t responseLength = 0;

	g_pci_read_config_request request{};
	request.header.command = G_PCI_READ_CONFIG;
	request.deviceAddress = address;
	request.offset = offset;
	request.bytes = bytes;

	if(pciSendRequest(&request, sizeof(request), responseBuffer, sizeof(responseBuffer), &responseLength) &&
	   responseLength == sizeof(g_pci_read_config_response))
	{
		auto response = reinterpret_cast<g_pci_read_config_response*>(responseBuffer);
		success = response->successful;
		if(success && outValue)
			*outValue = response->value;
	}

	g_mutex_release(g_pciRequestLock);
	return success;
}

bool pciDriverWriteConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t value)
{
	g_mutex_acquire(g_pciRequestLock);
	bool success = false;
	uint8_t responseBuffer[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	size_t responseLength = 0;

	g_pci_write_config_request request{};
	request.header.command = G_PCI_WRITE_CONFIG;
	request.deviceAddress = address;
	request.offset = offset;
	request.bytes = bytes;
	request.value = value;

	if(pciSendRequest(&request, sizeof(request), responseBuffer, sizeof(responseBuffer), &responseLength) &&
	   responseLength == sizeof(g_pci_write_config_response))
	{
		auto response = reinterpret_cast<g_pci_write_config_response*>(responseBuffer);
		success = response->successful;
	}

	g_mutex_release(g_pciRequestLock);
	return success;
}

bool pciDriverEnableResourceAccess(g_pci_device_address address, bool enabled)
{
	g_mutex_acquire(g_pciRequestLock);
	bool success = false;
	uint8_t responseBuffer[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	size_t responseLength = 0;

	g_pci_enable_resource_access_request request{};
	request.header.command = G_PCI_ENABLE_RESOURCE_ACCESS;
	request.deviceAddress = address;
	request.enabled = enabled;

	if(pciSendRequest(&request, sizeof(request), responseBuffer, sizeof(responseBuffer), &responseLength) &&
	   responseLength == sizeof(g_pci_enable_resource_access_response))
	{
		auto response = reinterpret_cast<g_pci_enable_resource_access_response*>(responseBuffer);
		success = response->successful;
	}

	g_mutex_release(g_pciRequestLock);
	return success;
}

bool pciDriverReadBAR(g_pci_device_address address, uint8_t bar, g_address* outValue)
{
	g_mutex_acquire(g_pciRequestLock);
	bool success = false;
	uint8_t responseBuffer[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	size_t responseLength = 0;

	g_pci_read_bar_request request{};
	request.header.command = G_PCI_READ_BAR;
	request.deviceAddress = address;
	request.bar = bar;

	if(pciSendRequest(&request, sizeof(request), responseBuffer, sizeof(responseBuffer), &responseLength) &&
	   responseLength == sizeof(g_pci_read_bar_response))
	{
		auto response = reinterpret_cast<g_pci_read_bar_response*>(responseBuffer);
		success = response->successful;
		if(success && outValue)
			*outValue = response->value;
	}

	g_mutex_release(g_pciRequestLock);
	return success;
}

bool pciDriverReadBARSize(g_pci_device_address address, uint8_t bar, g_address* outValue)
{
	g_mutex_acquire(g_pciRequestLock);
	bool success = false;
	uint8_t responseBuffer[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	size_t responseLength = 0;

	g_pci_read_bar_size_request request{};
	request.header.command = G_PCI_READ_BAR_SIZE;
	request.deviceAddress = address;
	request.bar = bar;

	if(pciSendRequest(&request, sizeof(request), responseBuffer, sizeof(responseBuffer), &responseLength) &&
	   responseLength == sizeof(g_pci_read_bar_size_response))
	{
		auto response = reinterpret_cast<g_pci_read_bar_size_response*>(responseBuffer);
		success = response->successful;
		if(success && outValue)
			*outValue = response->value;
	}

	g_mutex_release(g_pciRequestLock);
	return success;
}
