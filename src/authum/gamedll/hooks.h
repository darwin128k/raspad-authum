/*
 * Authum's event-publishing overlay over the game's DLL_FUNCTIONS /
 * NEW_DLL_FUNCTIONS tables.
 */
#ifndef AUTHUM_GAMEDLL_HOOKS_H
#define AUTHUM_GAMEDLL_HOOKS_H

#include <extdll.h>

/* logs helpers used by the frame / activate hooks (C linkage from logs.c). */
extern "C" void authum_logs_flush(void);
extern "C" void authum_logs_reload(void);

namespace authum_gamedll
{
/* Fill @p out with the game's entity table, Authum's hooks overlaid. The
 * game's own pointers are kept for later forwarding. */
bool hooks_fill_entity_table(DLL_FUNCTIONS *out, int interface_version);

/* Same for NEW_DLL_FUNCTIONS (CvarValue2, GameShutdown). */
bool hooks_fill_new_table(NEW_DLL_FUNCTIONS *out, int *interface_version);

} // namespace authum_gamedll

#endif /* AUTHUM_GAMEDLL_HOOKS_H */
