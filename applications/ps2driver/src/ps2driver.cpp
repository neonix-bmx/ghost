/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "ps2driver.hpp"

#include <libps2/ps2.hpp>

#include <ghost.h>
#include <stdint.h>
#include <stdio.h>

g_fd keyboardRead;
g_fd keyboardWrite;
g_fd mouseRead;
g_fd mouseWrite;

// Ring buffers to decouple producer/consumer
static g_ps2_mouse_packet mouseBuf[512];
static int mouseHead = 0, mouseTail = 0;
static uint8_t keyBuf[512];
static int keyHead = 0, keyTail = 0;

// Stats counters for diagnostics
static volatile uint64_t mouseProduced = 0;
static volatile uint64_t mouseFlushed = 0;
static volatile uint64_t mouseDropped = 0;
static volatile uint64_t keyProduced = 0;
static volatile uint64_t keyFlushed = 0;
static volatile uint64_t keyDropped = 0;

static void ps2FlushLoop();
static void ps2StatsLoop();




g_tid keyboardPartnerTask = G_TID_NONE;
g_tid mousePartnerTask = G_TID_NONE;


int main()
{
	if(!g_task_register_name(G_PS2_DRIVER_NAME))
	{
		klog("ps2driver: could not register with task name '%s'", (char*) G_PS2_DRIVER_NAME);
		return -1;
	}

	ps2DriverInitialize();
	ps2DriverReceiveMessages();
	return 0;
}

void ps2DriverInitialize()
{
		// Use non-blocking pipes; background flusher handles writes
	if(g_pipe_b(&keyboardWrite, &keyboardRead, false) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("ps2driver: failed to open pipe for keyboard data");
		return;
	}
	if(g_fs_publish_pipe(G_PS2_DEVICE_KEYBOARD_REL, keyboardRead, false) != G_FS_PUBLISH_PIPE_SUCCESS)
	{
		klog("ps2driver: failed to publish %s", G_PS2_DEVICE_KEYBOARD);
		return;
	}

	if(g_pipe_b(&mouseWrite, &mouseRead, false) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("ps2driver: failed to open pipe for mouse data");
		return;
	}
	if(g_fs_publish_pipe(G_PS2_DEVICE_MOUSE_REL, mouseRead, false) != G_FS_PUBLISH_PIPE_SUCCESS)
	{
		klog("ps2driver: failed to publish %s", G_PS2_DEVICE_MOUSE);
		return;
	}



		// Background flusher to drain ring buffers cooperatively
	g_create_task((void*) ps2FlushLoop);
	g_create_task((void*) ps2StatsLoop);

	ps2Initialize(ps2MouseCallback, ps2KeyboardCallback);
}



static void flushMouse()
{
	while(mouseHead != mouseTail)
	{
			int wrote = g_write(mouseWrite, &mouseBuf[mouseTail], sizeof(g_ps2_mouse_packet));
		if(wrote <= 0) break;
		mouseFlushed++;
		mouseTail = (mouseTail + 1) % (int)(sizeof(mouseBuf)/sizeof(mouseBuf[0]));
	}
}


static void flushKeyboard()
{
	while(keyHead != keyTail)
	{
			int wrote = g_write(keyboardWrite, &keyBuf[keyTail], 1);
		if(wrote <= 0) break;
		keyFlushed++;
		keyTail = (keyTail + 1) % (int)(sizeof(keyBuf)/sizeof(keyBuf[0]));
	}
}

static void ps2StatsLoop()
{
	for(;;)
	{
		klog("ps2 stats: prod=%llu flushed=%llu dropped=%llu head=%d tail=%d", (unsigned long long) mouseProduced, (unsigned long long) mouseFlushed, (unsigned long long) mouseDropped, mouseHead, mouseTail);
		mouseProduced = mouseFlushed = mouseDropped = 0;
		klog("kbd stats: prod=%llu flushed=%llu dropped=%llu head=%d tail=%d", (unsigned long long) keyProduced, (unsigned long long) keyFlushed, (unsigned long long) keyDropped, keyHead, keyTail);
		keyProduced = keyFlushed = keyDropped = 0;
		g_sleep(1000);
	}
}


static void ps2FlushLoop()
{
		for(;;)
	{
				flushMouse();
		flushKeyboard();
		g_sleep(1);
	}
}




void ps2MouseCallback(int16_t x, int16_t y, uint8_t flags, int8_t scroll)
{
				g_ps2_mouse_packet packet;
	packet.x = x;
	packet.y = y;
	packet.flags = flags;
	packet.scroll = scroll;

	int next = (mouseHead + 1) % (int)(sizeof(mouseBuf)/sizeof(mouseBuf[0]));
	if(next == mouseTail)
	{
		mouseTail = (mouseTail + 1) % (int)(sizeof(mouseBuf)/sizeof(mouseBuf[0])); // drop oldest
		mouseDropped++;
	}
	mouseBuf[mouseHead] = packet;
	mouseHead = next;
	mouseProduced++;
}



void ps2KeyboardCallback(uint8_t c)
{
	if(c == 0x3f) // F5
	{
		g_dump();
	}

			int next = (keyHead + 1) % (int)(sizeof(keyBuf)/sizeof(keyBuf[0]));
	if(next == keyTail)
	{
		keyTail = (keyTail + 1) % (int)(sizeof(keyBuf)/sizeof(keyBuf[0])); // drop oldest
		keyDropped++;
	}
	keyBuf[keyHead] = c;
	keyHead = next;
	keyProduced++;
}




void ps2DriverReceiveMessages()
{
	size_t buflen = sizeof(g_message_header) + sizeof(g_ps2_initialize_request) /*TODO*/;
	uint8_t buf[buflen];

	for(;;)
	{
		auto status = g_receive_message(buf, buflen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			klog("error receiving message, retrying");
			continue;
		}

		g_message_header* header = (g_message_header*) buf;
		g_ps2_request_header* request = (g_ps2_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_PS2_COMMAND_INITIALIZE)
		{
			ps2HandleCommandInitialize((g_ps2_initialize_request*) request, header->sender, header->transaction);
		}
		else
		{
			klog("vbedriver: received unknown command %i from task %i", request->command, header->sender);
		}
	}
}

void ps2HandleCommandInitialize(g_ps2_initialize_request* request, g_tid requestingTaskId,
                                g_message_transaction requestTransaction)
{
	keyboardPartnerTask = request->keyboardPartnerTask;
	mousePartnerTask = request->mousePartnerTask;

	g_ps2_initialize_response response;
	response.status = G_PS2_INITIALIZE_SUCCESS;

	g_send_message_t(requestingTaskId, &response, sizeof(g_ps2_initialize_response), requestTransaction);
}
