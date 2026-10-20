/**
 * @file client.h
 * @brief Player lifecycle events published by the host (`authum.client.*`).
 *
 * Authum hooks the game DLL calls (via Metamod) once and forwards them to
 * modules, so no module needs its own Metamod hooks:
 *
 * | Event | When | Payload fields in use |
 * |-------|------|------------------------|
 * | ::AUTHUM_EVENT_CLIENT_CONNECT | `ClientConnect` (after the engine auth step) | all; a handler may set `allow = false` + `reject_reason` |
 * | ::AUTHUM_EVENT_CLIENT_PUT_IN_SERVER | `ClientPutInServer` | index, userid, name, auth_id, times |
 * | ::AUTHUM_EVENT_CLIENT_INFO_CHANGED | `ClientUserInfoChanged` | index, userid, name, times |
 * | ::AUTHUM_EVENT_CLIENT_DISCONNECT | `ClientDisconnect` | index, userid, name, auth_id, times |
 *
 * `index` is the engine entity index (1 … maxClients), the same number AMXX
 * calls `id`.
 */

#ifndef AUTHUM_EVENT_CLIENT_H
#define AUTHUM_EVENT_CLIENT_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUTHUM_EVENT_CLIENT_CONNECT "authum.client.connect"
#define AUTHUM_EVENT_CLIENT_PUT_IN_SERVER "authum.client.put_in_server"
#define AUTHUM_EVENT_CLIENT_INFO_CHANGED "authum.client.info_changed"
#define AUTHUM_EVENT_CLIENT_DISCONNECT "authum.client.disconnect"

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
    float game_time;
    /** Wall clock, seconds since 1970-01-01 UTC. */
    authum_i64_t unix_time;
    /** Connect only: set to false to refuse the connection. */
    authum_bool_t allow;
    /** Connect only: message shown to the refused client. */
    authum_char_t reject_reason[authum_event_client_reject_max];
};
typedef struct authum_event_client authum_event_client_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_EVENT_CLIENT_H */
