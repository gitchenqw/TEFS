
#include <gvfs_config.h>
#include <gvfs_core.h>

static gvfs_module_t *first_module = NULL;
static gvfs_module_t **last_module = &first_module;

#define REGISTER_MODULE(x)													   \
	{                                                                          \
		extern gvfs_module_t gvfs_##x##_module;                                \
		gvfs_register_module(&gvfs_##x##_module);                              \
	}

gvfs_module_t *gvfs_module_next(gvfs_module_t *module)
{
	if (module) {
		return module->next;
	}
	
	return first_module;
}

static VOID *gvfs_atomic_ptr_cas(VOID * volatile *ptr, VOID *oldval,
	VOID *newval)
{
    if (*ptr == oldval) {
        *ptr = newval;
        return oldval;
    }
    return *ptr;
}

static VOID gvfs_register_module(gvfs_module_t *module)
{
	gvfs_module_t **p = last_module;

	module->next = NULL;
	
	while (*p || gvfs_atomic_ptr_cas((void * volatile *)p, NULL, module)) {
		p = &(*p)->next;
	}

	last_module = &module->next;
}

VOID gvfs_register_all(VOID)
{
	REGISTER_MODULE(core);
	REGISTER_MODULE(event);
	REGISTER_MODULE(http);
	REGISTER_MODULE(upstream);
}


