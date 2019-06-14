
#ifndef _GVFS_MODULES_H_INCLUDE_
#define _GVFS_MODULES_H_INCLUDE_

#include <gvfs_config.h>
#include <gvfs_core.h>

struct gvfs_module_s {
	CHAR *name;
	
	UINT32 index;
	
    VOID *(*create_conf)(gvfs_cycle_t *cycle);
    
    CHAR *(*init_conf)(gvfs_cycle_t *cycle);
    
    INT32 (*init_module)(gvfs_cycle_t *cycle);
    
    INT32 (*init_process)(gvfs_cycle_t *cycle);
    
    VOID (*exit_process)(gvfs_cycle_t *cycle);
    
    VOID (*exit_master)(gvfs_cycle_t *cycle);
    
	gvfs_module_t *next;
};

gvfs_module_t *gvfs_module_next(gvfs_module_t *module);

VOID gvfs_register_all(VOID);

#endif /* _GVFS_MODULES_H_INCLUDE_ */
