#include <extdll.h>
#include <meta_api.h>

#include <lh/memory.h>
#include <lh/null.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/void.h>

static enginefuncs_t g_engine_functions = {};

C_DLLEXPORT FORCE_STACK_ALIGN int
GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pengfuncsFromEngine)) ||
        lh_null_eq(lh_ptr_cast(lh_void, interfaceVersion)))
    {
        return FALSE;
    }
    if (*interfaceVersion != ENGINE_INTERFACE_VERSION)
    {
        *interfaceVersion = ENGINE_INTERFACE_VERSION;
        return FALSE;
    }
    lh_memory_copy(pengfuncsFromEngine, sizeof(enginefuncs_t), lh_addr_of(g_engine_functions),
                   sizeof(enginefuncs_t));
    return TRUE;
}
