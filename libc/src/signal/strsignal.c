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

#include "signal.h"

static const char* g_signal_names[SIG_COUNT] = {
	[SIGHUP] = "Hangup",
	[SIGINT] = "Interrupt",
	[SIGQUIT] = "Quit",
	[SIGILL] = "Illegal instruction",
	[SIGTRAP] = "Trace/breakpoint trap",
	[SIGABRT] = "Aborted",
	[SIGBUS] = "Bus error",
	[SIGFPE] = "Floating point exception",
	[SIGKILL] = "Killed",
	[SIGUSR1] = "User signal 1",
	[SIGSEGV] = "Segmentation fault",
	[SIGUSR2] = "User signal 2",
	[SIGPIPE] = "Broken pipe",
	[SIGALRM] = "Alarm clock",
	[SIGTERM] = "Terminated",
	[SIGCHLD] = "Child exited",
	[SIGCONT] = "Continued",
	[SIGSTOP] = "Stopped (signal)",
	[SIGTSTP] = "Stopped",
	[SIGTTIN] = "Stopped (tty input)",
	[SIGTTOU] = "Stopped (tty output)",
	[SIGSYS] = "Bad system call",
};

char* strsignal(int sig) {
	if(sig <= 0 || sig >= SIG_COUNT || g_signal_names[sig] == 0) {
		return "Unknown signal";
	}
	return (char*)g_signal_names[sig];
}
