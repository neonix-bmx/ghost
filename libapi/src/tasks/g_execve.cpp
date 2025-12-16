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

#include "ghost/syscall.h"
#include "ghost/tasks.h"
#include "ghost/tasks/callstructs.h"

#include <stdlib.h>
#include <string.h>

static char* g_execve_pack_arguments(const char* const argv[])
{
	if(!argv)
	{
		char* empty = (char*) malloc(1);
		if(empty)
			empty[0] = 0;
		return empty;
	}

	size_t totalLength = 0;
	size_t argc = 0;
	while(argv[argc])
	{
		totalLength += strlen(argv[argc]);
		if(argv[argc + 1])
			totalLength += 1; // separator
		argc++;
	}

	char* packed = (char*) malloc(totalLength + 1);
	if(!packed)
		return nullptr;

	char* out = packed;
	for(size_t i = 0; i < argc; ++i)
	{
		size_t len = strlen(argv[i]);
		memcpy(out, argv[i], len);
		out += len;
		if(i + 1 < argc)
			*out++ = G_CLIARGS_SEPARATOR;
	}
	*out = 0;
	return packed;
}

g_spawn_status g_execve(const char* path, const char* const argv[], const char* const envp[])
{
	(void) envp;

	char* packedArgs = g_execve_pack_arguments(argv);
	if(!packedArgs)
		return G_SPAWN_STATUS_MEMORY_ERROR;

	g_syscall_execve data;
	data.path = path;
	data.args = packedArgs;

	g_syscall(G_SYSCALL_EXECVE, (g_address) &data);

	free(packedArgs);
	return data.status;
}
