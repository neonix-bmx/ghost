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

#ifndef __KERNEL_FILESYSTEM_PROCFS_DELEGATE__
#define __KERNEL_FILESYSTEM_PROCFS_DELEGATE__

#include "kernel/filesystem/filesystem.hpp"

g_fs_open_status filesystemProcfsDelegateOpen(g_fs_node* node, g_file_flag_mode flags);
g_fs_phys_id filesystemProcfsRootId();
g_fs_open_status filesystemProcfsDelegateDiscover(g_fs_node* parent, const char* name, g_fs_node** outNode);
g_fs_read_status filesystemProcfsDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length,
                                              int64_t* outRead);
g_fs_length_status filesystemProcfsDelegateGetLength(g_fs_node* node, uint64_t* outLength);
g_fs_close_status filesystemProcfsDelegateClose(g_fs_node* node, g_file_flag_mode openFlags);
g_fs_directory_refresh_status filesystemProcfsDelegateRefreshDir(g_fs_node* node);

#endif
