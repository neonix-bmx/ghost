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

#include "termios.h"
#include "errno.h"
#include "string.h"

static int termios_is_tty(int fd)
{
	return (fd == 0 || fd == 1 || fd == 2);
}

static void termios_set_defaults(struct termios* t)
{
	memset(t, 0, sizeof(*t));
	t->c_iflag = ICRNL | IXON;
	t->c_oflag = OPOST | ONLCR;
	t->c_cflag = CREAD | CS8;
	t->c_lflag = ISIG | ICANON | ECHO | ECHOE;
	t->c_cc[VINTR] = 3;
	t->c_cc[VEOF] = 4;
	t->c_cc[VERASE] = 127;
	t->c_cc[VMIN] = 1;
	t->c_cc[VTIME] = 0;
	t->c_ispeed = B9600;
	t->c_ospeed = B9600;
}

int tcgetattr(int fd, struct termios* termios_p)
{
	if(!termios_p)
	{
		errno = EINVAL;
		return -1;
	}
	if(!termios_is_tty(fd))
	{
		errno = ENOTTY;
		return -1;
	}
	termios_set_defaults(termios_p);
	return 0;
}

int tcsetattr(int fd, int optional_actions, const struct termios* termios_p)
{
	if(!termios_p)
	{
		errno = EINVAL;
		return -1;
	}
	if(optional_actions != TCSANOW && optional_actions != TCSADRAIN && optional_actions != TCSAFLUSH)
	{
		errno = EINVAL;
		return -1;
	}
	if(!termios_is_tty(fd))
	{
		errno = ENOTTY;
		return -1;
	}
	return 0;
}

void cfmakeraw(struct termios* termios_p)
{
	if(!termios_p)
		return;

	termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	termios_p->c_oflag &= ~(OPOST);
	termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termios_p->c_cflag &= ~(CSIZE | PARENB);
	termios_p->c_cflag |= CS8;
	termios_p->c_cc[VMIN] = 1;
	termios_p->c_cc[VTIME] = 0;
}

int tcflush(int fd, int queue_selector)
{
	if(queue_selector != TCIFLUSH && queue_selector != TCOFLUSH && queue_selector != TCIOFLUSH)
	{
		errno = EINVAL;
		return -1;
	}
	if(!termios_is_tty(fd))
	{
		errno = ENOTTY;
		return -1;
	}
	return 0;
}

int tcflow(int fd, int action)
{
	if(action != TCOOFF && action != TCOON && action != TCIOFF && action != TCION)
	{
		errno = EINVAL;
		return -1;
	}
	if(!termios_is_tty(fd))
	{
		errno = ENOTTY;
		return -1;
	}
	return 0;
}

int tcdrain(int fd)
{
	if(!termios_is_tty(fd))
	{
		errno = ENOTTY;
		return -1;
	}
	return 0;
}

int tcsendbreak(int fd, int duration)
{
	(void) duration;
	if(!termios_is_tty(fd))
	{
		errno = ENOTTY;
		return -1;
	}
	return 0;
}

speed_t cfgetispeed(const struct termios* termios_p)
{
	if(!termios_p)
		return 0;
	return termios_p->c_ispeed;
}

speed_t cfgetospeed(const struct termios* termios_p)
{
	if(!termios_p)
		return 0;
	return termios_p->c_ospeed;
}

int cfsetispeed(struct termios* termios_p, speed_t speed)
{
	if(!termios_p)
	{
		errno = EINVAL;
		return -1;
	}
	termios_p->c_ispeed = speed;
	return 0;
}

int cfsetospeed(struct termios* termios_p, speed_t speed)
{
	if(!termios_p)
	{
		errno = EINVAL;
		return -1;
	}
	termios_p->c_ospeed = speed;
	return 0;
}
