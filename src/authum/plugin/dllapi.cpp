#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>

#include <lh/memory.h>
#include <lh/null.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/void.h>

static DLL_FUNCTIONS g_function_table = {};

C_DLLEXPORT FORCE_STACK_ALIGN int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pFunctionTable)) ||
        lh_null_eq(lh_ptr_cast(lh_void, interfaceVersion)))
    {
        return FALSE;
    }
    if (*interfaceVersion != INTERFACE_VERSION)
    {
        *interfaceVersion = INTERFACE_VERSION;
        return FALSE;
    }
    lh_memory_copy(pFunctionTable, sizeof(DLL_FUNCTIONS), lh_addr_of(g_function_table),
                   sizeof(DLL_FUNCTIONS));
    return TRUE;
}
