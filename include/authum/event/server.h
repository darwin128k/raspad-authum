/**
 * @file server.h
 * @brief Map / frame events published by the host (`authum.server.*`).
 *
 * | Event | When |
 * |-------|------|
 * | ::AUTHUM_EVENT_SERVER_ACTIVATE | after the game DLL's `ServerActivate` (map loaded) |
 * | ::AUTHUM_EVENT_SERVER_DEACTIVATE | `ServerDeactivate` (map change / shutdown) |
 * | ::AUTHUM_EVENT_SERVER_FRAME | `StartFrame`, once per server frame |
 * | ::AUTHUM_EVENT_SERVER_PRECACHE | the map is loading: the only time `authum_engine_precache_*`
 * works | | ::AUTHUM_EVENT_SERVER_SHUTDOWN | `GameShutdown`, the server is quitting | |
 * ::AUTHUM_EVENT_SERVER_LOG | a line of the HLDS log (`log on`), see ::authum_event_server_log_t |
 *
 * All but the log line carry ::authum_event_server_t. Frame handlers run every
 * tick: keep them short.
 */

#ifndef AUTHUM_EVENT_SERVER_H
#define AUTHUM_EVENT_SERVER_H

#include <authum/char.h>
#include <authum/extern.h>
#include <authum/float.h>
#include <authum/size.h>

#define AUTHUM_EVENT_SERVER_ACTIVATE "authum.server.activate"
#define AUTHUM_EVENT_SERVER_DEACTIVATE "authum.server.deactivate"
#define AUTHUM_EVENT_SERVER_FRAME "authum.server.frame"
#define AUTHUM_EVENT_SERVER_PRECACHE "authum.server.precache"
#define AUTHUM_EVENT_SERVER_SHUTDOWN "authum.server.shutdown"
#define AUTHUM_EVENT_SERVER_LOG "authum.server.log"

enum
{
    authum_event_server_map_max = 64,
    authum_event_server_log_max = 512
};

struct authum_event_server
{
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

/**
 * One line the game wrote to the HLDS log — the lines stats are made of
 * (`"Vega<2><STEAM_0:1:1><CT>" killed "Gordon<3><…><TERRORIST>" with "ak47"`).
 * Only while logging is on (`log on`). Read-only. Authum's own log lines
 * are not reported.
 */
struct authum_event_server_log
{
    /** The line without its date prefix and trailing newline. */
    authum_char_t text[authum_event_server_log_max];
    /** Seconds since map start. */
    authum_float_t game_time;
    authum_i64_t unix_time;
};
typedef struct authum_event_server_log authum_event_server_log_t;

#endif /* AUTHUM_EVENT_SERVER_H */
