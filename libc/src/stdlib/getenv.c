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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "unistd.h"

static int env_key_match(const char* entry, const char* key, size_t keylen)
{
	return (strncmp(entry, key, keylen) == 0 && entry[keylen] == '=');
}

/**
 *
 */
char* getenv(const char* key)
{
	if(!key || key[0] == '\0')
		return 0;

	if(!environ)
	{
		if(strcmp(key, "TERM") == 0)
			return (char*) "vt100";
		return 0;
	}

	size_t keylen = strlen(key);
	for(int i = 0; environ[i]; ++i)
	{
		if(env_key_match(environ[i], key, keylen))
			return environ[i] + keylen + 1;
	}

	if(strcmp(key, "TERM") == 0)
		return (char*) "vt100";

	return 0;
}
