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

#ifndef __LIBETH_ETHDRIVER_HPP__
#define __LIBETH_ETHDRIVER_HPP__

#include <cstdint>
#include <ghost/filesystem/types.h>
#include <ghost/messages/types.h>
#include <ghost/tasks/types.h>

#define G_ETH_DRIVER_NAME "ethdriver"
#define G_ETH_DEVICE_BASE "/dev/net/eth0"
#define G_ETH_DEVICE_RX G_ETH_DEVICE_BASE "/rx"
#define G_ETH_DEVICE_TX G_ETH_DEVICE_BASE "/tx"

constexpr size_t G_ETH_FRAME_DATA_SIZE = 1600;

typedef uint16_t g_eth_command;
#define G_ETH_COMMAND_INITIALIZE ((g_eth_command) 0)

typedef uint8_t g_eth_status;
#define G_ETH_STATUS_SUCCESS ((g_eth_status) 0)
#define G_ETH_STATUS_FAILURE ((g_eth_status) 1)

struct g_eth_frame
{
	uint16_t length;
	uint8_t data[G_ETH_FRAME_DATA_SIZE];
} __attribute__((packed));

struct g_eth_request_header
{
	g_eth_command command;
} __attribute__((packed));

struct g_eth_initialize_request
{
	g_eth_request_header header;
	g_tid rxPartnerTask;
} __attribute__((packed));

struct g_eth_initialize_response
{
	g_eth_status status;
	uint8_t mac[6];
	uint8_t linkUp;
} __attribute__((packed));

struct g_eth_channel
{
	g_fd rxPipe;
	g_fd txPipe;
	uint8_t mac[6];
	bool linkUp;
};

bool ethDriverInitialize(g_eth_channel* outChannel, g_tid rxPartnerTask = G_TID_NONE);

#endif
