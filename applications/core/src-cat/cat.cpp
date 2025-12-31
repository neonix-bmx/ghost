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

static int copy_stream(FILE* in, FILE* out)
{
	char buf[4096];
	for(;;)
	{
		size_t read = fread(buf, 1, sizeof(buf), in);
		if(read > 0)
		{
			if(fwrite(buf, 1, read, out) != read)
				return -1;
		}
		if(read < sizeof(buf))
		{
			if(ferror(in))
				return -1;
			break;
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		if(copy_stream(stdin, stdout) != 0)
		{
			perror("cat");
			return 1;
		}
		return 0;
	}

	for(int i = 1; i < argc; ++i)
	{
		const char* path = argv[i];
		FILE* in = fopen(path, "rb");
		if(!in)
		{
			fprintf(stderr, "cat: %s: %s\n", path, strerror(errno));
			return 1;
		}
		if(copy_stream(in, stdout) != 0)
		{
			fprintf(stderr, "cat: %s: %s\n", path, strerror(errno));
			fclose(in);
			return 1;
		}
		fclose(in);
	}
	return 0;
}
