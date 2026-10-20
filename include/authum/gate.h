/**
 * @file gate.h
 * @brief Connect gate — Authum decides whether a joining client is allowed in.
 *
 * The host owns the engine hook (`Steam_NotifyClientConnect` on classic HLDS).
 * Child modules register authorizers during their `start` (via the host API
 * that will grow here). Until an authorizer claims a ticket, native Steam
 * validation is left alone.
 *
 * Reference behaviour: qproto 1.0.0 (`raspad-qproto`). That plugin is the
 * freeze; Authum absorbs the gate + Vellum path so loader, auth, and later
 * 2D/3D modules live under one host.
 */

#ifndef AUTHUM_GATE_H
#define AUTHUM_GATE_H

#include <lh/bool.h>
#include <lh/compiler/extern/c.h>

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Install the connect gate after modules have been loaded.
 *
 * Safe to call when the hook is not yet implemented: returns ::lh_bool_true
 * and leaves native Steam auth unchanged.
 *
 * @return ::lh_bool_true on success (including “not yet hooked”).
 */
lh_bool_t
authum_gate_init(void);

/**
 * @brief Remove the connect gate (Meta_Detach / unload).
 */
void
authum_gate_deinit(void);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_GATE_H */
