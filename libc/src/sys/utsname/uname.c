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

#include "sys/utsname.h"
#include "string.h"
#include "errno.h"

static void uname_copy(char* dest, const char* src)
{
	size_t len = strlen(src);
	if(len >= UTSNAME_LENGTH)
		len = UTSNAME_LENGTH - 1;
	memcpy(dest, src, len);
	dest[len] = 0;
}

int uname(struct utsname* buf)
{
	if(!buf)
	{
		errno = EFAULT;
		return -1;
	}

	uname_copy(buf->sysname, "Heartix");
	uname_copy(buf->nodename, "ghost");
	uname_copy(buf->release, "0.1.0");
	uname_copy(buf->version, "Heartix 0.1.0");
	uname_copy(buf->machine, "x86_64");

	return 0;
}
