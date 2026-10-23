/*
 * Globals the Authum core (engine_server.cpp, log.cpp) links against,
 * defined here for the standalone build.
 *
 * The engine's function table and globals are filled by the standalone
 * GiveFnptrsToDll. Metamod is not part of the build: there is no plugin_info_t,
 * no meta_globals_t — the host is the game library itself.
 */
#include <extdll.h>

#if defined(__GNUC__)
#    define AUTHUM_EXPORT extern "C" __attribute__((visibility("default")))
#else
#    define AUTHUM_EXPORT extern "C"
#endif

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
