#include <extdll.h>
#include <h_export.h>

#include <lh/memory.h>
#include <lh/util/addr.h>

enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;

C_DLLEXPORT FORCE_STACK_ALIGN void WINAPI GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine,
                                                          globalvars_t *pGlobals)
{
    lh_memory_copy(lh_addr_of(g_engfuncs), sizeof(g_engfuncs), pengfuncsFromEngine,
                   sizeof(enginefuncs_t));
    gpGlobals = pGlobals;
}
