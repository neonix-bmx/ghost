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

#include "string.h"

static char* g_strtok_next = 0;

char* strtok(char* str, const char* delim) {
	char* token_end;

	if(delim == 0) {
		return 0;
	}

	if(str == 0) {
		str = g_strtok_next;
	}
	if(str == 0) {
		return 0;
	}

	str += strspn(str, delim);
	if(*str == '\0') {
		g_strtok_next = 0;
		return 0;
	}

	token_end = str + strcspn(str, delim);
	if(*token_end == '\0') {
		g_strtok_next = 0;
	} else {
		*token_end = '\0';
		g_strtok_next = token_end + 1;
	}

	return str;
}
