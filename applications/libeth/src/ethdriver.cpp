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

#include "libeth/ethdriver.hpp"

#include <cstring>
#include <ghost.h>

bool ethDriverInitialize(g_eth_channel* outChannel, g_tid rxPartnerTask)
{
	if(!outChannel)
		return false;

	g_tid driverTid = g_task_await_by_name(G_ETH_DRIVER_NAME);
	if(driverTid == G_TID_NONE)
		return false;

	g_message_transaction transaction = g_get_message_tx_id();

	g_eth_initialize_request request{};
	request.header.command = G_ETH_COMMAND_INITIALIZE;
	request.rxPartnerTask = rxPartnerTask;
	g_send_message_t(driverTid, &request, sizeof(request), transaction);

	size_t bufLen = sizeof(g_message_header) + sizeof(g_eth_initialize_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, transaction) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		return false;

	auto response = (g_eth_initialize_response*) G_MESSAGE_CONTENT(buf);
	if(response->status != G_ETH_STATUS_SUCCESS)
		return false;

	outChannel->rxPipe = response->rxPipe;
	outChannel->txPipe = response->txPipe;
	std::memcpy(outChannel->mac, response->mac, sizeof(response->mac));
	outChannel->linkUp = response->linkUp != 0;
	return true;
}
