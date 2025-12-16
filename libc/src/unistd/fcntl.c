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



/**
 * Minimal fcntl implementation: supports F_DUPFD.
 */
int fcntl(int fildes, int cmd, ...) {
	if(cmd == F_DUPFD)
	{
		va_list ap;
		va_start(ap, cmd);
		int minfd = va_arg(ap, int);
		va_end(ap);
		if(minfd < 0) return -1;
		g_pid pid = g_get_pid();
				g_fd newfd = g_clone_fd_ts(fildes, pid, minfd, pid, NULL);

		return (newfd == G_FD_NONE) ? -1 : newfd;
	}
	// Unsupported commands
	klog("warning: fcntl cmd %d not implemented", cmd);
	return -1;
}


