

#ifndef _GVFS_SETPROCTITLE_H_INCLUDED_
#define _GVFS_SETPROCTITLE_H_INCLUDED_


#define GVFS_SETPROCTITLE_PAD       '\0'

gvfs_int_t gvfs_init_setproctitle(void);
void gvfs_setproctitle(char *title);


#endif /* _GVFS_SETPROCTITLE_H_INCLUDED_ */
