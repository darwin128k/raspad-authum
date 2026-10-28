/**
 * @file message.h
 * @brief User messages the game sends: watch, change or block them.
 *
 * Subscribe to `authum.message.<Name>` — the game's message name, e.g.
 * `authum.message.DeathMsg`, `authum.message.SayText`, `authum.message.TextMsg`
 * (::AUTHUM_EVENT_MESSAGE_PREFIX). The handler gets the whole message as
 * ::authum_event_message_t: read its arguments, change them with
 * `authum_event_message_set_*`, or set `block` and nobody receives it.
 *
 * These are the real game's messages. Only messages someone subscribed to
 * are held back, so the others cost nothing; a held message reaches the
 * engine once, after the handlers ran. Messages Authum and its modules send
 * are not reported (no loops).
 *
 * Argument indices count from 0, as the game writes them; kinds are the
 * `authum_message_arg_*` values of <authum/engine/message.h>.
 */

#ifndef AUTHUM_EVENT_MESSAGE_H
#define AUTHUM_EVENT_MESSAGE_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/engine/message.h>
#include <authum/float.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/** Prefix of the per-message event names (`authum.message.DeathMsg`). */
#define AUTHUM_EVENT_MESSAGE_PREFIX "authum.message."

enum
{
    authum_event_message_name_max = 32,
    /** The engine caps a user message at 192 bytes: room for any of them. */
    authum_event_message_args_max = 192,
    authum_event_message_text_max = 512
};

struct authum_event_message_arg
{
    /** `authum_message_arg_*`. */
    authum_i32_t kind;
    /** byte / char / short / long / entity. */
    authum_i32_t value;
    /** angle / coord. */
    authum_float_t real;
    /** string: offset of its text in ::authum_event_message_t.text. */
    authum_u32_t text;
};
typedef struct authum_event_message_arg authum_event_message_arg_t;

struct authum_event_message
{
    /** sizeof(authum_event_message_t) as built by the host. */
    authum_usize_t size;
    /** `authum_message_dest_*` (or another engine MSG_* value). */
    authum_i32_t dest;
    authum_i32_t msg_id;
    authum_char_t name[authum_event_message_name_max];
    /** The receiving player for the one-player destinations, else 0. */
    authum_i32_t index;
    authum_bool_t has_origin;
    authum_float_t origin[3];
    authum_i32_t argc;
    authum_event_message_arg_t args[authum_event_message_args_max];
    /** String arguments, NUL-terminated, one after another. */
    authum_char_t text[authum_event_message_text_max];
    authum_u32_t text_used;
    /** Set to true to drop the message: nobody receives it. */
    authum_bool_t block;
};
typedef struct authum_event_message authum_event_message_t;

/** Argument @p i as an integer (byte … long, entity); 0 when out of range. */
authum_i32_t
authum_event_message_get_i32(const authum_event_message_t *ev, authum_i32_t i);

/** Argument @p i as a number (angle, coord); 0 when out of range. */
authum_float_t
authum_event_message_get_float(const authum_event_message_t *ev, authum_i32_t i);

/** Argument @p i as text; "" when out of range or not a string. */
const authum_char_t *
authum_event_message_get_string(const authum_event_message_t *ev, authum_i32_t i);

/** Replace integer argument @p i (its kind stays). False when out of range. */
authum_bool_t
authum_event_message_set_i32(authum_event_message_t *ev, authum_i32_t i, authum_i32_t value);

authum_bool_t
authum_event_message_set_float(authum_event_message_t *ev, authum_i32_t i,
                               authum_float_t value);

/** Replace string argument @p i; false when out of range, not a string or no room. */
authum_bool_t
authum_event_message_set_string(authum_event_message_t *ev, authum_i32_t i,
                                const authum_char_t *text);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENT_MESSAGE_H */
