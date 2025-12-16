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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <libeth/ethdriver.hpp>

namespace
{

#define PING_LOG(fmt, ...) klog("ping: " fmt, ##__VA_ARGS__)

constexpr uint16_t ETHERTYPE_ARP = 0x0806;
constexpr uint16_t ETHERTYPE_IPV4 = 0x0800;

constexpr uint16_t ARP_HTYPE_ETHERNET = 1;
constexpr uint16_t ARP_PTYPE_IPV4 = 0x0800;
constexpr uint8_t ARP_HLEN_ETHERNET = 6;
constexpr uint8_t ARP_PLEN_IPV4 = 4;
constexpr uint16_t ARP_OPER_REQUEST = 1;
constexpr uint16_t ARP_OPER_REPLY = 2;

constexpr uint8_t IP_VERSION = 4;
constexpr uint8_t IP_IHL_WORDS = 5;
constexpr uint8_t IP_TTL_DEFAULT = 64;
constexpr uint8_t IP_PROTOCOL_ICMP = 1;

constexpr uint8_t ICMP_TYPE_ECHO_REQUEST = 8;
constexpr uint8_t ICMP_TYPE_ECHO_REPLY = 0;

struct ethernet_header
{
	uint8_t dest[6];
	uint8_t src[6];
	uint16_t ethertype;
} __attribute__((packed));

struct arp_packet
{
	uint16_t htype;
	uint16_t ptype;
	uint8_t hlen;
	uint8_t plen;
	uint16_t oper;
	uint8_t sha[6];
	uint32_t spa;
	uint8_t tha[6];
	uint32_t tpa;
} __attribute__((packed));

struct ipv4_header
{
	uint8_t versionAndHeaderLength;
	uint8_t dscpEcn;
	uint16_t totalLength;
	uint16_t identification;
	uint16_t flagsAndFragment;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t headerChecksum;
	uint32_t source;
	uint32_t dest;
} __attribute__((packed));

struct icmp_echo
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identifier;
	uint16_t sequence;
	uint8_t payload[32];
} __attribute__((packed));

g_eth_channel g_channel;
uint32_t g_localIp = 0;
uint32_t g_targetIp = 0;

inline uint16_t hostToNetwork16(uint16_t value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap16(value);
#else
	return value;
#endif
}

inline uint32_t hostToNetwork32(uint32_t value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap32(value);
#else
	return value;
#endif
}

inline uint16_t networkToHost16(uint16_t value)
{
	return hostToNetwork16(value);
}

inline uint32_t networkToHost32(uint32_t value)
{
	return hostToNetwork32(value);
}

void formatIp(uint32_t ip, char* out, size_t len)
{
	::snprintf(out, len, "%u.%u.%u.%u",
	              (ip >> 24) & 0xFF,
	              (ip >> 16) & 0xFF,
	              (ip >> 8) & 0xFF,
	              ip & 0xFF);
}

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

