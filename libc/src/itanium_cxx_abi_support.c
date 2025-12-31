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

// The functions in this file are implemented as defined in the Itanium C++ ABI
// standard. These functions are required by GCC for special cases, see the
// individual documentation for details.

typedef struct {
	void (*func)(void*);
	void* arg;
	void* dso_handle;
	int called;
} g_atexit_entry;

#define G_ATEXIT_MAX 64

static g_atexit_entry g_atexit_entries[G_ATEXIT_MAX];
static unsigned int g_atexit_count = 0;

int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle) {
	if(func == 0 || g_atexit_count >= G_ATEXIT_MAX) {
		return -1;
	}

	g_atexit_entries[g_atexit_count].func = func;
	g_atexit_entries[g_atexit_count].arg = arg;
	g_atexit_entries[g_atexit_count].dso_handle = dso_handle;
	g_atexit_entries[g_atexit_count].called = 0;
	++g_atexit_count;
	return 0;
}

void __cxa_finalize(void* dso_handle) {
	if(g_atexit_count == 0) {
		return;
	}

	for(unsigned int i = g_atexit_count; i > 0; --i) {
		g_atexit_entry* entry = &g_atexit_entries[i - 1];
		if(entry->called) {
			continue;
		}
		if(dso_handle != 0 && entry->dso_handle != dso_handle) {
			continue;
		}
		entry->called = 1;
		entry->func(entry->arg);
	}
}

