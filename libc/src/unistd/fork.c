#include "unistd.h"
#include "ghost/tasks.h"

pid_t fork(void)
{
    return (pid_t) g_fork();
}
