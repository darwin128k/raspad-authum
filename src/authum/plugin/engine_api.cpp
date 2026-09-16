#include <extdll.h>
#include <meta_api.h>

#include <string.h>

static enginefuncs_t g_engine_functions = {};

C_DLLEXPORT FORCE_STACK_ALIGN int GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine,
                                                     int *interfaceVersion)
{
    if (!pengfuncsFromEngine || !interfaceVersion)
    {
        return FALSE;
    }
    if (*interfaceVersion != ENGINE_INTERFACE_VERSION)
    {
        *interfaceVersion = ENGINE_INTERFACE_VERSION;
        return FALSE;
    }
    memcpy(pengfuncsFromEngine, &g_engine_functions, sizeof(enginefuncs_t));
    return TRUE;
}
