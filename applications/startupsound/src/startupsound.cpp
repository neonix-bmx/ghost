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
#include <vector>
#include <cstdint>
#include <cstring>

#include <libac97/ac97.hpp>

namespace
{
constexpr const char* LOGIN_SOUND_PATH = "/system/sounds/login.wav";

struct riff_header
{
	char riffId[4];
	uint32_t riffSize;
	char waveId[4];
} __attribute__((packed));

struct chunk_header
{
	char id[4];
	uint32_t size;
} __attribute__((packed));

struct fmt_chunk
{
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
} __attribute__((packed));

bool readFully(g_fd fd, void* buffer, size_t bytes)
{
	uint8_t* dst = reinterpret_cast<uint8_t*>(buffer);
	size_t offset = 0;
	while(offset < bytes)
	{
		int32_t read = g_read(fd, dst + offset, bytes - offset);
		if(read <= 0)
			return false;
		offset += read;
	}
	return true;
}

bool skipBytes(g_fd fd, size_t bytes)
{
	uint8_t temp[256];
	size_t remaining = bytes;
	while(remaining > 0)
	{
		size_t chunk = remaining < sizeof(temp) ? remaining : sizeof(temp);
		int32_t read = g_read(fd, temp, chunk);
		if(read <= 0)
			return false;
		remaining -= read;
	}
	return true;
}

bool skipPadding(g_fd fd, uint32_t chunkSize)
{
	if(chunkSize & 1)
	{
		uint8_t pad;
		return readFully(fd, &pad, 1);
	}
	return true;
}

bool loadLoginSound(std::vector<uint8_t>& data)
{
	g_fd fd = g_open_f(LOGIN_SOUND_PATH, G_FILE_FLAG_MODE_READ);
	if(fd == G_FD_NONE)
	{
		klog("startupsound: missing %s", LOGIN_SOUND_PATH);
		return false;
	}

	riff_header riff{};
	if(!readFully(fd, &riff, sizeof(riff)))
	{
		klog("startupsound: failed to read RIFF header");
		g_close(fd);
		return false;
	}

	if(std::strncmp(riff.riffId, "RIFF", 4) != 0 || std::strncmp(riff.waveId, "WAVE", 4) != 0)
	{
		klog("startupsound: unsupported wav format");
		g_close(fd);
		return false;
	}

	fmt_chunk fmt{};
	bool haveFmt = false;
	bool haveData = false;
	klog("startupsound: parsing chunks in %s", LOGIN_SOUND_PATH);
	std::vector<uint8_t> pcm;

	while(true)
	{
		chunk_header chunk{};
		if(!readFully(fd, &chunk, sizeof(chunk)))
		{
			klog("startupsound: unexpected EOF while parsing chunks");
			break;
		}
		klog("startupsound: saw chunk %.4s (%u bytes)", chunk.id, chunk.size);

		if(std::strncmp(chunk.id, "fmt ", 4) == 0)
		{
			if(chunk.size < sizeof(fmt))
			{
				klog("startupsound: fmt chunk too small");
				break;
			}
			if(!readFully(fd, &fmt, sizeof(fmt)))
				break;
			if(chunk.size > sizeof(fmt) && !skipBytes(fd, chunk.size - sizeof(fmt)))
				break;
			if(!skipPadding(fd, chunk.size))
				break;
			haveFmt = true;

			if(fmt.audioFormat != 1 || fmt.numChannels != 2 || fmt.bitsPerSample != 16 ||
			   fmt.sampleRate != AC97_DEFAULT_SAMPLE_RATE)
			{
				klog("startupsound: unsupported pcm parameters");
				break;
			}
		}
		else if(std::strncmp(chunk.id, "data", 4) == 0)
		{
			pcm.resize(chunk.size);
			if(!readFully(fd, pcm.data(), chunk.size))
			{
				klog("startupsound: failed to read data chunk");
				break;
			}
			if(!skipPadding(fd, chunk.size))
				break;
			haveData = true;
			// data chunk typically last; break to avoid reading padding
			break;
		}
		else
		{
			if(!skipBytes(fd, chunk.size))
				break;
			if(!skipPadding(fd, chunk.size))
				break;
		}
	}

	g_close(fd);

	if(!haveFmt || !haveData)
		return false;

		klog("startupsound: loaded %zu bytes of PCM data", pcm.size());
	data = std::move(pcm);
	return true;
}

} // namespace

int main()
{
	g_ac97_channel channel;
	if(!ac97OpenChannel(&channel))
	{
		klog("startupsound: audio driver unavailable");
		return -1;
	}

	std::vector<uint8_t> pcmData;
	if(!loadLoginSound(pcmData))
		return -1;

	const uint8_t* data = pcmData.data();
	size_t totalBytes = pcmData.size();
	size_t offset = 0;

	klog("startupsound: streaming %zu bytes to PCM pipe %d", totalBytes, channel.pcmPipe);

	size_t lastReport = 0;
	while(offset < totalBytes)
	{
		int32_t wrote = g_write(channel.pcmPipe, data + offset, totalBytes - offset);
		if(wrote <= 0)
		{
			g_sleep(5);
			continue;
		}
		offset += wrote;

		if(offset - lastReport >= 32768)
		{
			klog("startupsound: wrote %zu/%zu bytes", offset, totalBytes);
			lastReport = offset;
		}
	}

	klog("startupsound: playback finished (%zu bytes)", totalBytes);
	g_close(channel.pcmPipe);
	klog("startupsound: closed PCM pipe");
	return 0;
}
