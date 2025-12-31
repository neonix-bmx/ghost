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

#ifndef __GHOST_LIBC_GETOPT__
#define __GHOST_LIBC_GETOPT__

#include "ghost/common.h"

__BEGIN_C

struct option {
	const char* name;
	int has_arg;
	int* flag;
	int val;
};

#define no_argument 0
#define required_argument 1
#define optional_argument 2

extern char* optarg;
extern int optind;
extern int opterr;
extern int optopt;

int getopt_long(int argc, char* const argv[], const char* optstring,
	const struct option* longopts, int* longindex);
int getopt_long_only(int argc, char* const argv[], const char* optstring,
	const struct option* longopts, int* longindex);

__END_C

#endif
