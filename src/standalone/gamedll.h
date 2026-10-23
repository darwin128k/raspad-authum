/*
 * The real game library behind standalone Authum: the DLL Authum loads and
 * forwards to, the way the engine would have loaded it directly.
 */
#ifndef AUTHUM_STANDALONE_GAMEDLL_H
#define AUTHUM_STANDALONE_GAMEDLL_H

#include <extdll.h>

namespace authum_standalone
{
/* Load the real game DLL named in cstrike/liblist.gam (gamedll_linux), from a
 * directory beside authum.so, and hand it the engine's functions. Returns
 * false and logs if the DLL is missing or has no GiveFnptrsToDll. */
bool gamedll_load(enginefuncs_t *engine_funcs, globalvars_t *globals);

/* The game's entity-API table (its own pointers), filled on first request.
 * Bridges the old flat GetEntityAPI to GetEntityAPI2 when that is all the game
 * exports. Returns false if the game provides neither. */
bool gamedll_entity_api(DLL_FUNCTIONS *out, int interface_version);

/* The game's NEW_DLL_FUNCTIONS, or false when it exports none. */
bool gamedll_new_functions(NEW_DLL_FUNCTIONS *out, int *interface_version);

/* Any symbol exported by the game DLL (entity classnames, Server_Get…). */
void *gamedll_symbol(const char *name);

} // namespace authum_standalone

#endif /* AUTHUM_STANDALONE_GAMEDLL_H */
