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

#include <ghost.h>
#include <libps2driver/ps2driver.hpp>

#include <cstring>

#include "libinput/mouse/mouse.hpp"

g_mouse_info g_mouse::readMouse(g_fd in)
{
	g_ps2_mouse_packet packet;

	// Keep last-known button state, but don't replay movement/scroll when no packet arrives.
	static g_mouse_info last{};
	static uint8_t pending[sizeof(g_ps2_mouse_packet)];
	static size_t pending_len = 0;

	while(pending_len < sizeof(pending))
	{
		g_fs_read_status status = G_FS_READ_SUCCESSFUL;
		int32_t rd = g_read_s(in, pending + pending_len, sizeof(pending) - pending_len, &status);
		if(rd <= 0)
		{
			if(status != G_FS_READ_BUSY)
				pending_len = 0;
			// No new packet: report no motion/scroll, keep button latch
			last.x = 0;
			last.y = 0;
			last.scroll = 0;
			return last;
		}
		pending_len += (size_t) rd;
	}

	std::memcpy(&packet, pending, sizeof(packet));
	pending_len = 0;
	last.x = packet.x;
	last.y = packet.y;
	last.scroll = packet.scroll;
	last.button1 = (packet.flags & (1 << 0));
	last.button2 = (packet.flags & (1 << 1));
	last.button3 = (packet.flags & (1 << 2));
	return last;
}
