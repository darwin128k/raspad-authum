/**
 * @file host.h
 * @brief Host facade passed into ::authum_module_api.start (no lh dependency).
 *
 * Filled by `authum`. Pointers are valid for the life of the loaded module
 * (until `stop`). Escape hatch: link your own lh / other libs privately.
 *
 * Modules must not read or write ::authum_host_t fields directly — use the
 * getters/setters below (and ::authum_host_copy_game_dir / `_server_print` /
 * `_server_command` for engine ops).
 */

#ifndef AUTHUM_HOST_H
#define AUTHUM_HOST_H

#include <authum/commands.h>
#include <authum/crypto.h>
#include <authum/engine.h>
#include <authum/env.h>
#include <authum/events.h>
#include <authum/logger/ops.h>
#include <authum/sandbox.h>
#include <authum/task.h>
#include <authum/thread.h>
#include <authum/http.h>
#include <authum/clock.h>
#include <authum/placeholders.h>

AUTHUM_EXTERN_C_BEGIN

typedef authum_ptr (*authum_host_get_ptr_fn)(void);

AUTHUM_EXTERN_C_END

typedef struct authum_host
{
    /**
     * This module's log channel: the host files each line under
     * `modules/<module>/logs/` per `authum.cfg`. Use <authum/logger.h>.
     */
    const authum_logger_channel_t *logger;
    /**
     * Escape hatch: `enginefuncs_t *` / `globalvars_t *` (cast in the module).
     * Prefer ::authum_host_get_engine for the common subset — no HLSDK required.
     */
    authum_host_get_ptr_fn engine_funcs;
    authum_host_get_ptr_fn globals;
    /**
     * Well-known directories (game / addons / models / this module / …).
     * Non-null on current hosts; individual fields may be null if unknown.
     */
    const authum_env_t *env;
    /**
     * Crypto ops (Rijndael + SHA subset from lh). Non-null on current hosts;
     * check before use if you support older `authum`.
     */
    const authum_crypto_t *crypto;
    /**
     * Engine ops (game dir / server print / server command). Non-null on
     * current hosts; check before use if you support older `authum`.
     */
    const authum_engine_t *engine;
    /** This module's memory sandbox; freed by the host after `stop`. */
    const authum_sandbox_t *sandbox;
    /** Event bus handle bound to this module (subscriptions dropped after `stop`). */
    const authum_events_t *events;
    /** `authum <module> <name>` console / RCON commands (dropped after `stop`). */
    const authum_commands_t *commands;
    /** Owner-bound background jobs, completed on server frames. */
    const authum_tasks_t *tasks;
    /** Managed long-lived workers, stopped and joined before module stop. */
    const authum_threads_t *threads;
    const authum_http_service_t *http;
    const authum_clock_t *clock;
    /** Path placeholders: standard names and those modules registered. */
    const authum_placeholder_service_t *placeholders;
    /**
     * The module's mark: how it is called in the logs and in what it prints.
     * Null on a host that is not a module.
     */
    const authum_char_t *tag;
} authum_host_t;

AUTHUM_EXTERN_C_BEGIN

/* --- getters (modules: only these) --------------------------------------- */
const authum_http_service_t *
authum_host_get_http(const authum_host_t *host);

const authum_clock_t *
authum_host_get_clock(const authum_host_t *host);

/** The placeholder service; null on hosts older than ABI 1.3. */
const authum_placeholder_service_t *
authum_host_get_placeholders(const authum_host_t *host);

const authum_threads_t *
authum_host_get_threads(const authum_host_t *host);

const authum_logger_channel_t *
authum_host_get_logger(const authum_host_t *host);

authum_host_get_ptr_fn
authum_host_get_engine_funcs(const authum_host_t *host);

authum_host_get_ptr_fn
authum_host_get_globals(const authum_host_t *host);

const authum_env_t *
authum_host_get_env(const authum_host_t *host);

const authum_crypto_t *
authum_host_get_crypto(const authum_host_t *host);

const authum_engine_t *
authum_host_get_engine(const authum_host_t *host);

/**
 * @brief The module's mark: how it is called in the logs and in what it prints.
 * @return Null when the host is not a module, or the module named no mark.
 */
const authum_char_t *
authum_host_get_tag(const authum_host_t *host);

const authum_sandbox_t *
authum_host_get_sandbox(const authum_host_t *host);

const authum_events_t *
authum_host_get_events(const authum_host_t *host);

const authum_commands_t *
authum_host_get_commands(const authum_host_t *host);

