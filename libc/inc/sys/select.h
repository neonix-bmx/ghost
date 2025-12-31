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

#ifndef __GHOST_LIBC_SYS_SELECT__
#define __GHOST_LIBC_SYS_SELECT__

#include "ghost/common.h"
#include "sys/time.h"

__BEGIN_C

#define FD_SETSIZE 1024

typedef struct
{
	unsigned long fds_bits[(FD_SETSIZE + (8 * sizeof(unsigned long)) - 1) / (8 * sizeof(unsigned long))];
} fd_set;

#define FD_ZERO(set) do { \
	unsigned long* __bits = (set)->fds_bits; \
	for(unsigned long __i = 0; __i < sizeof((set)->fds_bits) / sizeof((set)->fds_bits[0]); ++__i) \
		__bits[__i] = 0; \
} while(0)

#define FD_SET(fd, set) ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] |= \
	(1UL << ((fd) % (8 * sizeof(unsigned long)))))

#define FD_CLR(fd, set) ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] &= \
	~(1UL << ((fd) % (8 * sizeof(unsigned long)))))

#define FD_ISSET(fd, set) (((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] & \
	(1UL << ((fd) % (8 * sizeof(unsigned long))))) != 0)

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);

__END_C

#endif
