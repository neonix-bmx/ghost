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

#ifndef __GHOST_LIBC_TERMIOS__
#define __GHOST_LIBC_TERMIOS__

#include "ghost/common.h"
#include "sys/types.h"

__BEGIN_C

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int speed_t;

#define NCCS 32

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
	speed_t c_ispeed;
	speed_t c_ospeed;
};

// c_cc indices
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11

// input flags
#define IGNBRK 0x0001
#define BRKINT 0x0002
#define IGNPAR 0x0004
#define PARMRK 0x0008
#define INPCK  0x0010
#define ISTRIP 0x0020
#define INLCR  0x0040
#define IGNCR  0x0080
#define ICRNL  0x0100
#define IXON   0x0400
#define IXOFF  0x1000
#define IXANY  0x0800

// output flags
#define OPOST  0x0001
#define ONLCR  0x0002

// control flags
#define CSIZE  0x0030
#define CS5    0x0000
#define CS6    0x0010
#define CS7    0x0020
#define CS8    0x0030
#define CSTOPB 0x0040
#define CREAD  0x0080
#define PARENB 0x0100
#define PARODD 0x0200
#define HUPCL  0x0400
#define CLOCAL 0x0800

// local flags
#define ISIG   0x0001
#define ICANON 0x0002
#define ECHO   0x0008
#define ECHOE  0x0010
#define ECHOK  0x0020
#define ECHONL 0x0040
#define NOFLSH 0x0080
#define IEXTEN 0x8000

// tcsetattr options
#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

// tcflush queue selectors
#define TCIFLUSH  0
#define TCOFLUSH  1
#define TCIOFLUSH 2

// tcflow actions
#define TCOOFF 0
#define TCOON  1
#define TCIOFF 2
#define TCION  3

// baud rates
#define B0     0
#define B50    50
#define B75    75
#define B110   110
#define B134   134
#define B150   150
#define B200   200
#define B300   300
#define B600   600
#define B1200  1200
#define B1800  1800
#define B2400  2400
#define B4800  4800
#define B9600  9600
#define B19200 19200
#define B38400 38400

int tcgetattr(int fd, struct termios* termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios* termios_p);
void cfmakeraw(struct termios* termios_p);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
int tcdrain(int fd);
int tcsendbreak(int fd, int duration);
speed_t cfgetispeed(const struct termios* termios_p);
speed_t cfgetospeed(const struct termios* termios_p);
int cfsetispeed(struct termios* termios_p, speed_t speed);
int cfsetospeed(struct termios* termios_p, speed_t speed);

__END_C

#endif
