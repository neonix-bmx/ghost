#include "unistd.h"
#include "errno.h"
#include "ghost/tasks.h"

pid_t fork(void)
{
    pid_t pid = (pid_t) g_fork();
    if(pid == (pid_t) G_PID_NONE)
    {
        errno = ENOSYS;
        return -1;
    }
    return pid;
}
