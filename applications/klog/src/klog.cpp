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
#include <cstring>

int main()
{
	g_fd logPipe = g_open_log_pipe();
	if(logPipe == G_FD_NONE)
	{
		printf("klog: failed to open kernel log pipe\n");
		return -1;
	}

	printf("klog: streaming kernel log (Ctrl+C to exit)\n");

	constexpr size_t BUFFER_SIZE = 256;
	char buffer[BUFFER_SIZE];

	while(true)
	{
		int32_t read = g_read(logPipe, buffer, BUFFER_SIZE);
		if(read > 0)
		{
			fwrite(buffer, 1, read, stdout);
			fflush(stdout);
		}
		else
		{
			g_sleep(50);
		}
	}

	return 0;
}
