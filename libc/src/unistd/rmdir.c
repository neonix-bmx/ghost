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
#include <ghost/filesystem.h>

int rmdir(const char* path)
{
	g_fs_rmdir_status status = g_fs_rmdir(path);
	if(status == G_FS_RMDIR_SUCCESSFUL)
		return 0;

	switch(status)
	{
		case G_FS_RMDIR_NOT_FOUND:
			errno = ENOENT;
			break;
		case G_FS_RMDIR_NOT_EMPTY:
			errno = ENOTEMPTY;
			break;
		case G_FS_RMDIR_NOT_A_DIRECTORY:
			errno = ENOTDIR;
			break;
		default:
			errno = EIO;
			break;
	}

	return -1;
}
