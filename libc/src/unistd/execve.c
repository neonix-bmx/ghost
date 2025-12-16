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

#include <ghost/tasks.h>

static int execve_status_to_errno(g_spawn_status status)
{
	switch(status)
	{
		case G_SPAWN_STATUS_MEMORY_ERROR:
			return ENOMEM;
		case G_SPAWN_STATUS_FORMAT_ERROR:
			return ENOEXEC;
		case G_SPAWN_STATUS_DEPENDENCY_ERROR:
			return ENOENT;
		case G_SPAWN_STATUS_TASKING_ERROR:
			return EPERM;
		case G_SPAWN_STATUS_IO_ERROR:
		default:
			return EIO;
	}
}

int execve(const char* path, char* const argv[], char* const envp[])
{
	g_spawn_status status = g_execve(path, (const char* const*) argv, (const char* const*) envp);

	// On success, the syscall never returns and this code is not reached.
	if(status == G_SPAWN_STATUS_SUCCESSFUL)
		return 0;

	errno = execve_status_to_errno(status);
	return -1;
}

int execv(const char* path, char* const argv[])
{
	return execve(path, argv, environ);
}
