#include <extdll.h>
#include <h_export.h>

#include <string.h>

enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;

C_DLLEXPORT FORCE_STACK_ALIGN void WINAPI GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine,
                                                          globalvars_t *pGlobals)
{
    memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
    gpGlobals = pGlobals;
}
