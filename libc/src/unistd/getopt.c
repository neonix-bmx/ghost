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
#include "string.h"

char *optarg = 0;
int optind = 1;
int opterr = 1;
int optopt = 0;

static int optpos = 1;

int getopt(int argc, char * const argv[], const char *optstring)
{
	if(optind >= argc || !argv[optind])
		return -1;

	char *arg = argv[optind];
	if(optpos == 1)
	{
		if(arg[0] != '-' || arg[1] == '\0')
			return -1;
		if(strcmp(arg, "--") == 0)
		{
			++optind;
			return -1;
		}
	}

	char opt = arg[optpos];
	const char *spec = strchr(optstring, opt);
	if(!spec)
	{
		optopt = opt;
		++optpos;
		if(arg[optpos] == '\0')
		{
			optpos = 1;
			++optind;
		}
		return '?';
	}

	if(spec[1] == ':')
	{
		if(spec[2] == ':')
		{
			if(arg[optpos + 1] != '\0')
			{
				optarg = &arg[optpos + 1];
				++optind;
				optpos = 1;
				return opt;
			}

			optarg = 0;
			++optpos;
			if(arg[optpos] == '\0')
			{
				optpos = 1;
				++optind;
			}
			return opt;
		}

		if(arg[optpos + 1] != '\0')
		{
			optarg = &arg[optpos + 1];
			++optind;
			optpos = 1;
			return opt;
		}
		if(optind + 1 < argc)
		{
			optarg = argv[optind + 1];
			optind += 2;
			optpos = 1;
			return opt;
		}

		optopt = opt;
		optpos = 1;
		++optind;
		return (*optstring == ':') ? ':' : '?';
	}

	optarg = 0;
	++optpos;
	if(arg[optpos] == '\0')
	{
		optpos = 1;
		++optind;
	}
	return opt;
}
