/**
 * @file hook.h
 * @brief A module's place in the game's call chain (no lh).
 *
 * An **event** tells a module that something happened; a **hook** is a module
 * that stands in the call itself. A handler reads the call, may change it,
 * and — by clearing ::authum_hook_call_t::proceed — may stop the game from
 * seeing it at all.
 *
 * The core owns the chain: the handlers in it, their order (the module that
 * hooked last is called first), the budget they may spend, and their removal
 * when their module unloads. A module never touches the chain itself.
 *
 * @code
 * authum_bool_t
 * my_hook(authum_ptr user, authum_hook_call_t *call)
 * {
 *     if (!allowed(call->index))
 *     {
 *         call->proceed = authum_bool_false;
 *         authum_cstr_copy(call->reject, sizeof call->reject, "not welcome here");
 *         return authum_bool_false;
 *     }
 *     return authum_bool_true;
 * }
 *
 * authum_hooks_add(authum_host_get_hooks(host), authum_hook_point_client_connect, my_hook,
 *                  authum_null);
 * @endcode
 *
 * @see docs/hooks.md for the design.
 */

#ifndef AUTHUM_HOOK_H
#define AUTHUM_HOOK_H

#include <authum/extern.h>
#include <authum/hook/ops.h>
#include <authum/null.h>

/**
 * @struct authum_hooks
 * @brief Per-module handle: host ops + who is calling.
 */
struct authum_hooks
{
    const authum_hook_ops_t *ops;
    authum_ptr owner;
    /**
     * The module's tag: the host writes it, the chain names it in the log
     * when a handler of this module stops a call. Null when there is none.
     */
    const authum_char_t *tag;
};
typedef struct authum_hooks authum_hooks_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The host ops table behind @p self; null when there is none.
 */
const authum_hook_ops_t *
authum_hooks_get_ops(const authum_hooks_t *self);

/**
 * @brief The module that owns @p self.
 */
authum_ptr
authum_hooks_get_owner(const authum_hooks_t *self);

/**
 * @brief The tag of the module that owns @p self; null when there is none.
 */
const authum_char_t *
authum_hooks_get_tag(const authum_hooks_t *self);

/**
 * @brief Add a handler to a point; the module that owns @p self is the owner of
 * the handler, and its handlers are removed when it unloads.
 *
 * @return False when the point is unknown, the handler is null, or the point
 *         already holds as many handlers as it may.
 */
authum_bool_t
authum_hooks_add(const authum_hooks_t *self, authum_hook_point_t point, authum_hook_fn fn,
                 authum_ptr user);

/**
 * @brief Remove a handler from a point.
 */
authum_bool_t
authum_hooks_remove(const authum_hooks_t *self, authum_hook_point_t point, authum_hook_fn fn,
                    authum_ptr user);

/**
 * @brief The name of a point, for logs and messages.
 */
const authum_char_t *
authum_hook_point_name(authum_hook_point_t point);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOOK_H */
