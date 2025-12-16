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

#pragma once

#include <cstddef>
#include <cstdint>
#include <ghost/filesystem/types.h>
#include <ghost/messages/types.h>
#include <ghost/tasks/types.h>

#define G_AC97_DRIVER_NAME "ac97driver"

constexpr size_t AC97_BDL_ENTRY_COUNT = 32;
constexpr size_t AC97_DMA_BUFFER_SIZE = 4096;
constexpr uint32_t AC97_DEFAULT_SAMPLE_RATE = 48000;

// Mixer register offsets
constexpr uint16_t AC97_REG_RESET = 0x00;
constexpr uint16_t AC97_REG_MASTER_VOLUME = 0x02;
constexpr uint16_t AC97_REG_HEADPHONE_VOLUME = 0x04;
constexpr uint16_t AC97_REG_MONO_VOLUME = 0x06;
constexpr uint16_t AC97_REG_PCM_OUT_VOLUME = 0x18;
constexpr uint16_t AC97_REG_RECORD_SELECT = 0x1A;
constexpr uint16_t AC97_REG_RECORD_GAIN = 0x1C;
constexpr uint16_t AC97_REG_GENERAL_PURPOSE = 0x20;
constexpr uint16_t AC97_REG_3D_CONTROL = 0x22;
constexpr uint16_t AC97_REG_POWER_CONTROL = 0x26;
constexpr uint16_t AC97_REG_FRONT_DAC_RATE = 0x2C;

constexpr uint16_t AC97_POWER_EAPD = (1u << 15);

// Bus master register offsets
constexpr uint16_t AC97_BM_REG_GLOBAL_CONTROL = 0x2C;
constexpr uint16_t AC97_BM_REG_GLOBAL_STATUS = 0x30;
constexpr uint16_t AC97_BM_REG_CODEC_ACCESS_SEMA = 0x34;

constexpr uint16_t AC97_BM_REG_PO_BDBAR = 0x10;
constexpr uint16_t AC97_BM_REG_PO_CIV = 0x14;
constexpr uint16_t AC97_BM_REG_PO_LVI = 0x15;
constexpr uint16_t AC97_BM_REG_PO_SR = 0x16;
constexpr uint16_t AC97_BM_REG_PO_PICB = 0x18;
constexpr uint16_t AC97_BM_REG_PO_PIV = 0x1A;
constexpr uint16_t AC97_BM_REG_PO_CR = 0x1B;

constexpr uint32_t AC97_GLOB_CNT_COLD = (1u << 1);
constexpr uint32_t AC97_GLOB_CNT_WARM = (1u << 2);

constexpr uint16_t AC97_PO_SR_DCH = (1u << 0);
constexpr uint16_t AC97_PO_SR_CELV = (1u << 1);
constexpr uint16_t AC97_PO_SR_LVBCI = (1u << 2);
constexpr uint16_t AC97_PO_SR_BCIS = (1u << 3);
constexpr uint16_t AC97_PO_SR_FIFOE = (1u << 4);

constexpr uint8_t AC97_PO_CR_RUN = (1u << 0);
constexpr uint8_t AC97_PO_CR_RESET = (1u << 1);

constexpr uint16_t AC97_BDL_IOC = (1u << 15);

struct ac97_buffer_descriptor
{
	uint32_t buffer;
	uint16_t length;
	uint16_t control;
} __attribute__((packed));

typedef uint8_t g_ac97_command;
#define G_AC97_COMMAND_OPEN_CHANNEL ((g_ac97_command) 0)

typedef uint8_t g_ac97_status;
#define G_AC97_STATUS_SUCCESS ((g_ac97_status) 0)
#define G_AC97_STATUS_FAILURE ((g_ac97_status) 1)

struct g_ac97_request_header
{
	g_ac97_command command;
} __attribute__((packed));

struct g_ac97_open_request
{
	g_ac97_request_header header;
	g_tid clientTask;
} __attribute__((packed));

struct g_ac97_open_response
{
	g_ac97_status status;
	g_fd pcmPipe;
} __attribute__((packed));

struct g_ac97_channel
{
	g_fd pcmPipe = G_FD_NONE;
};

bool ac97OpenChannel(g_ac97_channel* channel);