void printMac(const uint8_t mac[6])
{
	::printf("%02x:%02x:%02x:%02x:%02x:%02x",
	            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

uint16_t checksum16(const void* data, size_t length)
{
	uint32_t sum = 0;
	const uint16_t* words = reinterpret_cast<const uint16_t*>(data);
	while(length > 1)
	{
		sum += *words++;
		length -= 2;
	}
	if(length)
	{
		sum += static_cast<uint16_t>(*reinterpret_cast<const uint8_t*>(words) << 8);
	}
	while(sum >> 16)
	{
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	return static_cast<uint16_t>(~sum);
}

bool sendFrame(const void* payload, size_t len)
{
	if(len > G_ETH_FRAME_DATA_SIZE)
		return false;

	g_eth_frame frame{};
	frame.length = static_cast<uint16_t>(len);
	std::memcpy(frame.data, payload, len);
	auto wrote = g_write(g_channel.txPipe, &frame, sizeof(frame));
	if(wrote != sizeof(frame))
	{
		PING_LOG("failed to transmit frame (%d bytes written)", (int) wrote);
		return false;
	}
	return true;
}

bool receiveFrame(g_eth_frame& frame)
{
	int32_t rd = g_read(g_channel.rxPipe, &frame, sizeof(frame));
	return rd == sizeof(frame);
}

void sendArpReply(const uint8_t targetMac[6], uint32_t targetIp)
{
	uint8_t buffer[sizeof(ethernet_header) + sizeof(arp_packet)];
	auto eth = reinterpret_cast<ethernet_header*>(buffer);
	auto arp = reinterpret_cast<arp_packet*>(buffer + sizeof(ethernet_header));

	std::memcpy(eth->dest, targetMac, 6);
	std::memcpy(eth->src, g_channel.mac, 6);
	eth->ethertype = hostToNetwork16(ETHERTYPE_ARP);

	arp->htype = hostToNetwork16(ARP_HTYPE_ETHERNET);
	arp->ptype = hostToNetwork16(ARP_PTYPE_IPV4);
	arp->hlen = ARP_HLEN_ETHERNET;
	arp->plen = ARP_PLEN_IPV4;
	arp->oper = hostToNetwork16(ARP_OPER_REPLY);
	std::memcpy(arp->sha, g_channel.mac, 6);
	arp->spa = hostToNetwork32(g_localIp);
	std::memcpy(arp->tha, targetMac, 6);
	arp->tpa = hostToNetwork32(targetIp);

	sendFrame(buffer, sizeof(buffer));
}

void sendArpRequest()
{
	uint8_t buffer[sizeof(ethernet_header) + sizeof(arp_packet)];
	auto eth = reinterpret_cast<ethernet_header*>(buffer);
	auto arp = reinterpret_cast<arp_packet*>(buffer + sizeof(ethernet_header));

	std::memset(eth->dest, 0xFF, 6);
	std::memcpy(eth->src, g_channel.mac, 6);
	eth->ethertype = hostToNetwork16(ETHERTYPE_ARP);

	arp->htype = hostToNetwork16(ARP_HTYPE_ETHERNET);
	arp->ptype = hostToNetwork16(ARP_PTYPE_IPV4);
	arp->hlen = ARP_HLEN_ETHERNET;
	arp->plen = ARP_PLEN_IPV4;
	arp->oper = hostToNetwork16(ARP_OPER_REQUEST);
	std::memcpy(arp->sha, g_channel.mac, 6);
	arp->spa = hostToNetwork32(g_localIp);
	std::memset(arp->tha, 0x00, 6);
	arp->tpa = hostToNetwork32(g_targetIp);

	sendFrame(buffer, sizeof(buffer));
}

bool handleArpRequest(const g_eth_frame& frame)
{
	if(frame.length < sizeof(ethernet_header) + sizeof(arp_packet))
		return false;

	auto eth = reinterpret_cast<const ethernet_header*>(frame.data);
	if(networkToHost16(eth->ethertype) != ETHERTYPE_ARP)
		return false;

	auto arp = reinterpret_cast<const arp_packet*>(frame.data + sizeof(ethernet_header));
	if(networkToHost16(arp->oper) != ARP_OPER_REQUEST)
		return false;
	if(networkToHost32(arp->tpa) != g_localIp)
		return false;

	sendArpReply(arp->sha, networkToHost32(arp->spa));
	char requesterIp[32];
	formatIp(networkToHost32(arp->spa), requesterIp, sizeof(requesterIp));
	PING_LOG("responded to ARP request from %s", requesterIp);
	return true;
}

bool tryParseArpReply(const g_eth_frame& frame, uint8_t outMac[6])
{
	if(frame.length < sizeof(ethernet_header) + sizeof(arp_packet))
		return false;

	auto eth = reinterpret_cast<const ethernet_header*>(frame.data);
	if(networkToHost16(eth->ethertype) != ETHERTYPE_ARP)
		return false;

	auto arp = reinterpret_cast<const arp_packet*>(frame.data + sizeof(ethernet_header));
	if(networkToHost16(arp->oper) != ARP_OPER_REPLY)
		return false;
	if(networkToHost32(arp->spa) != g_targetIp)
		return false;
	if(networkToHost32(arp->tpa) != g_localIp)
		return false;

	std::memcpy(outMac, arp->sha, 6);
	char responderIp[32];
	formatIp(networkToHost32(arp->spa), responderIp, sizeof(responderIp));
	PING_LOG("received ARP reply from %s", responderIp);
	return true;
}

bool resolveTargetMac(uint8_t mac[6])
{
	char localIpText[32];
	char targetIpText[32];
	formatIp(g_localIp, localIpText, sizeof(localIpText));
	formatIp(g_targetIp, targetIpText, sizeof(targetIpText));

	::printf("Resolving target via ARP...\n");
	PING_LOG("resolving target via ARP local=%s target=%s", localIpText, targetIpText);
	sendArpRequest();
	PING_LOG("sent ARP request for %s", targetIpText);

	for(int attempt = 0; attempt < 400; ++attempt)
	{
		g_eth_frame frame{};
		if(receiveFrame(frame))
		{
			if(handleArpRequest(frame))
				continue;
			if(tryParseArpReply(frame, mac))
				return true;
		}
		else
		{
			g_sleep(10);
		}

		if(attempt % 100 == 0)
		{
			PING_LOG("retrying ARP request (attempt %d)", (attempt / 100) + 2);
			sendArpRequest();
		}
	}
	PING_LOG("ARP resolution for %s timed out", targetIpText);
	return false;
}

bool sendIcmpEcho(uint16_t sequence, const uint8_t targetMac[6])
{
	uint8_t buffer[sizeof(ethernet_header) + sizeof(ipv4_header) + sizeof(icmp_echo)];
	std::memset(buffer, 0, sizeof(buffer));

	auto eth = reinterpret_cast<ethernet_header*>(buffer);
	std::memcpy(eth->dest, targetMac, 6);
	std::memcpy(eth->src, g_channel.mac, 6);
	eth->ethertype = hostToNetwork16(ETHERTYPE_IPV4);

	auto ip = reinterpret_cast<ipv4_header*>(buffer + sizeof(ethernet_header));
	ip->versionAndHeaderLength = (IP_VERSION << 4) | IP_IHL_WORDS;
	ip->dscpEcn = 0;
	ip->totalLength = hostToNetwork16(sizeof(ipv4_header) + sizeof(icmp_echo));
	ip->identification = 0;
	ip->flagsAndFragment = 0;
	ip->ttl = IP_TTL_DEFAULT;
	ip->protocol = IP_PROTOCOL_ICMP;
	ip->headerChecksum = 0;
	ip->source = hostToNetwork32(g_localIp);
	ip->dest = hostToNetwork32(g_targetIp);
	ip->headerChecksum = checksum16(ip, sizeof(ipv4_header));

	auto icmp = reinterpret_cast<icmp_echo*>(buffer + sizeof(ethernet_header) + sizeof(ipv4_header));
	icmp->type = ICMP_TYPE_ECHO_REQUEST;
	icmp->code = 0;
	icmp->checksum = 0;
	icmp->identifier = hostToNetwork16(0x1337);
	icmp->sequence = hostToNetwork16(sequence);
	const char payload[] = "ghost ping utility";
	std::memcpy(icmp->payload, payload, sizeof(payload));
	icmp->checksum = checksum16(icmp, sizeof(icmp_echo));

	return sendFrame(buffer, sizeof(buffer));
}

bool tryHandleIcmpReply(const g_eth_frame& frame, uint16_t sequence)
{
	if(frame.length < sizeof(ethernet_header) + sizeof(ipv4_header) + sizeof(icmp_echo))
		return false;

	auto eth = reinterpret_cast<const ethernet_header*>(frame.data);
	if(networkToHost16(eth->ethertype) != ETHERTYPE_IPV4)
		return false;

	auto ip = reinterpret_cast<const ipv4_header*>(frame.data + sizeof(ethernet_header));
	if((ip->versionAndHeaderLength >> 4) != IP_VERSION)
		return false;
	if((ip->versionAndHeaderLength & 0x0F) < IP_IHL_WORDS)
		return false;
	if(ip->protocol != IP_PROTOCOL_ICMP)
		return false;
	if(networkToHost32(ip->source) != g_targetIp || networkToHost32(ip->dest) != g_localIp)
		return false;

	size_t ipHeaderLength = (ip->versionAndHeaderLength & 0x0F) * 4;
	if(frame.length < sizeof(ethernet_header) + ipHeaderLength + sizeof(icmp_echo))
		return false;

	auto icmp = reinterpret_cast<const icmp_echo*>(frame.data + sizeof(ethernet_header) + ipHeaderLength);
	if(icmp->type != ICMP_TYPE_ECHO_REPLY || icmp->code != 0)
		return false;
	if(networkToHost16(icmp->identifier) != 0x1337)
		return false;
	if(networkToHost16(icmp->sequence) != sequence)
		return false;

	return true;
}

bool awaitIcmpReply(uint16_t sequence)
{
	for(int attempt = 0; attempt < 500; ++attempt)
	{
		g_eth_frame frame{};
		if(receiveFrame(frame))
		{
			if(handleArpRequest(frame))
				continue;
			if(tryHandleIcmpReply(frame, sequence))
				return true;
		}
		else
		{
			g_sleep(10);
		}
	}
	return false;
}

} // namespace

int main(int argc, char** argv)
{
	if(argc < 3)
	{
		printf("Usage: ping <target-ip> <source-ip>\n");
		return -1;
	}

	if(!parseIp(argv[1], &g_targetIp) || !parseIp(argv[2], &g_localIp))
	{
		printf("Invalid IPv4 address\n");
		return -1;
	}

	constexpr uint32_t defaultMask = 0xFFFFFF00u;
	if((g_targetIp & defaultMask) != (g_localIp & defaultMask))
	{
		char targetText[32];
		char localText[32];
		formatIp(g_targetIp, targetText, sizeof(targetText));
		formatIp(g_localIp, localText, sizeof(localText));
		::printf("Target %s is not on the same /24 network as %s. Routing not implemented yet.\n",
		         targetText, localText);
		PING_LOG("rejecting target %s via %s due to subnet mismatch", targetText, localText);
		return -1;
	}

	if(!ethDriverInitialize(&g_channel, g_get_tid()))
	{
		printf("Failed to initialize ethernet driver\n");
		PING_LOG("failed to reach ethernet driver");
		return -1;
	}

	char ipBuf[32];
	formatIp(g_targetIp, ipBuf, sizeof(ipBuf));
	::printf("Pinging %s from ", ipBuf);
	formatIp(g_localIp, ipBuf, sizeof(ipBuf));
	::printf("%s\n", ipBuf);

	::printf("Interface MAC: ");
	printMac(g_channel.mac);
	::printf("\n");
	if(!g_channel.linkUp)
	{
		::printf("Warning: link is down; ping may time out\n");
	}

	uint8_t targetMac[6];
	if(!resolveTargetMac(targetMac))
	{
		::printf("Failed to resolve target MAC address\n");
		PING_LOG("failed to resolve target MAC");
		return -1;
	}

	::printf("Target MAC: ");
	printMac(targetMac);
	::printf("\n");

	uint16_t sequence = 1;
	if(!sendIcmpEcho(sequence, targetMac))
	{
		::printf("Failed to send ICMP echo\n");
		PING_LOG("failed to send ICMP echo, seq=%u", sequence);
		return -1;
	}

	if(awaitIcmpReply(sequence))
	{
		::printf("Received ICMP echo reply\n");
		PING_LOG("received ICMP echo reply seq=%u", sequence);
	}
	else
	{
		::printf("Request timed out\n");
		PING_LOG("icmp request timed out seq=%u", sequence);
		return -1;
	}

	return 0;
}
