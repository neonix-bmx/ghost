#include "signal.h"
#include "errno.h"
#include "ghost/tasks.h"

int kill(pid_t pid, int sig)
{
    // Minimal: only supports SIGKILL-like behavior; other signals unsupported
    if(sig != SIGKILL && sig != SIGTERM)
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
