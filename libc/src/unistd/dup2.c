#include "unistd.h"
#include "ghost/filesystem.h"
#include "ghost/tasks.h"

int dup2(int oldfd, int newfd)
{
    if(newfd < 0) return -1;
    if(oldfd == newfd) return newfd;
    g_pid pid = g_get_pid();
    g_fd res = g_clone_fd_ts(oldfd, pid, newfd, pid, NULL);
    return (res == G_FD_NONE) ? -1 : res;
}
