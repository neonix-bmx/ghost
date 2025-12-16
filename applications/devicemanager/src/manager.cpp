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

#include "manager.hpp"
#include <ghost.h>
#include <libpci/driver.hpp>
#include <libdevice/interface.hpp>
#include <cstdio>
#include <unordered_map>

void _deviceManagerCheckPciDevices();
void _deviceManagerAwaitCommands();
void _deviceManagerHandleRegisterDevice(g_tid sender, g_message_transaction tx,
                                        g_device_manager_register_device_request* content);

static g_user_mutex devicesLock = g_mutex_initialize_r(true);
static std::unordered_map<g_device_id, device_t*> devices;
static g_device_id nextDeviceId = 1;

int main()
{
	g_tid comHandler = g_create_task((void*) _deviceManagerAwaitCommands);
	_deviceManagerCheckPciDevices();

	

	g_join(comHandler);
}

void _deviceManagerCheckPciDevices()
{
	int num;
	g_pci_device_data* devices;
	if(!pciDriverListDevices(&num, &devices))
	{
		klog("Failed to list PCI devices");
	}

bool foundVmsvga = false;
bool foundVboxVga = false;
bool foundBochsVbe = false;
bool foundE1000 = false;
bool foundAc97 = false;
bool foundAhci = false;


		for(int i = 0; i < num; i++)
	{
		uint32_t vendorId = devices[i].vendorId;
		uint32_t deviceId = devices[i].deviceId;

		bool isDisplay = (devices[i].classCode == PCI_BASE_CLASS_DISPLAY &&
		   devices[i].subclassCode == PCI_03_SUBCLASS_VGA) ||
		   (vendorId == 0x1234 && deviceId == 0x1111) || /* QEMU std VGA (Bochs VBE) */
		   (vendorId == 0x80EE && deviceId == 0xBEEF) || /* VirtualBox VGA */
		   (vendorId == 0x15AD && deviceId == 0x0405);   /* VMware SVGA2 */

		if(isDisplay)
		{
			if(vendorId == 0x15AD /* VMWare */ && deviceId == 0x0405 /* SVGA2 */)
			{
				foundVmsvga = true;
			}
			else if(vendorId == 0x80EE /* VirtualBox */ && deviceId == 0xBEEF /* VBox VGA */)
			{
				foundVboxVga = true;
			}
			else if(vendorId == 0x1234 /* QEMU std VGA (Bochs VBE) */ && deviceId == 0x1111)
			{
				foundBochsVbe = true;
			}
		}
		else if(devices[i].classCode == PCI_BASE_CLASS_NETWORK &&
		        devices[i].subclassCode == PCI_02_SUBCLASS_ETHERNET)
		{
			auto bus = G_PCI_DEVICE_ADDRESS_BUS(devices[i].deviceAddress);
			auto device = G_PCI_DEVICE_ADDRESS_DEVICE(devices[i].deviceAddress);
			auto function = G_PCI_DEVICE_ADDRESS_FUNCTION(devices[i].deviceAddress);
			klog("network device %02x:%02x.%u vendor=%04x device=%04x class=%02x/%02x/%02x",
			     bus, device, function, vendorId, deviceId,
			     devices[i].classCode, devices[i].subclassCode, devices[i].progIf);

			if(vendorId == 0x8086 && deviceId == 0x100E)
			{
				foundE1000 = true;
			}
		}

		else if(devices[i].classCode == PCI_BASE_CLASS_MULTIMEDIA &&
		        devices[i].subclassCode == PCI_04_SUBCLASS_MULTIMEDIA_AUDIO)
		{
			uint32_t vendorId = devices[i].vendorId;
			uint32_t deviceId = devices[i].deviceId;

			auto bus = G_PCI_DEVICE_ADDRESS_BUS(devices[i].deviceAddress);
			auto device = G_PCI_DEVICE_ADDRESS_DEVICE(devices[i].deviceAddress);
			auto function = G_PCI_DEVICE_ADDRESS_FUNCTION(devices[i].deviceAddress);
			klog("audio device %02x:%02x.%u vendor=%04x device=%04x class=%02x/%02x/%02x",
			     bus, device, function, vendorId, deviceId,
			     devices[i].classCode, devices[i].subclassCode, devices[i].progIf);

						if(vendorId == 0x8086 && deviceId == 0x2415)
			{
				foundAc97 = true;
			}
		}
		else if (devices[i].classCode == 0x01 && devices[i].subclassCode == 0x06)
		{
			uint32_t vendorId = devices[i].vendorId;
			uint32_t deviceId = devices[i].deviceId;
			if (vendorId == 0x8086 && deviceId == 0x2829)
			{
				foundAhci = true;
			}
		}
	}
	pciDriverFreeDeviceList(devices);

		// TODO Implement something more sophisticated
	if(foundVmsvga)
	{
		klog("starting VMSVGA driver");
		g_spawn("/applications/vmsvgadriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
		else if(foundVboxVga)
	{
		klog("starting VBox VGA driver");
		g_spawn("/applications/vboxvgadriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
	else if(foundBochsVbe)
	{
		// Prefer VBox driver for Bochs/QEMU std VGA as well (BGA-compatible)
		klog("starting VBox VGA driver for Bochs/QEMU std VGA");
		g_spawn("/applications/vboxvgadriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
	else
	{
		klog("starting EFI FB driver");
		g_spawn("/applications/efifbdriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}



	if(foundE1000)
	{
		klog("starting ethernet driver");
		g_spawn("/applications/ethdriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
	else
	{
		klog("no supported ethernet device detected (expecting Intel 82540EM 8086:100E)");
	}

		if(foundAc97)
	{
		klog("starting AC97 audio driver");
		g_spawn("/applications/ac97driver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
	else
	{
		klog("no supported AC97 controller detected (expecting Intel 82801AA 8086:2415)");
	}

	if (foundAhci)
	{
		klog("starting AHCI driver");
		g_spawn("/applications/ahcidriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
}

void _deviceManagerAwaitCommands()
{
	if(!g_task_register_name(G_DEVICE_MANAGER_NAME))
	{
		klog("failed to register as %s", G_DEVICE_MANAGER_NAME);
		g_exit(-1);
	}

	size_t bufLen = 1024;
	uint8_t buf[bufLen];

	while(true)
	{
		auto status = g_receive_message(buf, bufLen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;
		auto message = (g_message_header*) buf;
		auto content = (g_device_manager_header*) G_MESSAGE_CONTENT(message);

			if(content->command == G_DEVICE_MANAGER_REGISTER_DEVICE)
	{
				auto req = (g_device_manager_register_device_request*) content;
		klog("devicemanager: received register request from task %i (type=%i handler=%i)", message->sender, req->type, req->handler);
		_deviceManagerHandleRegisterDevice(message->sender, message->transaction, req);

	}

	}
}

void _deviceManagerHandleRegisterDevice(g_tid sender, g_message_transaction tx,
                                        g_device_manager_register_device_request* content)
{
	g_mutex_acquire(devicesLock);
	auto id = nextDeviceId++;
	auto device = new device_t();
	device->id = id;
	device->handler = content->handler;
	device->type = content->type;
	devices[id] = device;
	g_mutex_release(devicesLock);

		// Respond to registerer
	g_device_manager_register_device_response response{};
	response.status = G_DEVICE_MANAGER_SUCCESS;
	response.id = id;
	klog("devicemanager: registering device id=%u type=%i handler=%i", id, content->type, content->handler);
	g_send_message_t(sender, &response, sizeof(response), tx);

	// Post to topic
	g_device_event_device_registered event{};
	event.header.event = G_DEVICE_EVENT_DEVICE_REGISTERED;
	event.id = device->id;
	event.type = content->type;
	event.driver = device->handler;
	klog("devicemanager: broadcast device_registered id=%u type=%i handler=%i", event.id, event.type, event.driver);
	g_send_topic_message(G_DEVICE_EVENT_TOPIC, &event, sizeof(event));

}
