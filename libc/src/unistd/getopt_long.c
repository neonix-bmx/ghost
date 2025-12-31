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

#include "getopt.h"
#include "string.h"
#include "unistd.h"

static int getopt_long_internal(int argc, char* const argv[], const char* optstring,
	const struct option* longopts, int* longindex, int long_only) {
	if(optind >= argc || argv[optind] == 0) {
		return -1;
	}

	char* arg = argv[optind];
	if(arg[0] != '-' || arg[1] == '\0') {
		return -1;
	}

	if(strcmp(arg, "--") == 0) {
		++optind;
		return -1;
	}

	if(long_only && arg[1] != '-' && arg[2] == '\0') {
		if(optstring && strchr(optstring, arg[1]) != 0) {
			return getopt(argc, argv, optstring);
		}
	}

	if(arg[1] == '-' || long_only) {
		const char* name = (arg[1] == '-') ? arg + 2 : arg + 1;
		if(*name == '\0') {
			return getopt(argc, argv, optstring);
		}

		const char* name_end = strchr(name, '=');
		size_t name_len = name_end ? (size_t)(name_end - name) : strlen(name);

		int match_index = -1;
		if(longopts) {
			for(int i = 0; longopts[i].name; ++i) {
				if(strlen(longopts[i].name) == name_len &&
					strncmp(longopts[i].name, name, name_len) == 0) {
					match_index = i;
					break;
				}
			}
		}

		if(match_index >= 0) {
			const struct option* opt = &longopts[match_index];
			if(longindex) {
				*longindex = match_index;
			}

			int next = optind + 1;
			optarg = 0;
			if(opt->has_arg == required_argument) {
				if(name_end) {
					optarg = (char*)(name_end + 1);
				} else if(next < argc) {
					optarg = argv[next++];
				} else {
					optopt = opt->val;
					optind = next;
					return (optstring && optstring[0] == ':') ? ':' : '?';
				}
			} else if(opt->has_arg == optional_argument) {
				if(name_end) {
					optarg = (char*)(name_end + 1);
				}
			}

			optind = next;
			if(opt->flag) {
				*(opt->flag) = opt->val;
				return 0;
			}
			return opt->val;
		}

		if(arg[1] == '-') {
			optopt = 0;
			++optind;
			return '?';
		}
	}

	return getopt(argc, argv, optstring);
}

int getopt_long(int argc, char* const argv[], const char* optstring,
	const struct option* longopts, int* longindex) {
	return getopt_long_internal(argc, argv, optstring, longopts, longindex, 0);
}

int getopt_long_only(int argc, char* const argv[], const char* optstring,
	const struct option* longopts, int* longindex) {
	return getopt_long_internal(argc, argv, optstring, longopts, longindex, 1);
}
