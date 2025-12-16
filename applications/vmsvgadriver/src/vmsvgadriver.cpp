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

#include "vmsvgadriver.hpp"
#include "svga.hpp"

#include <libdevice/manager.hpp>
#include <libpci/driver.hpp>
#include <ghost.h>
#include <cstdio>
#include <libvideo/videodriver.hpp>

static bool g_svga_initialized = false;
g_device_id deviceId;

int main()
{
	g_task_register_name("vmsvgadriver");
	klog("started");
	g_task_await_by_name(G_PCI_DRIVER_NAME);

	g_svga_initialized = svgaInitializeDevice();
	if(!g_svga_initialized)
	{
				klog("failed to initialize SVGA controller");
		g_exit(-1);
	}

		if(!deviceManagerRegisterDevice(G_DEVICE_TYPE_VIDEO, g_get_tid(), &deviceId))
	{
		klog("failed to register device with device manager");
		g_exit(-1);
	}
	klog("registered VMSVGA device %i", deviceId);

	vmsvgaDriverReceiveMessages();
	return 0;
}

void vmsvgaDriverReceiveMessages()
{
	size_t buflen = sizeof(g_message_header) + 1024;
	uint8_t buf[buflen];

	for(;;)
	{
		auto status = g_receive_message(buf, buflen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;

		auto header = (g_message_header*) buf;
		auto request = (g_video_request_header*) G_MESSAGE_CONTENT(buf);

				
				if(request->command == G_VIDEO_COMMAND_SET_MODE)
		{
			auto modeSetRequest = (g_video_set_mode_request*) request;
			g_video_set_mode_response response{};
			response.status = G_VIDEO_SET_MODE_STATUS_FAILED; // Default: başarısız

			if(g_svga_initialized)
			{
				klog("vmsvgadriver: setting video mode to %ix%i@%i",
					modeSetRequest->width, modeSetRequest->height, modeSetRequest->bpp);
				svgaSetMode(modeSetRequest->width, modeSetRequest->height, modeSetRequest->bpp);

				void* fb = svgaGetFb();
				size_t fbsz = svgaGetFbSize();
				klog("vmsvgadriver: fb pointer = %p, fb size = %u", fb, (unsigned)fbsz);

				if(fb && fbsz)
				{
					void* addressInRequestersSpace = g_share_mem(fb, fbsz, header->sender);
					klog("vmsvgadriver: g_share_mem returned %p", addressInRequestersSpace);
					uint32_t pitchReg = svgaReadReg(SVGA_REG_BYTES_PER_LINE);
					uint32_t pitch = pitchReg ? pitchReg : (uint32_t)modeSetRequest->width * (modeSetRequest->bpp / 8);
					klog("vmsvgadriver: pitch bytes-per-line reg=%u using=%u", pitchReg, pitch);

					if(addressInRequestersSpace)
					{
											response.status = G_VIDEO_SET_MODE_STATUS_SUCCESS;
					response.mode_info.lfb = (g_address) addressInRequestersSpace;
					response.mode_info.resX = modeSetRequest->width;
					response.mode_info.resY = modeSetRequest->height;
					response.mode_info.bpp = modeSetRequest->bpp;
					// Prefer device-reported pitch if available
					uint32_t pitchReg = svgaReadReg(SVGA_REG_BYTES_PER_LINE);
					uint32_t pitch = pitchReg ? pitchReg : (uint32_t)modeSetRequest->width * (modeSetRequest->bpp / 8);
					response.mode_info.bpsl = (uint16_t) pitch;
					response.mode_info.explicit_update = true;

					}
					else
					{
						klog("vmsvgadriver: failed to share framebuffer with task %i", header->sender);
					}
				}
				else
				{
					klog("vmsvgadriver: fb pointer or size invalid, fb=%p, size=%u", fb, (unsigned)fbsz);
				}
			}
			else
			{
				klog("vmsvgadriver: svga not initialized!");
			}
			g_send_message_t(header->sender, &response, sizeof(response), header->transaction);
		}
		else if(request->command == G_VIDEO_COMMAND_UPDATE)
		{
			auto upd = (g_video_update_request*) request;
			// Clamp to current mode dimensions (SVGA FIFO update requires valid rect)
			uint32_t w = upd->width ? upd->width : 1;
			uint32_t h = upd->height ? upd->height : 1;
			svgaUpdate(upd->x, upd->y, w, h);
		}
	}
}

