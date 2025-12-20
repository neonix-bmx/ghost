/* Minimal POSIX wait interfaces */
#pragma once

#include "ghost/common.h"
#include "ghost/stdint.h"
#include "sys/types.h"

__BEGIN_C

/* Options */
#define WNOHANG 1
#define WUNTRACED 2
#define WCONTINUED 4

/* Macros for status (minimal stubs to satisfy ports/tests) */
#define WIFEXITED(status)    (1)
#define WEXITSTATUS(status)  ((int)(((status) >> 8) & 0xFF))
#define WIFSIGNALED(status)  (0)
#define WTERMSIG(status)     (0)
#define WIFSTOPPED(status)   (0)
#define WSTOPSIG(status)     (0)
#define WIFCONTINUED(status) (0)
#define WCOREDUMP(status)    (0)

/* Prototypes for completeness */
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(int *status);

pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(int *status);

__END_C
