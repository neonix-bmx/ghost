#include "unistd.h"
#include "ghost/filesystem.h"
#include "ghost/tasks.h"

int dup(int oldfd)
{
    g_pid pid = g_get_pid();
    g_fd newfd = g_clone_fd(oldfd, pid, pid);
    return (newfd == G_FD_NONE) ? -1 : newfd;
}
