

#include <gvfs_config.h>
#include <gvfs_core.h>


int gvfs_nonblocking(gvfs_socket_t s)
{
    int  nb;

    nb = 1;

    return ioctl(s, FIONBIO, &nb);
}


int gvfs_blocking(gvfs_socket_t s)
{
    int  nb;

    nb = 0;

    return ioctl(s, FIONBIO, &nb);
}
