/**
 * @file host.h
 * @brief Module-side bind to services exported by `authum_mm` (option B).
 *
 * Architecture: lh lives **inside the host** as one static copy. Child modules
 * do not share that runtime. What is shared is this small host facade —
 * resolve symbols on the parent image once in `start`.
 *
 * Escape hatch (“hole”): a module may still link its own static lh (or any
 * other library) for private utilities. That copy is not the host’s; do not
 * use `lh_runtime_logger()` expecting `authum.log`.
 */

#ifndef AUTHUM_HOST_H
#define AUTHUM_HOST_H

#include <authum/module.h>

#include <lh/attribute/force_inline.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/null.h>
#include <lh/os/module.h>

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief `authum_host_log` on the host image.
 */
typedef void (*authum_host_log_fn)(int is_error, const char *line);

/**
 * @brief `authum_engine_funcs` / `authum_globals` getters (cast results in the module).
 */
typedef void *(*authum_host_get_ptr_fn)(void);

/**
 * @struct authum_host
 * @brief Bound host entry points. Null function pointers mean “not found”.
 */
typedef struct authum_host
{
    authum_host_log_fn log;
    authum_host_get_ptr_fn engine_funcs;
    authum_host_get_ptr_fn globals;
} authum_host_t;

/**
 * @brief Resolve host symbols from @p self's parent (`authum_mm`).
 *
 * @return ::lh_bool_true if the parent exists (individual slots may still be null).
 */
LH_ATTRIBUTE_FORCE_INLINE lh_bool_t
authum_host_bind(authum_module_t *self, authum_host_t *out)
{
    authum_module_t *parent;

    if (lh_null_eq(out)) {
        return lh_bool_false;
    }
    out->log = lh_null;
    out->engine_funcs = lh_null;
    out->globals = lh_null;

    parent = lh_os_module_get_parent(self);
    if (lh_null_eq(parent)) {
        return lh_bool_false;
    }

    out->log = (authum_host_log_fn)lh_os_module_get_sym(parent, "authum_host_log");
    out->engine_funcs =
        (authum_host_get_ptr_fn)lh_os_module_get_sym(parent, "authum_engine_funcs");
    out->globals = (authum_host_get_ptr_fn)lh_os_module_get_sym(parent, "authum_globals");
    return lh_bool_true;
}

/**
 * @brief Forward one line to the host log, or no-op if unbound.
 */
LH_ATTRIBUTE_FORCE_INLINE void
authum_host_log_line(const authum_host_t *host, int is_error, const char *line)
{
    if (!lh_null_eq(host) && !lh_null_eq(host->log) && !lh_null_eq(line)) {
        host->log(is_error, line);
    }
}

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_HOST_H */
