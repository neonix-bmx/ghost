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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <libeth/ethdriver.hpp>

namespace
{

constexpr uint32_t DEFAULT_IP = (10 << 24) | (0 << 16) | (2 << 8) | 15; // 10.0.2.15

bool parseIp(const char* text, uint32_t* out)
{
	if(!text || !out)
		return false;

	uint32_t parts[4] = {0};
	const char* cursor = text;
	for(int i = 0; i < 4; ++i)
	{
		char* end = nullptr;
		long value = std::strtol(cursor, &end, 10);
		if(end == cursor || value < 0 || value > 255)
			return false;
		parts[i] = static_cast<uint32_t>(value);
		if(i < 3)
		{
			if(*end != '.')
				return false;
			cursor = end + 1;
		}
		else
		{
			cursor = end;
		}
	}
	if(*cursor != '\0')
		return false;

	*out = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
	return true;
}

void formatIp(uint32_t ip, char* out, size_t len)
{
	::snprintf(out, len, "%u.%u.%u.%u",
	           (ip >> 24) & 0xFF,
	           (ip >> 16) & 0xFF,
	           (ip >> 8) & 0xFF,
	           ip & 0xFF);
}

void printMac(const uint8_t mac[6])
{
	::printf("%02x:%02x:%02x:%02x:%02x:%02x",
	         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

} // namespace

int main(int argc, char** argv)
{
	uint32_t ip = DEFAULT_IP;
	bool customIp = false;

	if(argc >= 2)
	{
		if(!parseIp(argv[1], &ip))
		{
			::printf("Usage: ipconfig [ipv4-address]\n");
			return -1;
		}
		customIp = true;
	}

	g_eth_channel channel;
	if(!ethDriverInitialize(&channel, g_get_tid()))
	{
		::printf("Failed to reach ethernet driver\n");
		return -1;
	}

	::printf("Interface  : eth0\n");
	::printf("MAC Address: ");
	printMac(channel.mac);
	::printf("\n");
	if(!channel.linkUp)
	{
		::printf("Warning    : link down (no carrier)\n");
	}

	char ipText[32];
	formatIp(ip, ipText, sizeof(ipText));
	if(customIp)
	{
		::printf("IPv4 (arg) : %s\n", ipText);
	}
	else
	{
		::printf("IPv4 (default NAT): %s\n", ipText);
		::printf("         (pass ipconfig <address> to display a custom assignment)\n");
	}

	return 0;
}
