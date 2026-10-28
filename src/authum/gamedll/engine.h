/*
 * The engine as the real game sees it: the engine's own functions, with the
 * few Authum listens to — user messages and the HLDS log — routed through
 * Authum first.
 */

#ifndef AUTHUM_GAMEDLL_ENGINE_H
#define AUTHUM_GAMEDLL_ENGINE_H

#include <extdll.h>

namespace authum_gamedll
{
/* Fill @p out with the engine's functions (@p engine) and Authum's hooks over
 * them: RegUserMsg (to name the messages), MessageBegin … MessageEnd (the
 * game's user messages become `authum.message.<Name>` events) and
 * AlertMessage (log lines become `authum.server.log`). The game is handed
 * @p out instead of the engine's table; Authum itself keeps calling the
 * engine directly, so what it sends is never caught here. */
void engine_hook_table(enginefuncs_t *out, const enginefuncs_t *engine);

} // namespace authum_gamedll

#endif /* AUTHUM_GAMEDLL_ENGINE_H */
