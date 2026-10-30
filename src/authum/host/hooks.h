/**
 * @file hooks.h
 * @brief The chain of module handlers inside `authum` (not for modules).
 *
 * One slot per point, the handlers in it and the game's own function at the
 * end. The module that hooked last is called first. A handler that overruns
 * its budget is dropped, so one module cannot stall the game loop.
 */

#ifndef AUTHUM_HOST_HOOKS_H
#define AUTHUM_HOST_HOOKS_H

#include <authum/hook.h>
#include <authum/size.h>

/** @brief How many handlers one point may hold. */
enum
{
    authum_hooks_max = 16
};

/** @brief What one handler may spend on one call, in microseconds. */
enum
{
    authum_hook_budget_us = 5000
};

/** @brief One handler in a chain. */
typedef struct authum_hook_handler
{
    /** The handler. */
    authum_hook_fn fn;
    /** The value it was given. */
    authum_ptr user;
    /** The module that added it. */
    authum_ptr owner;
} authum_hook_handler_t;

/** @brief The handlers of one point. */
typedef struct authum_hook_chain
{
    /** The handlers, in the order they were hooked. */
    authum_hook_handler_t handlers[authum_hooks_max];
    /** How many handlers the slot holds. */
    authum_usize_t count;
} authum_hook_chain_t;

/** @brief Every point's chain. */
typedef struct authum_hook_registry
{
    /** One chain per point. */
    authum_hook_chain_t chains[authum_hook_point_count];
} authum_hook_registry_t;

AUTHUM_EXTERN_C_BEGIN

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
 * @return False when the point is unknown, the handler is null, or the point
 *         already holds as many handlers as it may.
 */
authum_bool_t
authum_hook_registry_add(authum_hook_registry_t *self, authum_ptr owner, authum_hook_point_t point,
                 authum_hook_fn fn, authum_ptr user);

/**
 * @brief Remove a handler from a point.
 */
authum_bool_t
authum_hook_registry_remove(authum_hook_registry_t *self, authum_ptr owner, authum_hook_point_t point,
                    authum_hook_fn fn, authum_ptr user);

/**
 * @brief Remove every handler @p owner added.
 */
void
authum_hook_registry_drop_owner(authum_hook_registry_t *self, authum_ptr owner);

/**
 * @brief Run the chain of @p point.
 *
 * A handler that stops the chain ends it. A handler that overruns the budget
 * is dropped and the rest of the chain runs.
 *
 * @param call    The call the handlers see.
 * @param overrun Receives whether a handler was dropped for its budget; may be
 *                null. The core does not log: the caller owns the journal.
 * @return Whether the game function is called at all.
 */
authum_bool_t
authum_hook_registry_call(authum_hook_registry_t *self, authum_hook_point_t point,
                          authum_hook_call_t *call, authum_bool_t *overran);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_HOOKS_H */
