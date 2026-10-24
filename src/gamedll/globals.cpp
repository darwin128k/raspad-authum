/*
 * Globals the Authum core (engine_server.cpp, log.cpp) links against,
 * defined here, in the game-library layer.
 *
 * The engine's function table and globals are filled by Authum's
 * GiveFnptrsToDll. Metamod is not part of the build: there is no plugin_info_t,
 * no meta_globals_t — the host is the game library itself.
 */
#include <authum/export.h>
#include <extdll.h>

#define AUTHUM_EXPORT extern "C" AUTHUM_IMAGE_EXPORT

enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals = nullptr;

AUTHUM_EXPORT void *
authum_engine_funcs(void)
{
    return &g_engfuncs;
}

AUTHUM_EXPORT void *
authum_globals(void)
{
    return gpGlobals;
}
