// clang-format off
/**
 * @file client.h
 * @brief Player lifecycle events published by the host (`authum.client.*`).
 *
 * Authum hooks the game DLL calls once and forwards them to
 * modules, so no module needs its own hooks:
 *
 * | Event | When | Payload |
 * |-------|------|---------|
 * | ::AUTHUM_EVENT_CLIENT_CONNECT | `ClientConnect` (after the engine auth step) | ::authum_event_client_t, all fields; a handler may set `allow = false` + `reject_reason` |
 * | ::AUTHUM_EVENT_CLIENT_PUT_IN_SERVER | `ClientPutInServer` | ::authum_event_client_t: index, userid, name, auth_id, times |
 * | ::AUTHUM_EVENT_CLIENT_INFO_CHANGED | `ClientUserInfoChanged` | ::authum_event_client_t: index, userid, name, times |
 * | ::AUTHUM_EVENT_CLIENT_DISCONNECT | `ClientDisconnect` | ::authum_event_client_t: index, userid, name, auth_id, times |
 * | ::AUTHUM_EVENT_CLIENT_COMMAND | `ClientCommand` (`say`, `menuselect`, …) | ::authum_event_client_command_t; a handler may set `block` |
 * | ::AUTHUM_EVENT_CLIENT_CVAR_VALUE | `CvarValue2`, the answer to `QueryClientCvarValue2` | ::authum_event_client_cvar_t |
 * | ::AUTHUM_EVENT_CLIENT_KILL | `ClientKill`, the `kill` command | ::authum_event_client_t; set `allow = false` to refuse |
 *
 * `index` is the engine entity index (1 … maxClients), the same number AMXX
 * calls `id`.
 */
// clang-format on

#ifndef AUTHUM_EVENT_CLIENT_H
#define AUTHUM_EVENT_CLIENT_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/float.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define AUTHUM_EVENT_CLIENT_CONNECT "authum.client.connect"
#define AUTHUM_EVENT_CLIENT_PUT_IN_SERVER "authum.client.put_in_server"
#define AUTHUM_EVENT_CLIENT_INFO_CHANGED "authum.client.info_changed"
#define AUTHUM_EVENT_CLIENT_DISCONNECT "authum.client.disconnect"
#define AUTHUM_EVENT_CLIENT_COMMAND "authum.client.command"
#define AUTHUM_EVENT_CLIENT_CVAR_VALUE "authum.client.cvar_value"
#define AUTHUM_EVENT_CLIENT_KILL "authum.client.kill"

    enum
    {
        authum_event_client_name_max = 32,
        authum_event_client_address_max = 64,
        authum_event_client_auth_id_max = 64,
        authum_event_client_reject_max = 128
    };

    struct authum_event_client
    {
        /** sizeof(authum_event_client_t) as built by the host. */
        authum_usize_t size;
        /** Entity index, 1 … maxClients. */
        authum_i32_t index;
        /** Engine userid (`kick #userid`). */
        authum_i32_t userid;
        authum_char_t name[authum_event_client_name_max];
        /** `ip:port` (connect only). */
        authum_char_t address[authum_event_client_address_max];
        /** `STEAM_0:…` / `VALVE_…` / `HLTV` — empty when not known yet. */
        authum_char_t auth_id[authum_event_client_auth_id_max];
        /** Seconds since map start (`gpGlobals->time`). */
        authum_float_t game_time;
        /** Wall clock, seconds since 1970-01-01 UTC. */
        authum_i64_t unix_time;
        /** Connect only: set to false to refuse the connection. */
        authum_bool_t allow;
        /** Connect only: message shown to the refused client. */
        authum_char_t reject_reason[authum_event_client_reject_max];
    };
    typedef struct authum_event_client authum_event_client_t;

    enum
    {
        authum_event_client_command_argv_max = 16,
        authum_event_client_command_arg_max = 128,
        authum_event_client_command_args_max = 512,
        authum_event_client_cvar_name_max = 64,
        authum_event_client_cvar_value_max = 256
    };

    struct authum_event_client_command
    {
        /** sizeof(authum_event_client_command_t) as built by the host. */
        authum_usize_t size;
        /** Who sent it — same fields as the other `authum.client.*` events. */
        authum_event_client_t client;
        /** Arguments the engine parsed, `argv[0]` is the command (`say`). */
        authum_i32_t argc;
        /** First ::authum_event_client_command_argv_max arguments, each cut to fit. */
        authum_char_t argv[authum_event_client_command_argv_max]
                          [authum_event_client_command_arg_max];
        /** Everything after the command as one line (`CMD_ARGS`). */
        authum_char_t args[authum_event_client_command_args_max];
        /** Set to true to swallow the command: the game DLL never sees it. */
        authum_bool_t block;
    };
    typedef struct authum_event_client_command authum_event_client_command_t;

    struct authum_event_client_cvar
    {
        /** sizeof(authum_event_client_cvar_t) as built by the host. */
        authum_usize_t size;
        /** Who answered. */
        authum_event_client_t client;
        /** The `requestID` passed to `QueryClientCvarValue2`. */
        authum_i32_t request_id;
        authum_char_t cvar[authum_event_client_cvar_name_max];
        /** The value, or the engine's `Bad CVAR request` / `Bad Player` text. */
        authum_char_t value[authum_event_client_cvar_value_max];
    };
    typedef struct authum_event_client_cvar authum_event_client_cvar_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_EVENT_CLIENT_H */
