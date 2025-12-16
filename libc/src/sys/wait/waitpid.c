#include "sys/wait.h"
#include "errno.h"
#include "ghost/tasks.h"

/*
 * Minimal waitpid: only supports pid > 0 (explicit wait). Options ignored
 * except WNOHANG: if set, returns 0 immediately.
 */
pid_t waitpid(pid_t pid, int *status, int options)
{
    if(options & WNOHANG) return 0;
    if(pid <= 0)
    {
        errno = ENOSYS;
        return -1;
    }
    g_join(pid);
    if(status) *status = 0; // no exit code propagation available
    return pid;
}

pid_t wait(int *status)
{
    // No child tracking; require explicit pid
    errno = ENOSYS;
    return -1;
}
