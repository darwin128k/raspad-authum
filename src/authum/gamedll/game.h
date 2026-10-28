/*
 * The real game library behind Authum: the DLL Authum loads and
 * forwards to, the way the engine would have loaded it directly.
 */
#ifndef AUTHUM_GAMEDLL_GAME_H
#define AUTHUM_GAMEDLL_GAME_H

#include <extdll.h>
#include <authum/ptr.h>
#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>

namespace authum_gamedll
{
/* Load the real game library named in addons/authum/gamedll.txt, and hand it
 * the engine's functions. Returns false and logs if the library is missing or
 * has no GiveFnptrsToDll. */
authum_bool_t
gamedll_load(enginefuncs_t *engine_funcs, globalvars_t *globals);

/* The game's entity-API table (its own pointers), filled on first request.
 * Bridges the old flat GetEntityAPI to GetEntityAPI2 when that is all the game
 * exports. Returns false if the game provides neither. */
authum_bool_t
gamedll_entity_api(DLL_FUNCTIONS *out, authum_i32_t interface_version);

/* The game's NEW_DLL_FUNCTIONS, or false when it exports none. */
authum_bool_t
gamedll_new_functions(NEW_DLL_FUNCTIONS *out, authum_i32_t *interface_version);

/* Any symbol exported by the game DLL (entity classnames, Server_Get…). */
authum_ptr
gamedll_symbol(const authum_char_t *name);

} // namespace authum_gamedll

#endif /* AUTHUM_GAMEDLL_GAME_H */
