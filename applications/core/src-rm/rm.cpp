/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schluessel <lokoxe@gmail.com>                    *
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

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void print_usage()
{
	printf("usage: rm [-f] <path> [path...]\n");
}

int main(int argc, char** argv)
{
	bool force = false;
	int start = 1;

	if(argc < 2)
	{
		print_usage();
		return 1;
	}

	if(strcmp(argv[1], "-f") == 0)
	{
		force = true;
		start = 2;
	}

	if(start >= argc)
	{
		print_usage();
		return 1;
	}

	int result = 0;
	for(int i = start; i < argc; ++i)
	{
		const char* path = argv[i];
		if(remove(path) != 0 && !force)
		{
			fprintf(stderr, "rm: %s: %s\n", path, strerror(errno));
			result = 1;
		}
	}

	return result;
}
