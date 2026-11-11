/**
 * @file hooks.h
 * @brief The chain of module handlers inside `authum` (not for modules).
 *
 * One lh hook chain (`lh/os/module/hook.h`) per point, and the game's own
 * function at the end. The module that hooked last is called first. A handler
 * that overruns its budget is dropped, so one module cannot stall the game loop.
 * Authum keeps the points, the budget and the call's verdict.
 */

#ifndef AUTHUM_HOST_HOOKS_H
#define AUTHUM_HOST_HOOKS_H

#include <authum/hook.h>
#include <authum/size.h>
#include <lh/os/module/hook.h>

/** @brief What one handler may spend on one call, in microseconds. */
enum
{
    authum_hook_budget_us = 5000
};

/** @brief Every point's chain. */
typedef struct authum_hook_registry
{
    /** One chain per point. */
    lh_os_module_hook_t chains[authum_hook_point_count];
} authum_hook_registry_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The chain of @p point (level 1).
 *
 * @param self  The registry; must not be null.
 * @param point Below ::authum_hook_point_count.
 */
lh_os_module_hook_t *
authum_hook_registry_get_chain_mut(authum_hook_registry_t *self, authum_hook_point_t point);

/**
 * @brief lh's handler for a module's handler: the same arguments, an Authum call
 *        for lh's opaque one, so the module ABI meets the chain in one cast.
 */
lh_os_module_hook_cb
authum_hook_registry_get_callback(authum_hook_fn fn);

/**
 * @brief The chain every module hooks into.
 */
authum_hook_registry_t *
authum_hook_registry(void);

/**
 * @brief The ops table a module's handle points at.
 */
const authum_hook_ops_t *
authum_hooks_host_ops(void);

/**
 * @brief Empty every chain.
 */
void
authum_hook_registry_init(authum_hook_registry_t *self);

/**
 * @brief Add a handler to a point; the last one hooked is called first.
 *
 * @param owner The module that adds it; its handlers go when it unloads.
 * @param tag   The tag of that module: the chain names it when the handler
 *              stops a call. May be null.
 * @return False when the point is unknown, the handler is null, or the point
 *         already holds as many handlers as it may.
 */
authum_bool_t
authum_hook_registry_add(authum_hook_registry_t *self, authum_ptr owner, const authum_char_t *tag,
                         authum_hook_point_t point, authum_hook_fn fn, authum_ptr user);

/**
 * @brief Remove a handler from a point.
 */
authum_bool_t
authum_hook_registry_remove(authum_hook_registry_t *self, authum_ptr owner,
                            authum_hook_point_t point, authum_hook_fn fn, authum_ptr user);

/**
 * @brief Remove every handler @p owner added.
 */
void
authum_hook_registry_drop_owner(authum_hook_registry_t *self, authum_ptr owner);

/**
 * @brief Run the chain of @p point.
 *
 * A handler that stops the chain ends it, and the call gets the mark of the
 * module that stopped it. A handler that overruns the budget is dropped and the
 * rest of the chain runs.
 *
 * @param call    The call the handlers see.
 * @param overran Receives whether a handler was dropped for its budget; may be
 *                null. The core does not log: the caller owns the journal.
 * @return Whether the game function is called at all.
 */
authum_bool_t
authum_hook_registry_call(authum_hook_registry_t *self, authum_hook_point_t point,
                          authum_hook_call_t *call, authum_bool_t *overran);

/**
 * @brief Register @p fn of module @p owner at hook @p point: the hook table's add.
 */
authum_bool_t
authum_hooks_host_add(authum_ptr owner, const authum_char_t *tag, authum_hook_point_t point,
                      authum_hook_fn fn, authum_ptr user);

/**
 * @brief Remove @p owner's @p fn from hook @p point: the hook table's remove.
 */
authum_bool_t
authum_hooks_host_remove(authum_ptr owner, authum_hook_point_t point, authum_hook_fn fn,
                         authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_HOOKS_H */
