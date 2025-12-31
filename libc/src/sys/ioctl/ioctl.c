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

#include "sys/ioctl.h"
#include "errno.h"
#include <stdarg.h>

static void ioctl_default_winsize(struct winsize* ws)
{
	ws->ws_col = 80;
	ws->ws_row = 25;
	ws->ws_xpixel = 0;
	ws->ws_ypixel = 0;
}

int ioctl(int fd, unsigned long request, ...)
{
	(void) fd;

	va_list ap;
	va_start(ap, request);

	switch(request)
	{
		case TIOCGWINSZ:
		{
			struct winsize* ws = va_arg(ap, struct winsize*);
			if(!ws)
			{
				va_end(ap);
				errno = EFAULT;
				return -1;
			}
			ioctl_default_winsize(ws);
			va_end(ap);
			return 0;
		}
		case TIOCSWINSZ:
			va_end(ap);
			return 0;
		default:
			break;
	}

	va_end(ap);
	errno = ENOTTY;
	return -1;
}
