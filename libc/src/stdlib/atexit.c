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

extern int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle);

typedef struct {
	void (*func)(void);
} g_atexit_wrapper;

#define G_ATEXIT_MAX 64

static g_atexit_wrapper g_atexit_wrappers[G_ATEXIT_MAX];
static unsigned int g_atexit_wrapper_count = 0;

static void g_atexit_thunk(void* arg) {
	g_atexit_wrapper* wrapper = (g_atexit_wrapper*)arg;
	if(wrapper != 0 && wrapper->func != 0) {
		wrapper->func();
	}
}

int atexit(void (*func)(void)) {
	if(func == 0) {
		return -1;
	}
	if(g_atexit_wrapper_count >= G_ATEXIT_MAX) {
		return -1;
	}

	g_atexit_wrappers[g_atexit_wrapper_count].func = func;
	if(__cxa_atexit(g_atexit_thunk, &g_atexit_wrappers[g_atexit_wrapper_count], 0) != 0) {
		return -1;
	}
	++g_atexit_wrapper_count;
	return 0;
}
