/*
 * Globals the Authum core (engine_server.cpp, log.cpp) links against,
 * defined here for the standalone build instead of by the Metamod plugin.
 *
 * The Metamod-only pointers are kept as null: the core's "also log through
 * Metamod" path is guarded on them and simply skips, leaving Authum's own file
 * logging. g_engfuncs / gpGlobals are filled by the standalone GiveFnptrsToDll.
 */
#include <extdll.h>
#include <meta_api.h>

#if defined(__GNUC__)
#    define AUTHUM_EXPORT extern "C" __attribute__((visibility("default")))
#else
#    define AUTHUM_EXPORT extern "C"
#endif

enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals = nullptr;

/* Referenced by core engine_server.cpp / log.cpp; never set in standalone. */
meta_globals_t *gpMetaGlobals = nullptr;
mutil_funcs_t *gpMetaUtilFuncs = nullptr;
gamedll_funcs_t *gpGamedllFuncs = nullptr;

/* engine_server.cpp reads this (its logtag); the Metamod plugin fills it in
 * Meta_Query. Standalone leaves it zeroed. */
plugin_info_t Plugin_info;

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
