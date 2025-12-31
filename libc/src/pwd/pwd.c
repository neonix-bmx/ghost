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

#include "pwd.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"

#define G_PWD_NAME_LEN 64
#define G_PWD_DIR_LEN 128
#define G_PWD_SHELL_LEN 64
#define G_PWD_GECOS_LEN 64

static struct passwd g_passwd;
static char g_name_buf[G_PWD_NAME_LEN];
static char g_dir_buf[G_PWD_DIR_LEN];
static char g_shell_buf[G_PWD_SHELL_LEN];
static char g_gecos_buf[G_PWD_GECOS_LEN];

static void g_copy_string(char* dst, size_t dst_size, const char* src) {
	if(dst_size == 0) {
		return;
	}
	if(src == 0) {
		dst[0] = '\0';
		return;
	}
	size_t len = strlen(src);
	if(len >= dst_size) {
		len = dst_size - 1;
	}
	memcpy(dst, src, len);
	dst[len] = '\0';
}

static struct passwd* g_fill_passwd(const char* name, uid_t uid, gid_t gid) {
	const char* home = getenv("HOME");
	const char* shell = getenv("SHELL");
	const char* gecos = getenv("USER");

	if(home == 0 || *home == '\0') {
		home = "/";
	}
	if(shell == 0 || *shell == '\0') {
		shell = "/bin/sh";
	}
	if(gecos == 0 || *gecos == '\0') {
		gecos = name;
	}

	g_copy_string(g_name_buf, sizeof(g_name_buf), name);
	g_copy_string(g_dir_buf, sizeof(g_dir_buf), home);
	g_copy_string(g_shell_buf, sizeof(g_shell_buf), shell);
	g_copy_string(g_gecos_buf, sizeof(g_gecos_buf), gecos);

	g_passwd.pw_name = g_name_buf;
	g_passwd.pw_passwd = (char*)"*";
	g_passwd.pw_uid = uid;
	g_passwd.pw_gid = gid;
	g_passwd.pw_gecos = g_gecos_buf;
	g_passwd.pw_dir = g_dir_buf;
	g_passwd.pw_shell = g_shell_buf;

	return &g_passwd;
}

struct passwd* getpwuid(uid_t uid) {
	uid_t self = getuid();
	if(uid != self && uid != 0) {
		return 0;
	}

	const char* name = getenv("USER");
	if(name == 0 || *name == '\0') {
		name = "root";
	}
	return g_fill_passwd(name, uid, getgid());
}

struct passwd* getpwnam(const char* name) {
	if(name == 0 || *name == '\0') {
		return 0;
	}

	const char* user = getenv("USER");
	if(user == 0 || *user == '\0') {
		user = "root";
	}

	if(strcmp(name, user) != 0 && strcmp(name, "root") != 0) {
		return 0;
	}

	return g_fill_passwd(name, getuid(), getgid());
}
