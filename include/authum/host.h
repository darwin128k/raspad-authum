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
#include <authum/crypto.h>
#include <authum/engine.h>
#include <authum/env.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
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
} authum_host_t;

/* --- getters (modules: only these) --------------------------------------- */

static inline authum_host_log_fn
authum_host_get_log(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->log : 0;
}

static inline authum_host_get_ptr_fn
authum_host_engine_funcs(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->engine_funcs : 0;
}

static inline authum_host_get_ptr_fn
authum_host_globals(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->globals : 0;
}

static inline const authum_env_t *
authum_host_env(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->env : authum_null;
}

static inline const authum_crypto_t *
authum_host_crypto(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->crypto : authum_null;
}

static inline const authum_engine_t *
authum_host_engine(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->engine : authum_null;
}

static inline const authum_char_t *
authum_host_module_dir(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->module_dir : authum_null;
}

static inline const authum_path_t *
authum_host_env_module(const authum_host_t *host)
{
    return authum_env_module(authum_host_env(host));
}

static inline const authum_path_t *
authum_host_env_game(const authum_host_t *host)
{
    return authum_env_game(authum_host_env(host));
}

static inline const authum_char_t *
authum_host_env_module_cstr(const authum_host_t *host)
{
    return authum_env_module_cstr(authum_host_env(host));
}

/* --- setters (module-owned copies, e.g. clear on stop) ------------------- */

static inline void
authum_host_set_log(authum_host_t *host, authum_host_log_fn log)
{
    if (!authum_null_eq(host)) {
        host->log = log;
    }
}

static inline void
authum_host_set_engine_funcs(authum_host_t *host, authum_host_get_ptr_fn fn)
{
    if (!authum_null_eq(host)) {
        host->engine_funcs = fn;
    }
}

static inline void
authum_host_set_globals(authum_host_t *host, authum_host_get_ptr_fn fn)
{
    if (!authum_null_eq(host)) {
        host->globals = fn;
    }
}

static inline void
authum_host_set_env(authum_host_t *host, const authum_env_t *env)
{
    if (!authum_null_eq(host)) {
        host->env = env;
    }
}

static inline void
authum_host_set_module_dir(authum_host_t *host, const authum_char_t *module_dir)
{
    if (!authum_null_eq(host)) {
        host->module_dir = module_dir;
    }
}

static inline void
authum_host_set_crypto(authum_host_t *host, const authum_crypto_t *crypto)
{
    if (!authum_null_eq(host)) {
        host->crypto = crypto;
    }
}

static inline void
authum_host_set_engine(authum_host_t *host, const authum_engine_t *engine)
{
    if (!authum_null_eq(host)) {
        host->engine = engine;
    }
}

/** Clear a module-owned host copy after `stop` (does not free host memory). */
static inline void
authum_host_clear(authum_host_t *host)
{
    if (authum_null_eq(host)) {
        return;
    }
    host->log = 0;
    host->engine_funcs = 0;
    host->globals = 0;
    host->env = authum_null;
    host->module_dir = authum_null;
    host->crypto = authum_null;
    host->engine = authum_null;
}

/* --- engine ops via host (no `host->engine` at call sites) ---------------- */

static inline void
authum_host_get_game_dir(const authum_host_t *host, authum_char_t *buf, authum_usize_t buf_size)
{
    authum_engine_get_game_dir(authum_host_engine(host), buf, buf_size);
}

static inline void
authum_host_server_print(const authum_host_t *host, const authum_char_t *text)
{
    authum_engine_server_print(authum_host_engine(host), text);
}

static inline void
authum_host_server_command(const authum_host_t *host, const authum_char_t *cmd)
{
    authum_engine_server_command(authum_host_engine(host), cmd);
}

static inline void
authum_host_log_line(const authum_host_t *host, int is_error, const authum_char_t *line)
{
    authum_host_log_fn log = authum_host_get_log(host);
    if (log != 0 && !authum_null_eq(line)) {
        log(is_error, line);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_HOST_H */
