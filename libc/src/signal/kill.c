#include "signal.h"
#include "errno.h"
#include "ghost/tasks.h"

int kill(pid_t pid, int sig)
{
    if(sig < 0 || sig >= SIG_COUNT)
    {
        errno = EINVAL;
        return -1;
    }

    if(pid == g_get_pid())
        return raise(sig);

    if(sig != SIGTERM)
    {
        errno = ENOSYS;
        return -1;
    }

    g_kill_status st = g_kill(pid);
    if(st != G_KILL_STATUS_SUCCESSFUL)
    {
        errno = ESRCH;
        return -1;
    }
    return 0;
}
