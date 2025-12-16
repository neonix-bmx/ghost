/* Minimal POSIX pipe wrapper using Ghost pipe API */
#include "unistd.h"
#include "ghost/filesystem.h"

int pipe(int fds[2])
{
    if(!fds) return -1;
    g_fd w = G_FD_NONE;
    g_fd r = G_FD_NONE;
    g_fs_pipe_status st = g_pipe(&w, &r);
    if(st != G_FS_PIPE_SUCCESSFUL) return -1;
    // POSIX order: fds[0] = read end, fds[1] = write end
    fds[0] = r;
    fds[1] = w;
    return 0;
}
