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

int chdir(const char* path)
{
	g_set_working_directory_status status = g_set_working_directory(path);
	if(status == G_SET_WORKING_DIRECTORY_SUCCESSFUL)
		return 0;

	switch(status)
	{
		case G_SET_WORKING_DIRECTORY_NOT_A_FOLDER:
			errno = ENOTDIR;
			break;
		case G_SET_WORKING_DIRECTORY_NOT_FOUND:
			errno = ENOENT;
			break;
		default:
			errno = EIO;
			break;
	}
	return -1;
}
