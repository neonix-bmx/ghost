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

#include "unistd.h"
#include "errno.h"
#include "fcntl.h"
#include "ghost/filesystem.h"
#include "ghost/tasks.h"
#include <stdarg.h>


static int fcntl_check_fd(int fildes)
{
	g_fs_stat_data st;
	g_fs_stat_status status = g_fs_fstat(fildes, &st);
	if(status != G_FS_STAT_SUCCESS)
	{
		errno = EBADF;
		return -1;
	}
	return 0;
}

/**
 * Minimal fcntl implementation with basic fd/flag handling.
 */
int fcntl(int fildes, int cmd, ...) {
	switch(cmd)
	{
		case F_DUPFD:
		{
			va_list ap;
			va_start(ap, cmd);
			int minfd = va_arg(ap, int);
			va_end(ap);
			if(minfd < 0)
			{
				errno = EINVAL;
				return -1;
			}
			g_pid pid = g_get_pid();
			g_fd newfd = g_clone_fd_ts(fildes, pid, minfd, pid, NULL);
			if(newfd == G_FD_NONE)
			{
				errno = EBADF;
				return -1;
			}
			return newfd;
		}
		case F_GETFD:
			if(fcntl_check_fd(fildes) < 0)
				return -1;
			return 0;
		case F_SETFD:
		{
			va_list ap;
			va_start(ap, cmd);
			(void) va_arg(ap, int);
			va_end(ap);
			if(fcntl_check_fd(fildes) < 0)
				return -1;
			return 0;
		}
		case F_GETFL:
			if(fcntl_check_fd(fildes) < 0)
				return -1;
			return 0;
		case F_SETFL:
		{
			va_list ap;
			va_start(ap, cmd);
			(void) va_arg(ap, int);
			va_end(ap);
			if(fcntl_check_fd(fildes) < 0)
				return -1;
			return 0;
		}
		default:
			errno = ENOSYS;
			return -1;
	}
}
