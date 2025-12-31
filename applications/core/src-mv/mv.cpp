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
#include <unistd.h>

static int copy_file(const char* src, const char* dst)
{
	FILE* in = fopen(src, "rb");
	if(!in)
		return -1;
	FILE* out = fopen(dst, "wb");
	if(!out)
	{
		fclose(in);
		return -1;
	}

	char buf[8192];
	for(;;)
	{
		size_t read = fread(buf, 1, sizeof(buf), in);
		if(read > 0)
		{
			if(fwrite(buf, 1, read, out) != read)
			{
				fclose(in);
				fclose(out);
				return -1;
			}
		}
		if(read < sizeof(buf))
		{
			if(ferror(in))
			{
				fclose(in);
				fclose(out);
				return -1;
			}
			break;
		}
	}

	fclose(in);
	fclose(out);
	return 0;
}

static void print_usage()
{
	printf("usage: mv <source> <dest>\n");
}

int main(int argc, char** argv)
{
	if(argc != 3)
	{
		print_usage();
		return 1;
	}

	if(rename(argv[1], argv[2]) == 0)
		return 0;

	if(errno == EXDEV)
	{
		if(copy_file(argv[1], argv[2]) != 0)
		{
			fprintf(stderr, "mv: %s -> %s: %s\n", argv[1], argv[2], strerror(errno));
			return 1;
		}
		if(unlink(argv[1]) != 0)
		{
			fprintf(stderr, "mv: %s: %s\n", argv[1], strerror(errno));
			return 1;
		}
		return 0;
	}

	fprintf(stderr, "mv: %s -> %s: %s\n", argv[1], argv[2], strerror(errno));
	return 1;
}
