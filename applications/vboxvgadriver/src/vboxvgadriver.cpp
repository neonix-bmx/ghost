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
#include <ghost/system.h>

#include <libdevice/manager.hpp>
#include <libpci/driver.hpp>
#include <libvideo/videodriver.hpp>

#include <cstdio>
#include <cstring>

namespace
{

constexpr uint16_t BGA_INDEX_PORT = 0x1CE;
constexpr uint16_t BGA_DATA_PORT = 0x1CF;

constexpr uint16_t BGA_INDEX_ID = 0;
constexpr uint16_t BGA_INDEX_XRES = 1;
constexpr uint16_t BGA_INDEX_YRES = 2;
constexpr uint16_t BGA_INDEX_BPP = 3;
constexpr uint16_t BGA_INDEX_ENABLE = 4;
constexpr uint16_t BGA_INDEX_VIRT_WIDTH = 5;
constexpr uint16_t BGA_INDEX_VIRT_HEIGHT = 6;
constexpr uint16_t BGA_INDEX_X_OFFSET = 7;
constexpr uint16_t BGA_INDEX_Y_OFFSET = 8;

constexpr uint16_t BGA_ENABLE = 0x0001;
constexpr uint16_t BGA_LFB_ENABLED = 0x0040;

constexpr uint16_t VBOX_VENDOR = 0x80EE;
constexpr uint16_t VBOX_DEVICE_VGA = 0xBEEF;
// Bochs/QEMU stdvga (for fallback)
constexpr uint16_t BOCHS_VENDOR = 0x1234;
constexpr uint16_t BOCHS_DEVICE_STD = 0x1111;

struct vbox_context
{
	g_pci_device_address deviceAddress = 0;
	g_address fbPhys = 0;
	g_address fbSize = 0;
	void* fbMapping = nullptr;
	g_device_id deviceId = 0;
	bool ready = false;
} g_ctx;

// Forward declarations
bool bgaSetMode(uint16_t width, uint16_t height, uint16_t bpp);

void bgaWrite(uint16_t index, uint16_t value)
{
	g_io_port_write_word(BGA_INDEX_PORT, index);
	g_io_port_write_word(BGA_DATA_PORT, value);
}

uint16_t bgaRead(uint16_t index)
{
	g_io_port_write_word(BGA_INDEX_PORT, index);
	return g_io_port_read_word(BGA_DATA_PORT);
}

bool bgaSetMode(uint16_t width, uint16_t height, uint16_t bpp)
{
	if(bpp != 32 && bpp != 24)
		return false;

	bgaWrite(BGA_INDEX_ENABLE, 0);
	bgaWrite(BGA_INDEX_XRES, width);
	bgaWrite(BGA_INDEX_YRES, height);
	bgaWrite(BGA_INDEX_VIRT_WIDTH, width);
	bgaWrite(BGA_INDEX_VIRT_HEIGHT, height);
	bgaWrite(BGA_INDEX_X_OFFSET, 0);
	bgaWrite(BGA_INDEX_Y_OFFSET, 0);
	bgaWrite(BGA_INDEX_BPP, bpp);
	bgaWrite(BGA_INDEX_ENABLE, BGA_ENABLE | BGA_LFB_ENABLED);

	return bgaRead(BGA_INDEX_XRES) == width && bgaRead(BGA_INDEX_YRES) == height;
}

bool detectVBoxController()
{
	klog("vboxvgadriver: detectVBoxController begin");
	int count = 0;
	g_pci_device_data* devices = nullptr;
	klog("vboxvgadriver: requesting PCI device list");
	if(!pciDriverListDevices(&count, &devices))
	{
		klog("vboxvgadriver: failed to enumerate PCI devices");
		return false;
	}

	bool found = false;
	klog("vboxvgadriver: scanning %i PCI devices", count);
	for(int i = 0; i < count; ++i)
	{
		auto& dev = devices[i];
		uint32_t vendorId = dev.vendorId;
		uint32_t deviceId = dev.deviceId;
		klog("vboxvgadriver: candidate %02x:%02x.%u vendor=%04x device=%04x class=%02x/%02x/%02x",
		     G_PCI_DEVICE_ADDRESS_BUS(dev.deviceAddress),
		     G_PCI_DEVICE_ADDRESS_DEVICE(dev.deviceAddress),
		     G_PCI_DEVICE_ADDRESS_FUNCTION(dev.deviceAddress),
		     vendorId, deviceId, dev.classCode, dev.subclassCode, dev.progIf);

		bool isDisplay = (dev.classCode == PCI_BASE_CLASS_DISPLAY && dev.subclassCode == PCI_03_SUBCLASS_VGA);
		if((vendorId == VBOX_VENDOR && deviceId == VBOX_DEVICE_VGA) ||
		   (vendorId == BOCHS_VENDOR && deviceId == BOCHS_DEVICE_STD))
		{
			g_ctx.deviceAddress = dev.deviceAddress;
			found = true;
			klog("vboxvgadriver: using %s device %04x:%04x (isDisplay=%i)", (vendorId == VBOX_VENDOR ? "VBox" : "Bochs/QEMU"), vendorId, deviceId, isDisplay);
			break;
		}
	}
	pciDriverFreeDeviceList(devices);

	if(!found)
	{
		klog("vboxvgadriver: VBox VGA controller not detected");
		return false;
	}

	if(!pciDriverEnableResourceAccess(g_ctx.deviceAddress, true))
	{
		klog("vboxvgadriver: failed to enable device resources");
		return false;
	}
	klog("vboxvgadriver: enabled resource access");

	if(!pciDriverReadBAR(g_ctx.deviceAddress, 0, &g_ctx.fbPhys))
	{
		klog("vboxvgadriver: failed to read BAR0");
		return false;
	}
	// Mask flag bits (memory BAR) and validate.
	g_address fbBarRaw = g_ctx.fbPhys;
	g_ctx.fbPhys &= ~((g_address) 0xF);
	klog("vboxvgadriver: BAR0 raw=%p masked phys=%p", (void*) fbBarRaw, (void*) g_ctx.fbPhys);
	if(g_ctx.fbPhys == 0)
	{
		klog("vboxvgadriver: BAR0 masked to zero, aborting");
		return false;
	}
	// Accept high BAR addresses and rely on g_map_mmio to validate the mapping.

	// Force a capped framebuffer size to avoid exhausting the MMIO mapper / page tables.
	// VBox typically reports 16MB. Map 8MB here to cover 1600x1200x32bpp (~7.6MB).
	// Default to 16MB to cover 1920x1080x4 comfortably
const g_address kDefaultFbSize = 16 * 1024 * 1024;	g_ctx.fbSize = kDefaultFbSize;
	klog("vboxvgadriver: forcing framebuffer size to %u bytes", (uint32_t) g_ctx.fbSize);

	// Extra tracing to see whether we reach g_map_mmio at all.
	klog("vboxvgadriver: mapping framebuffer phys=%p size=%u", (void*) g_ctx.fbPhys, (uint32_t) g_ctx.fbSize);
	klog("vboxvgadriver: before g_map_mmio");
	g_ctx.fbMapping = g_map_mmio((void*) g_ctx.fbPhys, g_ctx.fbSize);
	klog("vboxvgadriver: after g_map_mmio result=%p", g_ctx.fbMapping);
	if(!g_ctx.fbMapping)
	{
		klog("vboxvgadriver: failed to map framebuffer at %p", (void*) g_ctx.fbPhys);
		return false;
	}
	klog("vboxvgadriver: framebuffer mapped at %p", g_ctx.fbMapping);

	uint16_t id = bgaRead(BGA_INDEX_ID);
	klog("vboxvgadriver: BGA version %x", id);
	if(id < 0xB0C0)
	{
		klog("vboxvgadriver: unsupported BGA version %x", id);
		return false;
	}

	g_ctx.ready = true;
	klog("vboxvgadriver: detected controller, framebuffer at %p (%u bytes)", (void*) g_ctx.fbPhys, (uint32_t) g_ctx.fbSize);
	klog("vboxvgadriver: detectVBoxController returning success ready=%i fbPhys=%p fbSize=%u mapping=%p", g_ctx.ready, (void*) g_ctx.fbPhys, (uint32_t) g_ctx.fbSize, g_ctx.fbMapping);
	return true;
}

void handleSetMode(g_video_set_mode_request* request, g_tid sender, g_message_transaction tx)
{
	g_video_set_mode_response response{};
	response.status = G_VIDEO_SET_MODE_STATUS_FAILED;
	klog("vboxvgadriver: set mode request %ux%u@%u from task %i",
	     request->width, request->height, request->bpp, sender);

	if(g_ctx.ready && g_ctx.fbMapping)
	{
		uint32_t bppBytes = (uint32_t) (request->bpp / 8);
		// For debugging: clamp to a smaller mode (1024x768@32) to rule out VBox refresh quirks.
		if(request->width > 1024 || request->height > 768 || request->bpp != 32)
		{
			request->width = 1024;
			request->height = 768;
			request->bpp = 32;
			bppBytes = 4;
			klog("vboxvgadriver: forcing mode to 1024x768@32 for debug");
		}
		uint32_t requiredSize = request->width * request->height * bppBytes;
		if(requiredSize > g_ctx.fbSize)
		{
			klog("vboxvgadriver: rejecting mode %ux%u@%u, requires %u bytes > mapped %u", request->width, request->height, request->bpp, requiredSize, (uint32_t) g_ctx.fbSize);
		}
		else
		{
			klog("vboxvgadriver: handling set mode %ux%u@%u (ready=%i mapping=%p)",
			     request->width, request->height, request->bpp, g_ctx.ready, g_ctx.fbMapping);
			if(bgaSetMode((uint16_t) request->width, (uint16_t) request->height, (uint16_t) request->bpp))
			{
				// Re-apply virt width/height after enable (VBox sometimes zeros virt width).
				bgaWrite(BGA_INDEX_VIRT_WIDTH, (uint16_t) request->width);
				bgaWrite(BGA_INDEX_VIRT_HEIGHT, (uint16_t) request->height);

				// BGA register dump after mode set
				uint16_t regX = bgaRead(BGA_INDEX_XRES);
				uint16_t regY = bgaRead(BGA_INDEX_YRES);
				uint16_t regBpp = bgaRead(BGA_INDEX_BPP);
				uint16_t regVirtW = bgaRead(BGA_INDEX_VIRT_WIDTH);
				uint16_t regVirtH = bgaRead(BGA_INDEX_VIRT_HEIGHT);
				uint16_t regEnable = bgaRead(BGA_INDEX_ENABLE);
				// Use requested width for pitch (BGA virt width reads as 64/0 in VBox, unusable).
				uint32_t pitch = request->width * bppBytes;
				uint32_t pitchSize = pitch * request->height;
				klog("vboxvgadriver: mode set pitch=%u needed=%u mapped=%u regs x=%u y=%u bpp=%u virtw=%u virth=%u enable=%04x",
				     pitch, pitchSize, (uint32_t) g_ctx.fbSize, regX, regY, regBpp, regVirtW, regVirtH, regEnable);

				void* shared = g_share_mem(g_ctx.fbMapping, g_ctx.fbSize, sender);
				klog("vboxvgadriver: share framebuffer (%u bytes) result=%p to task %i", (uint32_t) g_ctx.fbSize, shared, sender);
				if(shared)
				{
					response.status = G_VIDEO_SET_MODE_STATUS_SUCCESS;
					response.mode_info.lfb = (g_address) shared;
					response.mode_info.resX = (uint16_t) request->width;
					response.mode_info.resY = (uint16_t) request->height;
					response.mode_info.bpp = (uint8_t) request->bpp;
					response.mode_info.bpsl = (uint16_t) pitch;
					response.mode_info.explicit_update = false;
				}
				else
				{
					klog("vboxvgadriver: failed to share framebuffer with task %i", sender);
				}

				// Draw a test pattern directly into the mapped framebuffer using the computed pitch.
				klog("vboxvgadriver: draw test pattern pitch=%u bppBytes=%u", pitch, bppBytes);
				uint8_t* fb = (uint8_t*) g_ctx.fbMapping;
				for(uint32_t y = 0; y < request->height; ++y)
				{
					for(uint32_t x = 0; x < request->width; ++x)
					{
						size_t off = y * pitch + x * bppBytes;
						if(off + bppBytes <= g_ctx.fbSize)
						{
							if(bppBytes >= 1) fb[off + 0] = (uint8_t) (x & 0xFF);
							if(bppBytes >= 2) fb[off + 1] = (uint8_t) (y & 0xFF);
							if(bppBytes >= 3) fb[off + 2] = 0x7F;
							if(bppBytes == 4) fb[off + 3] = 0xFF;
						}
					}
				}

				// Read back a couple of pixels after writing (before flush)
				uint32_t* p = (uint32_t*) g_ctx.fbMapping;
				uint32_t p0 = p[0];
				uint32_t p1 = p[(pitch/4) * 10 + 10];
				klog("fb readback after draw: p0=%08x p1=%08x", p0, p1);

				// Flush framebuffer range to push writes out of cache (VBox sometimes delays LFB updates).
				size_t flushBytes = pitchSize;
				if(flushBytes > g_ctx.fbSize) flushBytes = g_ctx.fbSize;
				for(size_t off = 0; off < flushBytes; off += 64)
				{
					__asm__ volatile("clflush (%0)" :: "r" (fb + off));
				}
			}
			else
			{
				klog("vboxvgadriver: BGA rejected mode %ux%u@%u", request->width, request->height, request->bpp);
			}
		}
	}
	else
	{
		klog("vboxvgadriver: not ready for mode set (ready=%i mapping=%p)", g_ctx.ready, g_ctx.fbMapping);
	}

	g_send_message_t(sender, &response, sizeof(response), tx);
}

void driverLoop()
{
	size_t bufSize = sizeof(g_message_header) + 1024;
	uint8_t buffer[bufSize];

	while(true)
	{
		auto status = g_receive_message(buffer, bufSize);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;

		auto header = reinterpret_cast<g_message_header*>(buffer);
		auto request = reinterpret_cast<g_video_request_header*>(G_MESSAGE_CONTENT(buffer));
		klog("vboxvgadriver: received command %i from task %i", request->command, header->sender);

		if(request->command == G_VIDEO_COMMAND_SET_MODE)
		{
			handleSetMode(reinterpret_cast<g_video_set_mode_request*>(request), header->sender, header->transaction);
		}
		else if(request->command == G_VIDEO_COMMAND_UPDATE)
		{
			// Nothing required, framebuffer updates automatically.
			g_yield();
		}
		else
		{
			klog("vboxvgadriver: unknown command %i from task %i", request->command, header->sender);
		}
	}
}

} // namespace

int main()
{
	if(!g_task_register_name("vboxvgadriver"))
	{
		klog("vboxvgadriver: failed to register task name");
		return -1;
	}

	klog("vboxvgadriver: initializing");
	g_task_await_by_name(G_PCI_DRIVER_NAME);

	if(!detectVBoxController())
	{
		klog("vboxvgadriver: detectVBoxController failed, exiting");
		return -1;
	}
	klog("vboxvgadriver: detectVBoxController succeeded");
	klog("vboxvgadriver: detectVBoxController succeeded");

	klog("vboxvgadriver: registering video device with devicemanager");
	if(!deviceManagerRegisterDevice(G_DEVICE_TYPE_VIDEO, g_get_tid(), &g_ctx.deviceId))
	{
		klog("vboxvgadriver: failed to register with device manager");
		return -1;
	}

	klog("vboxvgadriver: registered video device %i, entering driver loop", g_ctx.deviceId);
	driverLoop();
	return 0;
}