const authum_tasks_t *
authum_host_get_tasks(const authum_host_t *host);

const authum_path_t *
authum_host_get_game_path(const authum_host_t *host);

/** `modules/<module>`: this module's folder. */
const authum_path_t *
authum_host_get_module_path(const authum_host_t *host);

/** `modules/<module>/configs`. */
const authum_path_t *
authum_host_get_module_config_path(const authum_host_t *host);

/** `modules/<module>/lang`: translations. */
const authum_path_t *
authum_host_get_module_lang_path(const authum_host_t *host);

/** `modules/<module>/data`: what the module writes. */
const authum_path_t *
authum_host_get_module_data_path(const authum_host_t *host);

/** `modules/<module>/configs/<leaf>` — see ::authum_env_join_config_path. True when it exists. */
authum_bool_t
authum_host_join_config_path(const authum_host_t *host, const authum_char_t *leaf,
                             authum_path_t *out);

/** `modules/<module>/data/<leaf>`, path only — see ::authum_env_join_data_path. */
authum_bool_t
authum_host_join_data_path(const authum_host_t *host, const authum_char_t *leaf,
                           authum_path_t *out);

/* --- setters (module-owned copies, e.g. clear on stop) ------------------- */

void
authum_host_set_logger(authum_host_t *host, const authum_logger_channel_t *logger);

void
authum_host_set_engine_funcs(authum_host_t *host, authum_host_get_ptr_fn fn);

void
authum_host_set_globals(authum_host_t *host, authum_host_get_ptr_fn fn);

void
authum_host_set_env(authum_host_t *host, const authum_env_t *env);

void
authum_host_set_crypto(authum_host_t *host, const authum_crypto_t *crypto);

void
authum_host_set_engine(authum_host_t *host, const authum_engine_t *engine);

void
authum_host_set_sandbox(authum_host_t *host, const authum_sandbox_t *sandbox);

void
authum_host_set_events(authum_host_t *host, const authum_events_t *events);

void
authum_host_set_commands(authum_host_t *host, const authum_commands_t *commands);

/** Clear a module-owned host copy after `stop` (does not free host memory). */
void
authum_host_clear(authum_host_t *host);

/**
 * Keep the host passed to `start`: copies the whole table; a null @p src
 * leaves @p dst zeroed.
 */
void
authum_host_copy(authum_host_t *dst, const authum_host_t *src);

/* --- engine ops via host (no `host->engine` at call sites) ---------------- */

void
authum_host_copy_game_dir(const authum_host_t *host, authum_char_t *buf, authum_usize_t buf_size);

void
authum_host_server_print(const authum_host_t *host, const authum_char_t *text);

void
authum_host_server_command(const authum_host_t *host, const authum_char_t *cmd);

/** `kick #userid "reason"` (reason sanitized; empty → plain kick). */
void
authum_host_kick(const authum_host_t *host, authum_i32_t userid, const authum_char_t *reason);

/* Same as the `authum_engine_*` of the same name on ::authum_host_get_engine. */

authum_i32_t
authum_host_get_max_clients(const authum_host_t *host);

authum_float_t
authum_host_get_game_time(const authum_host_t *host);

void
authum_host_copy_map_name(const authum_host_t *host, authum_char_t *buf, authum_usize_t buf_size);

authum_bool_t
authum_host_cvar_copy(const authum_host_t *host, const authum_char_t *name, authum_char_t *buf,
                      authum_usize_t buf_size);

authum_bool_t
authum_host_cvar_set(const authum_host_t *host, const authum_char_t *name,
                     const authum_char_t *value);

authum_bool_t
authum_host_client_is_connected(const authum_host_t *host, authum_i32_t index);

authum_i32_t
authum_host_client_get_userid(const authum_host_t *host, authum_i32_t index);

authum_i32_t
authum_host_client_get_index_by_userid(const authum_host_t *host, authum_i32_t userid);

authum_bool_t
authum_host_client_copy_name(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                             authum_usize_t buf_size);

authum_bool_t
authum_host_client_copy_auth_id(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                                authum_usize_t buf_size);

authum_bool_t
authum_host_client_copy_address(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                                authum_usize_t buf_size);

authum_bool_t
authum_host_client_copy_info(const authum_host_t *host, authum_i32_t index,
                             const authum_char_t *key, authum_char_t *buf, authum_usize_t buf_size);

authum_bool_t
authum_host_client_print(const authum_host_t *host, authum_i32_t index, authum_i32_t kind,
                         const authum_char_t *text);

