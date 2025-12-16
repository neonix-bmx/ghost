/* Minimal POSIX wait interfaces */
#pragma once

#include "ghost/common.h"
#include "ghost/stdint.h"
#include "sys/types.h"

__BEGIN_C

/* Options */
#define WNOHANG 1
#define WUNTRACED 2

/* Macros for status (we only report normal exit) */
#define WIFEXITED(status) (1)
#define WEXITSTATUS(status) ((int)(((status) >> 8) & 0xFF))

pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(int *status);

__END_C
