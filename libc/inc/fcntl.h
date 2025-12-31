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

#ifndef __GHOST_LIBC_FCNTL__
#define __GHOST_LIBC_FCNTL__

#include <ghost/filesystem.h>
#include <ghost/stdint.h>

__BEGIN_C

// mode type
typedef uint32_t mode_t;

// POSIX open flags
#define O_READ				G_FILE_FLAG_MODE_READ
#define O_WRITE				G_FILE_FLAG_MODE_WRITE
#define O_APPEND			G_FILE_FLAG_MODE_APPEND
#define O_CREAT 			G_FILE_FLAG_MODE_CREATE
#define O_TRUNC				G_FILE_FLAG_MODE_TRUNCATE
#define O_EXCL				G_FILE_FLAG_MODE_EXCLUSIVE
#define O_NONBLOCK			0x00000800
#define O_CLOEXEC			0x00080000

#define	O_RDONLY			G_FILE_FLAG_MODE_READ
#define O_WRONLY 			G_FILE_FLAG_MODE_WRITE
#define O_RDWR				(G_FILE_FLAG_MODE_READ | G_FILE_FLAG_MODE_WRITE)

// commands & flags for fcntl()
#define	F_DUPFD				0
#define F_GETFD				1
#define F_SETFD				2
#define F_GETFL				3
#define F_SETFL				4
#define F_GETLK				5
#define F_SETLK				6
#define F_SETLKW			7
// #define F_GETOWN
// #define F_SETOWN

#define FD_CLOEXEC			1

#ifndef AT_FDCWD
#define AT_FDCWD			(-100)
#endif

#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x0100
#endif

// POSIX
int open(const char* pathname, int flags, ...);

__END_C

#endif
