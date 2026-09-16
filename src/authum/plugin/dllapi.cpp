#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>

#include <string.h>

static DLL_FUNCTIONS g_function_table = {};

C_DLLEXPORT FORCE_STACK_ALIGN int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
    if (!pFunctionTable || !interfaceVersion)
    {
        return FALSE;
    }
    if (*interfaceVersion != INTERFACE_VERSION)
    {
        *interfaceVersion = INTERFACE_VERSION;
        return FALSE;
    }
    memcpy(pFunctionTable, &g_function_table, sizeof(DLL_FUNCTIONS));
    return TRUE;
}
