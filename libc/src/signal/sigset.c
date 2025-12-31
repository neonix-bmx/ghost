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
#include "errno.h"

static int g_sig_valid(int sig) {
	return sig > 0 && sig < SIG_COUNT;
}

int sigemptyset(sigset_t* set) {
	if(set == 0) {
		errno = EINVAL;
		return -1;
	}
	*set = 0;
	return 0;
}

int sigaddset(sigset_t* set, int sig) {
	if(set == 0 || !g_sig_valid(sig)) {
		errno = EINVAL;
		return -1;
	}
	*set |= (1u << sig);
	return 0;
}

int sigdelset(sigset_t* set, int sig) {
	if(set == 0 || !g_sig_valid(sig)) {
		errno = EINVAL;
		return -1;
	}
	*set &= ~(1u << sig);
	return 0;
}

int sigismember(const sigset_t* set, int sig) {
	if(set == 0 || !g_sig_valid(sig)) {
		errno = EINVAL;
		return -1;
	}
	return ((*set) & (1u << sig)) ? 1 : 0;
}
