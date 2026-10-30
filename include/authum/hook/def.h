/**
 * @file def.h
 * @brief The types a hook is made of: the point, the call, the handler.
 */

#ifndef AUTHUM_HOOK_DEF_H
#define AUTHUM_HOOK_DEF_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/ptr.h>

/**
 * @brief The points a module may hook.
 */
typedef enum authum_hook_point
{
    /** `ClientConnect` — the player may be refused here. */
    authum_hook_point_client_connect = 0,
    /** `ClientKill` — the kill may be refused here. */
    authum_hook_point_client_kill = 1,
    /** `ClientCommand` — the command may be swallowed here. */
    authum_hook_point_client_command = 2,
    /** How many points there are. */
    authum_hook_point_count = 3
} authum_hook_point_t;

/** @brief How long a name in a call may be. */
enum
{
    authum_hook_name_max = 64,
    /** How long an address in a call may be. */
    authum_hook_address_max = 64,
    /** How long the reason shown to a refused client may be. */
    authum_hook_reject_max = 128
};

/**
 * @struct authum_hook_call
 * @brief The call a handler sees; it may change it.
 */
struct authum_hook_call
{
    /** The point being hooked. */
    authum_hook_point_t point;
    /** The player index the call is about, or -1. */
    authum_i32_t index;
    /** The player's name in the call; a handler may change it. */
    authum_char_t name[authum_hook_name_max];
    /** The player's address in the call; a handler may change it. */
    authum_char_t address[authum_hook_address_max];
    /** Whether the game function is called at all. True by default. */
    authum_bool_t proceed;
    /** The reason shown to a refused client; empty means the default one. */
    authum_char_t reject[authum_hook_reject_max];
};
typedef struct authum_hook_call authum_hook_call_t;

/**
 * @typedef authum_hook_fn
 * @brief A handler: it may change @p call and say whether to continue.
 *
 * @param user The value given when the handler was added.
 * @param call The call; change it in place.
 * @return True to continue with the next handler, false to stop the chain.
 */
typedef authum_bool_t (*authum_hook_fn)(authum_ptr user, authum_hook_call_t *call);

#endif /* AUTHUM_HOOK_DEF_H */
