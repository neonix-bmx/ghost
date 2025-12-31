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

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

static void print_usage()
{
	printf("usage: uname [-a] [-s] [-n] [-r] [-v] [-m]\n");
}

int main(int argc, char** argv)
{
	bool show_sysname = false;
	bool show_nodename = false;
	bool show_release = false;
	bool show_version = false;
	bool show_machine = false;
	bool any_flag = false;

	for(int i = 1; i < argc; ++i)
	{
		const char* arg = argv[i];
		if(arg[0] != '-')
		{
			print_usage();
			return 1;
		}
		if(strcmp(arg, "--all") == 0 || strcmp(arg, "-a") == 0)
		{
			show_sysname = true;
			show_nodename = true;
			show_release = true;
			show_version = true;
			show_machine = true;
			any_flag = true;
			continue;
		}

		for(size_t j = 1; arg[j]; ++j)
		{
			any_flag = true;
			switch(arg[j])
			{
				case 's':
					show_sysname = true;
					break;
				case 'n':
					show_nodename = true;
					break;
				case 'r':
					show_release = true;
					break;
				case 'v':
					show_version = true;
					break;
				case 'm':
					show_machine = true;
					break;
				default:
					print_usage();
					return 1;
			}
		}
	}

	if(!any_flag)
	{
		show_sysname = true;
	}

	struct utsname info;
	if(uname(&info) != 0)
	{
		perror("uname");
		return 1;
	}

	bool first = true;
	if(show_sysname)
	{
		printf("%s", info.sysname);
		first = false;
	}
	if(show_nodename)
	{
		printf("%s%s", first ? "" : " ", info.nodename);
		first = false;
	}
	if(show_release)
	{
		printf("%s%s", first ? "" : " ", info.release);
		first = false;
	}
	if(show_version)
	{
		printf("%s%s", first ? "" : " ", info.version);
		first = false;
	}
	if(show_machine)
	{
		printf("%s%s", first ? "" : " ", info.machine);
		first = false;
	}

	printf("\n");
	return 0;
}