authum_bool_t
authum_host_client_command(const authum_host_t *host, authum_i32_t index, const authum_char_t *cmd);

authum_bool_t
authum_host_client_try_get_stats(const authum_host_t *host, authum_i32_t index, authum_i32_t *ping,
                                 authum_i32_t *loss);

authum_bool_t
authum_host_client_set_info(const authum_host_t *host, authum_i32_t index, const authum_char_t *key,
                            const authum_char_t *value);

authum_bool_t
authum_host_client_chat(const authum_host_t *host, authum_i32_t index, authum_i32_t sender,
                        const authum_char_t *text);

authum_bool_t
authum_host_cvar_register(const authum_host_t *host, const authum_char_t *name,
                          const authum_char_t *value, authum_i32_t flags);

authum_i32_t
authum_host_cvar_get_i32(const authum_host_t *host, const authum_char_t *name,
                         authum_i32_t fallback);

authum_float_t
authum_host_cvar_get_float(const authum_host_t *host, const authum_char_t *name,
                           authum_float_t fallback);

authum_i32_t
authum_host_message_get_id(const authum_host_t *host, const authum_char_t *name);

authum_bool_t
authum_host_message_begin(const authum_host_t *host, authum_i32_t dest, authum_i32_t msg_id,
                          authum_i32_t index);

void
authum_host_message_write_byte(const authum_host_t *host, authum_i32_t value);

void
authum_host_message_write_char(const authum_host_t *host, authum_i32_t value);

void
authum_host_message_write_short(const authum_host_t *host, authum_i32_t value);

void
authum_host_message_write_long(const authum_host_t *host, authum_i32_t value);

void
authum_host_message_write_entity(const authum_host_t *host, authum_i32_t index);

void
authum_host_message_write_angle(const authum_host_t *host, authum_float_t value);

void
authum_host_message_write_coord(const authum_host_t *host, authum_float_t value);

void
authum_host_message_write_string(const authum_host_t *host, const authum_char_t *text);

void
authum_host_message_end(const authum_host_t *host);

authum_i32_t
authum_host_precache_model(const authum_host_t *host, const authum_char_t *path);

authum_i32_t
authum_host_precache_sound(const authum_host_t *host, const authum_char_t *path);

authum_i32_t
authum_host_precache_generic(const authum_host_t *host, const authum_char_t *path);

authum_i32_t
authum_host_entity_get_max(const authum_host_t *host);

authum_bool_t
authum_host_entity_is_valid(const authum_host_t *host, authum_i32_t index);

authum_bool_t
authum_host_entity_copy_classname(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                                  authum_usize_t buf_size);

authum_i32_t
authum_host_entity_find_by_classname(const authum_host_t *host, authum_i32_t start,
                                     const authum_char_t *classname);

authum_bool_t
authum_host_entity_try_get_float(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                                 authum_float_t *out);

authum_bool_t
authum_host_entity_set_float(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                             authum_float_t value);

authum_bool_t
authum_host_entity_try_get_i32(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                               authum_i32_t *out);

authum_bool_t
authum_host_entity_set_i32(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                           authum_i32_t value);

authum_bool_t
authum_host_entity_copy_vector(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                               authum_float_t *out);

authum_bool_t
authum_host_entity_set_vector(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                              const authum_float_t *value);

authum_bool_t
authum_host_client_is_alive(const authum_host_t *host, authum_i32_t index);

authum_bool_t
authum_host_client_is_bot(const authum_host_t *host, authum_i32_t index);

/** Ask a client for a cvar; see ::authum_engine_query_cvar. */
authum_bool_t
authum_host_query_cvar(const authum_host_t *host, authum_i32_t index, const authum_char_t *cvar,
                       authum_i32_t request_id);

void
authum_host_log_line(const authum_host_t *host, authum_bool_t is_error, const authum_char_t *line);

/* --- bus shortcuts (same as authum_events_* on ::authum_host_get_events) ------ */

authum_bool_t
authum_host_subscribe(const authum_host_t *host, const authum_char_t *name,
                      authum_event_handler_fn fn, authum_ptr user);

authum_event_result_t
authum_host_publish(const authum_host_t *host, const authum_char_t *name, authum_ptr payload);

/* --- command shortcuts (same as authum_commands_* on ::authum_host_get_commands) */

authum_bool_t
authum_host_command_add(const authum_host_t *host, const authum_command_def_t *def);

authum_bool_t
authum_host_command_add_all(const authum_host_t *host, const authum_command_def_t *defs,
                            authum_usize_t count);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_H */
