/*
 * The engine's contract with the game module — the part Metamod-P covers.
 *
 * The engine resolves more than the four entry points in the game module by
 * name. Two of them Metamod-P answers itself and forwards to the game:
 *
 *   GameDLLInit  — the game's own start-up. It sizes the entity hash table;
 *                  without it the game divides by zero on the first entity
 *                  (CaseInsensitiveHash with an empty table).
 *   DispatchSpawn — the spawner the engine calls for every map entity.
 *
 * The game exports both as C++ names (_Z11GameDLLInitv,
 * _Z13DispatchSpawnP7edict_s), the engine looks for the plain ones. Standing
 * in for Metamod, Authum exports the plain names and forwards them.
 */
#include "gamedll.h"

#include <authum/export.h>

#include <extdll.h>

/* The engine looks these up by name in the game module: they must be
 * exported (the build hides everything else). */
#define AUTHUM_GAME_API extern "C" AUTHUM_IMAGE_EXPORT

namespace
{
/* The plain name first (a game that exports it directly), then the C++ one. */
void *
find_game_symbol(const char *plain, const char *mangled)
{
    void *symbol = authum_standalone::gamedll_symbol(plain);

    if (symbol == nullptr)
    {
        symbol = authum_standalone::gamedll_symbol(mangled);
    }
    return symbol;
}
} // namespace

AUTHUM_GAME_API void
GameDLLInit(void)
{
    typedef void (*fn_t)(void);
    fn_t fn = reinterpret_cast<fn_t>(find_game_symbol("GameDLLInit", "_Z11GameDLLInitv"));

    if (fn != nullptr)
    {
        fn();
    }
}

AUTHUM_GAME_API void
DispatchSpawn(edict_t *pent)
{
    typedef void (*fn_t)(edict_t *);
    fn_t fn =
        reinterpret_cast<fn_t>(find_game_symbol("DispatchSpawn", "_Z13DispatchSpawnP7edict_s"));

    if (fn != nullptr)
    {
        fn(pent);
    }
}
