/**
 * @file message.h
 * @brief User message constants: where a message goes and what it carries.
 *
 * A user message (`SayText`, `TextMsg`, `DeathMsg`, …) is a game-defined
 * packet: an id looked up by name, a destination and a list of typed
 * arguments. Send one with `authum_engine_message_begin` / `_write_*` /
 * `_end`; watch or block the game's with the `authum.message.<Name>` events
 * (<authum/event/message.h>).
 */

#ifndef AUTHUM_ENGINE_MESSAGE_H
#define AUTHUM_ENGINE_MESSAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

    /** Destination (the engine's MSG_* values). */
    enum
    {
        /** Every player, unreliable. */
        authum_message_dest_broadcast = 0,
        /** One player (index given), reliable. */
        authum_message_dest_one = 1,
        /** Every player, reliable. */
        authum_message_dest_all = 2,
        /** One player (index given), unreliable. */
        authum_message_dest_one_unreliable = 8,
        /** Spectators / HLTV. */
        authum_message_dest_spec = 9
    };

    /** Argument types (the engine's WRITE_* calls). */
    enum
    {
        authum_message_arg_byte = 0,
        authum_message_arg_char = 1,
        authum_message_arg_short = 2,
        authum_message_arg_long = 3,
        authum_message_arg_angle = 4,
        authum_message_arg_coord = 5,
        authum_message_arg_string = 6,
        authum_message_arg_entity = 7
    };

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENGINE_MESSAGE_H */
