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

#include "sys/sysinfo.h"
#include "errno.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"

#include <ghost/tasks.h>

static long parse_kb_value(const char* buf, const char* key)
{
	const char* match = strstr(buf, key);
	if(!match)
		return -1;
	match += strlen(key);
	while(*match == ' ' || *match == '\t')
		++match;
	long value = 0;
	while(*match >= '0' && *match <= '9')
	{
		value = value * 10 + (*match - '0');
		++match;
	}
	return value;
}

int sysinfo(struct sysinfo* info)
{
	if(!info)
	{
		errno = EINVAL;
		return -1;
	}

	memset(info, 0, sizeof(*info));
	info->mem_unit = 1;
	info->uptime = (long) (g_millis() / 1000);

	int fd = open("/proc/meminfo", O_RDONLY);
	if(fd >= 0)
	{
		char buf[512];
		ssize_t rd = read(fd, buf, sizeof(buf) - 1);
		close(fd);
		if(rd > 0)
		{
			buf[rd] = 0;
			long total = parse_kb_value(buf, "MemTotal:");
			long free = parse_kb_value(buf, "MemFree:");
			long buffers = parse_kb_value(buf, "Buffers:");
			if(total > 0)
				info->totalram = (unsigned long) total * 1024;
			if(free > 0)
				info->freeram = (unsigned long) free * 1024;
			if(buffers > 0)
				info->bufferram = (unsigned long) buffers * 1024;
		}
	}

	return 0;
}
