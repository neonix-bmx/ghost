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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"

#include "unistd.h"

static int env_key_match(const char* entry, const char* key, size_t keylen)
{
	return (strncmp(entry, key, keylen) == 0 && entry[keylen] == '=');
}

static int env_count()
{
	if(!environ)
		return 0;
	int count = 0;
	while(environ[count])
		++count;
	return count;
}

static int env_ensure()
{
	if(environ)
		return 0;
	environ = (char**) calloc(1, sizeof(char*));
	if(!environ)
	{
		errno = ENOMEM;
		return -1;
	}
	return 0;
}

/**
 *
 */
int setenv(const char *key, const char *val, int overwrite) {

	if(!key || key[0] == '\0' || strchr(key, '='))
	{
		errno = EINVAL;
		return -1;
	}

	if(!val)
		val = "";

	if(env_ensure() < 0)
		return -1;

	size_t keylen = strlen(key);
	int count = env_count();
	for(int i = 0; i < count; ++i)
	{
		if(env_key_match(environ[i], key, keylen))
		{
			if(!overwrite)
				return 0;

			size_t total = keylen + 1 + strlen(val) + 1;
			char* entry = (char*) malloc(total);
			if(!entry)
			{
				errno = ENOMEM;
				return -1;
			}
			snprintf(entry, total, "%s=%s", key, val);
			free(environ[i]);
			environ[i] = entry;
			return 0;
		}
	}

	char** resized = (char**) realloc(environ, sizeof(char*) * (count + 2));
	if(!resized)
	{
		errno = ENOMEM;
		return -1;
	}
	environ = resized;

	size_t total = keylen + 1 + strlen(val) + 1;
	char* entry = (char*) malloc(total);
	if(!entry)
	{
		errno = ENOMEM;
		return -1;
	}
	snprintf(entry, total, "%s=%s", key, val);

	environ[count] = entry;
	environ[count + 1] = 0;
	return 0;
}
