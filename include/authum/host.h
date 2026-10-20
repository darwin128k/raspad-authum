/**
 * @file host.h
 * @brief Host facade passed into ::authum_module_api.start (no lh dependency).
 *
 * Filled by `authum_mm`. Pointers are valid for the life of the loaded module
 * (until `stop`). Escape hatch: link your own lh / other libs privately.
 */

#ifndef AUTHUM_HOST_H
#define AUTHUM_HOST_H

#include <authum/char.h>
#include <authum/crypto.h>
#include <authum/engine.h>
#include <authum/null.h>
#include <authum/ptr.h>

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
     * Prefer ::authum_host.engine for the common subset — no HLSDK required.
     */
    authum_host_get_ptr_fn engine_funcs;
    authum_host_get_ptr_fn globals;
    /**
     * Directory that holds this module's image (e.g. `…/modules/qproto`).
     * NUL-terminated; valid until `stop`. May be null if unknown.
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

static inline void
authum_host_log_line(const authum_host_t *host, int is_error, const authum_char_t *line)
{
    if (!authum_null_eq(host) && host->log != 0 && !authum_null_eq(line)) {
        host->log(is_error, line);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_HOST_H */
