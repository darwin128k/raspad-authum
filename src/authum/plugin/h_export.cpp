#include <extdll.h>
#include <h_export.h>

#include <lh/memory.h>
#include <lh/util/addr.h>

#ifndef AUTHUM_MODULE_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#        define AUTHUM_MODULE_EXPORT __declspec(dllexport)
#    elif defined(__GNUC__)
#        define AUTHUM_MODULE_EXPORT __attribute__((visibility("default")))
#    else
#        define AUTHUM_MODULE_EXPORT
#    endif
#endif

enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;

C_DLLEXPORT FORCE_STACK_ALIGN void WINAPI GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine,
                                                          globalvars_t *pGlobals)
{
    lh_memory_copy(lh_addr_of(g_engfuncs), sizeof(g_engfuncs), pengfuncsFromEngine,
                   sizeof(enginefuncs_t));
    gpGlobals = pGlobals;
}

extern "C" {

AUTHUM_MODULE_EXPORT void *
authum_engine_funcs(void)
{
    return &g_engfuncs;
}

AUTHUM_MODULE_EXPORT void *
authum_globals(void)
{
    return gpGlobals;
}

} /* extern "C" */
