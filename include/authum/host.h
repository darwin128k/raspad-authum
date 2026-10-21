/**
 * @file host.h
 * @brief Host facade passed into ::authum_module_api.start (no lh dependency).
 *
 * Filled by `authum_mm`. Pointers are valid for the life of the loaded module
 * (until `stop`). Escape hatch: link your own lh / other libs privately.
 *
 * Modules must not read or write ::authum_host_t fields directly — use the
 * getters/setters below (and ::authum_host_get_game_dir / `_server_print` /
 * `_server_command` for engine ops).
 */

#ifndef AUTHUM_HOST_H
#define AUTHUM_HOST_H

#include <authum/char.h>
#include <authum/commands.h>
#include <authum/crypto.h>
#include <authum/engine.h>
#include <authum/env.h>
#include <authum/events.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/sandbox.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*authum_host_log_fn)(int is_error, const authum_char_t *line);
    typedef authum_ptr (*authum_host_get_ptr_fn)(void);

    typedef struct authum_host
    {
        authum_host_log_fn log;
        /**
         * Escape hatch: `enginefuncs_t *` / `globalvars_t *` (cast in the module).
         * Prefer ::authum_host_engine for the common subset — no HLSDK required.
         */
        authum_host_get_ptr_fn engine_funcs;
        authum_host_get_ptr_fn globals;
        /**
         * Well-known directories (game / addons / models / this module / …).
         * Non-null on current hosts; individual fields may be null if unknown.
         */
        const authum_env_t *env;
        /**
         * @deprecated Prefer ::authum_host_env_module_cstr.
         */
        const authum_char_t *module_dir;
        /**
         * Crypto ops (Rijndael + SHA subset from lh). Non-null on current hosts;
         * check before use if you support older `authum_mm`.
         */
        const authum_crypto_t *crypto;
        /**
         * Engine ops (game dir / server print / server command). Non-null on
         * current hosts; check before use if you support older `authum_mm`.
         */
        const authum_engine_t *engine;
        /** This module's memory sandbox; freed by the host after `stop`. */
        const authum_sandbox_t *sandbox;
        /** Event bus handle bound to this module (subscriptions dropped after `stop`). */
        const authum_events_t *events;
        /** `authum <module> <name>` console / RCON commands (dropped after `stop`). */
        const authum_commands_t *commands;
    } authum_host_t;

    /* --- getters (modules: only these) --------------------------------------- */

    authum_host_log_fn
    authum_host_get_log(const authum_host_t *host);

    authum_host_get_ptr_fn
    authum_host_engine_funcs(const authum_host_t *host);

    authum_host_get_ptr_fn
    authum_host_globals(const authum_host_t *host);

    const authum_env_t *
    authum_host_env(const authum_host_t *host);

    const authum_crypto_t *
    authum_host_crypto(const authum_host_t *host);

    const authum_engine_t *
    authum_host_engine(const authum_host_t *host);

    const authum_char_t *
    authum_host_module_dir(const authum_host_t *host);

    const authum_sandbox_t *
    authum_host_sandbox(const authum_host_t *host);

    const authum_events_t *
    authum_host_events(const authum_host_t *host);

    const authum_commands_t *
    authum_host_commands(const authum_host_t *host);

    const authum_path_t *
    authum_host_env_module(const authum_host_t *host);

    const authum_path_t *
    authum_host_env_game(const authum_host_t *host);

    const authum_char_t *
    authum_host_env_module_cstr(const authum_host_t *host);

    /* --- setters (module-owned copies, e.g. clear on stop) ------------------- */

    void
    authum_host_set_log(authum_host_t *host, authum_host_log_fn log);

    void
    authum_host_set_engine_funcs(authum_host_t *host, authum_host_get_ptr_fn fn);

    void
    authum_host_set_globals(authum_host_t *host, authum_host_get_ptr_fn fn);

    void
    authum_host_set_env(authum_host_t *host, const authum_env_t *env);

    void
    authum_host_set_module_dir(authum_host_t *host, const authum_char_t *module_dir);

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

    /* --- engine ops via host (no `host->engine` at call sites) ---------------- */

    void
    authum_host_get_game_dir(const authum_host_t *host, authum_char_t *buf,
                             authum_usize_t buf_size);

    void
    authum_host_server_print(const authum_host_t *host, const authum_char_t *text);

    void
    authum_host_server_command(const authum_host_t *host, const authum_char_t *cmd);

    /** `kick #userid "reason"` (reason sanitized; empty → plain kick). */
    void
    authum_host_kick(const authum_host_t *host, authum_i32_t userid, const authum_char_t *reason);

    void
    authum_host_log_line(const authum_host_t *host, int is_error, const authum_char_t *line);

    /* --- bus shortcuts (same as authum_events_* on ::authum_host_events) ------ */

    authum_bool_t
    authum_host_subscribe(const authum_host_t *host, const authum_char_t *name,
                          authum_event_handler_fn fn, authum_ptr user);

    authum_event_result_t
    authum_host_publish(const authum_host_t *host, const authum_char_t *name, authum_ptr payload);

    /* --- command shortcuts (same as authum_commands_* on ::authum_host_commands) */

    authum_bool_t
    authum_host_command_add(const authum_host_t *host, const authum_command_def_t *def);

    authum_bool_t
    authum_host_command_add_all(const authum_host_t *host, const authum_command_def_t *defs,
                                authum_usize_t count);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_HOST_H */
