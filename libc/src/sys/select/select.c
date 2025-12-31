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

#include "sys/select.h"
#include "errno.h"
#include "ghost/tasks.h"

static int fdset_count(fd_set* set, int nfds)
{
	if(!set)
		return 0;
	int count = 0;
	for(int fd = 0; fd < nfds; ++fd)
	{
		if(FD_ISSET(fd, set))
			++count;
	}
	return count;
}

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout)
{
	if(nfds < 0)
	{
		errno = EINVAL;
		return -1;
	}

	int ready = 0;
	ready += fdset_count(readfds, nfds);
	ready += fdset_count(writefds, nfds);
	ready += fdset_count(exceptfds, nfds);

	if(timeout)
	{
		uint64_t millis = timeout->tv_sec * 1000;
		millis += (timeout->tv_usec + 999) / 1000;
		if(millis > 0)
			g_sleep(millis);
		return ready ? ready : 0;
	}

	return ready;
}
