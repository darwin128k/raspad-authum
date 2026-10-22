/**
 * @file engine.h
 * @brief Host engine facade for modules — the engfuncs modules need, no HLSDK.
 *
 * Full `enginefuncs_t` / `globalvars_t` stay inside `authum_mm`. Modules go
 * through ::authum_engine_t on ::authum_host_t.engine (or the `authum_host_*`
 * shortcuts). Raw pointers remain on the host as an escape hatch
 * (`engine_funcs` / `globals`) for modules that link HLSDK privately — every
 * call that goes through here instead is one less thing an ABI change breaks.
 *
 * Players are addressed by entity `index` (1 … maxClients), the number every
 * `authum.client.*` event carries. A function given an index that is not a
 * connected player does nothing and returns false / 0.
 *
 * Wrappers check the table with ::AUTHUM_ABI_HAS: on a host that predates a
 * function they return false / 0 / an empty string instead of calling it.
 */

#ifndef AUTHUM_ENGINE_H
#define AUTHUM_ENGINE_H

#include <authum/abi.h>
#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/engine/message.h>
#include <authum/float.h>
#include <authum/null.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Where ::authum_engine_client_print puts the text. */
    enum
    {
        /** The player's console. */
        authum_engine_print_console = 0,
        /** Middle of the screen. */
        authum_engine_print_center = 1,
        /** Chat / notify area. */
        authum_engine_print_chat = 2
    };

    /**
     * @struct authum_engine
     * @brief Ops table filled by the host from HLDS engfuncs. Append only.
     */
    typedef struct authum_engine
    {
        /** sizeof this table as built by the host — see <authum/abi.h>. */
        authum_usize_t size;

        /* --- server ------------------------------------------------------ */

        /** Game directory (e.g. `cstrike`); always NUL-terminated. */
        void (*get_game_dir)(authum_char_t *buf, authum_usize_t buf_size);
        /** `pfnServerPrint` (text as-is, typically already `\n`-terminated). */
        void (*server_print)(const authum_char_t *text);
        /** `pfnServerCommand` (e.g. `kick #3\n`). */
        void (*server_command)(const authum_char_t *cmd);
        /** `maxplayers` of the current map. */
        authum_i32_t (*max_clients)(void);
        /** Seconds since map start (`gpGlobals->time`). */
        authum_float_t (*game_time)(void);
        /** Current map (`de_dust2`); always NUL-terminated. */
        void (*get_map_name)(authum_char_t *buf, authum_usize_t buf_size);
        /** String value of server cvar @p name; false when there is no such cvar. */
        authum_bool_t (*cvar_get)(const authum_char_t *name, authum_char_t *buf,
                                  authum_usize_t buf_size);
        /** Set server cvar @p name; false when there is no such cvar. */
        authum_bool_t (*cvar_set)(const authum_char_t *name, const authum_char_t *value);

        /* --- players ----------------------------------------------------- */

        /** True when slot @p index holds a connected (or connecting) player. */
        authum_bool_t (*client_is_connected)(authum_i32_t index);
        /** Engine userid (`kick #userid`), or 0. */
        authum_i32_t (*client_userid)(authum_i32_t index);
        /** Entity index of @p userid, or 0. */
        authum_i32_t (*client_index_by_userid)(authum_i32_t userid);
        /** Player name. */
        authum_bool_t (*client_get_name)(authum_i32_t index, authum_char_t *buf,
                                         authum_usize_t buf_size);
        /** `STEAM_0:…` / `VALVE_…` / `HLTV`; empty while not authorized yet. */
        authum_bool_t (*client_get_auth_id)(authum_i32_t index, authum_char_t *buf,
                                            authum_usize_t buf_size);
        /** `ip:port` the player connected from. */
        authum_bool_t (*client_get_address)(authum_i32_t index, authum_char_t *buf,
                                            authum_usize_t buf_size);
        /** Value of @p key in the player's userinfo (`setinfo`), e.g. `_pw`, `*hltv`. */
        authum_bool_t (*client_get_info)(authum_i32_t index, const authum_char_t *key,
                                         authum_char_t *buf, authum_usize_t buf_size);
        /**
         * Show @p text to player @p index (0 = every player) as
         * `authum_engine_print_*` @p kind. Add the `\n` yourself.
         */
        authum_bool_t (*client_print)(authum_i32_t index, authum_i32_t kind,
                                      const authum_char_t *text);
        /** Run @p cmd in the player's own console (a `\n` is added). */
        authum_bool_t (*client_command)(authum_i32_t index, const authum_char_t *cmd);
        /**
         * `pfnQueryClientCvarValue2`: the answer arrives as
         * ::AUTHUM_EVENT_CLIENT_CVAR_VALUE carrying @p request_id.
         */
        authum_bool_t (*query_client_cvar)(authum_i32_t index, const authum_char_t *cvar,
                                           authum_i32_t request_id);
        /** Ping (ms) and packet loss (%) of the player. */
        authum_bool_t (*client_get_stats)(authum_i32_t index, authum_i32_t *ping,
                                          authum_i32_t *loss);
        /** Set @p key in the player's userinfo (`setinfo`). */
        authum_bool_t (*client_set_info)(authum_i32_t index, const authum_char_t *key,
                                         const authum_char_t *value);

        /* --- cvars and messages ------------------------------------------ */

        /**
         * Create server cvar @p name with @p value and `authum_engine_cvar_*`
         * @p flags; true also when it already exists (it is left as it is).
         * The engine keeps it until the server quits — module reloads included.
         */
        authum_bool_t (*cvar_register)(const authum_char_t *name, const authum_char_t *value,
                                       authum_i32_t flags);
        /** Id of user message @p name (`SayText`); 0 when the game has none (yet). */
        authum_i32_t (*message_id)(const authum_char_t *name);
        /**
         * Start user message @p msg_id to `authum_message_dest_*` @p dest; @p index
         * is the player for the `_one` destinations, else 0.
         */
        authum_bool_t (*message_begin)(authum_i32_t dest, authum_i32_t msg_id, authum_i32_t index);
        /** One argument: `authum_message_arg_*` @p kind takes @p value, @p real or @p text. */
        void (*message_write)(authum_i32_t kind, authum_i32_t value, authum_float_t real,
                              const authum_char_t *text);
        void (*message_end)(void);

        /* --- precache (only during authum.server.precache) ---------------- */

        /**
         * Precache `models/…` / `sound/…` (relative to `sound/`) / any file for
         * download. Returns the engine index, or 0 outside the precache window
         * (the engine would stop the server) or on failure.
         */
        authum_i32_t (*precache_model)(const authum_char_t *path);
        authum_i32_t (*precache_sound)(const authum_char_t *path);
        authum_i32_t (*precache_generic)(const authum_char_t *path);
    } authum_engine_t;

    /** Flags for ::authum_engine_cvar_register (the engine's FCVAR_*). */
    enum
    {
        /** Saved in config.cfg (client-side meaning; harmless on a server). */
        authum_engine_cvar_archive = 1,
        /** Listed in the server's rules (`rules` query, server browsers). */
        authum_engine_cvar_server = 4,
        /** The value is hidden in the rules (passwords). */
        authum_engine_cvar_protected = 32,
        /** Changing it is not written to the log. */
        authum_engine_cvar_unlogged = 256
    };

    /* --- server ------------------------------------------------------------ */

    void
    authum_engine_get_game_dir(const authum_engine_t *engine, authum_char_t *buf,
                               authum_usize_t buf_size);

    void
    authum_engine_server_print(const authum_engine_t *engine, const authum_char_t *text);

    void
    authum_engine_server_command(const authum_engine_t *engine, const authum_char_t *cmd);

    authum_i32_t
    authum_engine_max_clients(const authum_engine_t *engine);

    authum_float_t
    authum_engine_game_time(const authum_engine_t *engine);

    void
    authum_engine_get_map_name(const authum_engine_t *engine, authum_char_t *buf,
                               authum_usize_t buf_size);

    authum_bool_t
    authum_engine_cvar_get(const authum_engine_t *engine, const authum_char_t *name,
                           authum_char_t *buf, authum_usize_t buf_size);

    authum_bool_t
    authum_engine_cvar_set(const authum_engine_t *engine, const authum_char_t *name,
                           const authum_char_t *value);

    /**
     * Issue `kick #userid "reason"`. Quotes, `;` and line breaks in @p reason are
     * replaced with spaces so the command cannot be split or injected.
     */
    void
    authum_engine_kick(const authum_engine_t *engine, authum_i32_t userid,
                       const authum_char_t *reason);

    /* --- players ----------------------------------------------------------- */

    authum_bool_t
    authum_engine_client_is_connected(const authum_engine_t *engine, authum_i32_t index);

    authum_i32_t
    authum_engine_client_userid(const authum_engine_t *engine, authum_i32_t index);

    authum_i32_t
    authum_engine_client_index_by_userid(const authum_engine_t *engine, authum_i32_t userid);

    authum_bool_t
    authum_engine_client_get_name(const authum_engine_t *engine, authum_i32_t index,
                                  authum_char_t *buf, authum_usize_t buf_size);

    authum_bool_t
    authum_engine_client_get_auth_id(const authum_engine_t *engine, authum_i32_t index,
                                     authum_char_t *buf, authum_usize_t buf_size);

    authum_bool_t
    authum_engine_client_get_address(const authum_engine_t *engine, authum_i32_t index,
                                     authum_char_t *buf, authum_usize_t buf_size);

    authum_bool_t
    authum_engine_client_get_info(const authum_engine_t *engine, authum_i32_t index,
                                  const authum_char_t *key, authum_char_t *buf,
                                  authum_usize_t buf_size);

    authum_bool_t
    authum_engine_client_print(const authum_engine_t *engine, authum_i32_t index, authum_i32_t kind,
                               const authum_char_t *text);

    authum_bool_t
    authum_engine_client_command(const authum_engine_t *engine, authum_i32_t index,
                                 const authum_char_t *cmd);

    /**
     * Ask the client at entity @p index for the value of @p cvar. The answer is
     * published as ::AUTHUM_EVENT_CLIENT_CVAR_VALUE carrying @p request_id.
     * Returns false when nothing was sent (no engine, empty @p cvar, bad index).
     */
    authum_bool_t
    authum_engine_query_cvar(const authum_engine_t *engine, authum_i32_t index,
                             const authum_char_t *cvar, authum_i32_t request_id);

    /** Ping (ms) and loss (%); both 0 and false when the player is not there. */
    authum_bool_t
    authum_engine_client_get_stats(const authum_engine_t *engine, authum_i32_t index,
                                   authum_i32_t *ping, authum_i32_t *loss);

    authum_bool_t
    authum_engine_client_set_info(const authum_engine_t *engine, authum_i32_t index,
                                  const authum_char_t *key, const authum_char_t *value);

    /**
     * Colored chat line for player @p index (0 = everyone) through the game's
     * `SayText`. In Counter-Strike `\x01` is the default color, `\x04` green
     * and `\x03` the team color of player @p sender (0 = grey). False when
     * the game has no `SayText` or nobody received it.
     */
    authum_bool_t
    authum_engine_client_chat(const authum_engine_t *engine, authum_i32_t index,
                              authum_i32_t sender, const authum_char_t *text);

    /* --- cvars ------------------------------------------------------------- */

    authum_bool_t
    authum_engine_cvar_register(const authum_engine_t *engine, const authum_char_t *name,
                                const authum_char_t *value, authum_i32_t flags);

    /** Server cvar @p name as an integer; @p fallback when missing or not a number. */
    authum_i32_t
    authum_engine_cvar_get_i32(const authum_engine_t *engine, const authum_char_t *name,
                               authum_i32_t fallback);

    /** Server cvar @p name as a number (`0.5`); @p fallback when missing or not a number. */
    authum_float_t
    authum_engine_cvar_get_float(const authum_engine_t *engine, const authum_char_t *name,
                                 authum_float_t fallback);

    /* --- user messages (<authum/engine/message.h>) ------------------------- */

    authum_i32_t
    authum_engine_message_id(const authum_engine_t *engine, const authum_char_t *name);

    /** Every _write_* and _end after a false begin does nothing. */
    authum_bool_t
    authum_engine_message_begin(const authum_engine_t *engine, authum_i32_t dest,
                                authum_i32_t msg_id, authum_i32_t index);

    void
    authum_engine_message_write_byte(const authum_engine_t *engine, authum_i32_t value);

    void
    authum_engine_message_write_char(const authum_engine_t *engine, authum_i32_t value);

    void
    authum_engine_message_write_short(const authum_engine_t *engine, authum_i32_t value);

    void
    authum_engine_message_write_long(const authum_engine_t *engine, authum_i32_t value);

    void
    authum_engine_message_write_entity(const authum_engine_t *engine, authum_i32_t index);

    void
    authum_engine_message_write_angle(const authum_engine_t *engine, authum_float_t value);

    void
    authum_engine_message_write_coord(const authum_engine_t *engine, authum_float_t value);

    void
    authum_engine_message_write_string(const authum_engine_t *engine, const authum_char_t *text);

    void
    authum_engine_message_end(const authum_engine_t *engine);

    /* --- precache: call from an authum.server.precache handler ------------- */

    authum_i32_t
    authum_engine_precache_model(const authum_engine_t *engine, const authum_char_t *path);

    authum_i32_t
    authum_engine_precache_sound(const authum_engine_t *engine, const authum_char_t *path);

    authum_i32_t
    authum_engine_precache_generic(const authum_engine_t *engine, const authum_char_t *path);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENGINE_H */
