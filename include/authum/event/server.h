/**
 * @file server.h
 * @brief Map / frame events published by the host (`authum.server.*`).
 *
 * | Event | When |
 * |-------|------|
 * | ::AUTHUM_EVENT_SERVER_ACTIVATE | after the game DLL's `ServerActivate` (map loaded) |
 * | ::AUTHUM_EVENT_SERVER_DEACTIVATE | `ServerDeactivate` (map change / shutdown) |
 * | ::AUTHUM_EVENT_SERVER_FRAME | `StartFrame`, once per server frame |
 *
 * Every one carries ::authum_event_server_t. Frame handlers run every tick:
 * keep them short.
 */

#ifndef AUTHUM_EVENT_SERVER_H
#define AUTHUM_EVENT_SERVER_H

#include <authum/byte.h>
#include <authum/char.h>
#include <authum/float.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define AUTHUM_EVENT_SERVER_ACTIVATE "authum.server.activate"
#define AUTHUM_EVENT_SERVER_DEACTIVATE "authum.server.deactivate"
#define AUTHUM_EVENT_SERVER_FRAME "authum.server.frame"

    enum
    {
        authum_event_server_map_max = 64
    };

    struct authum_event_server
    {
        /** sizeof(authum_event_server_t) as built by the host. */
        authum_usize_t size;
        /** Current map (`de_dust2`). */
        authum_char_t map[authum_event_server_map_max];
        /** `maxplayers` for this map. */
        authum_i32_t max_clients;
        /** Activate only: edicts in use when the map started. */
        authum_i32_t edict_count;
        /** Seconds since map start (`gpGlobals->time`). */
        authum_float_t game_time;
        /** Length of the last frame in seconds (`gpGlobals->frametime`). */
        authum_float_t frame_time;
        /** Wall clock, seconds since 1970-01-01 UTC. */
        authum_i64_t unix_time;
    };
    typedef struct authum_event_server authum_event_server_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_EVENT_SERVER_H */
