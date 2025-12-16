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

#include "libac97/ac97.hpp"

#include <ghost.h>

bool ac97OpenChannel(g_ac97_channel* channel)
{
	if(!channel)
		return false;

	g_tid driver = g_task_await_by_name(G_AC97_DRIVER_NAME);
	if(driver == G_TID_NONE)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ac97_open_request request{};
	request.header.command = G_AC97_COMMAND_OPEN_CHANNEL;
	request.clientTask = g_get_tid();

	g_send_message_t(driver, &request, sizeof(request), tx);

	size_t bufLen = sizeof(g_message_header) + sizeof(g_ac97_open_response);
	uint8_t buf[bufLen];

	if(g_receive_message_t(buf, bufLen, tx) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		return false;

	auto response = (g_ac97_open_response*) G_MESSAGE_CONTENT(buf);
	if(response->status != G_AC97_STATUS_SUCCESS)
		return false;

	channel->pcmPipe = response->pcmPipe;
	return true;
}

