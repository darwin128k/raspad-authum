/**
 * @file placeholders.h
 * @brief Host side of the placeholder registry — implemented in `authum` only.
 *
 * Modules use <authum/placeholders.h>. The loader binds each module's
 * ::authum_placeholder_service_t to these ops with the module as owner and
 * drops the owner's registrations after its `stop`.
 *
 * Two kinds of name:
 *
 * - Standard names (`game`, `module`, `data`, `config`, `lang`, …) come from
 *   the host and describe the module being served: every module sees its own
 *   folder under the same name. They are reserved — a module cannot register
 *   them.
 * - Registered names are host-wide and shared. A module registers one and
 *   every module may use it; this is how modules talk to each other without
 *   knowing each other. Names are unique: a taken name is refused rather
 *   than stolen.
 *
 * Values are absolute paths, canonicalized (symlinks resolved) so that a
 * folder may be published as a source root: the HTTP server opens a source
 * root without following a symlink at its end.
 */

#ifndef AUTHUM_HOST_PLACEHOLDERS_H
#define AUTHUM_HOST_PLACEHOLDERS_H

#include <authum/env.h>
#include <authum/path.h>
#include <authum/placeholders.h>

AUTHUM_EXTERN_C_BEGIN

enum
{
    authum_placeholder_name_max = 64,
    authum_placeholder_value_max = 4096,
    authum_placeholder_standard_max = 1024
};

/**
 * @brief The standard names every module knows.
 *
 * The order matches the host's own table (see placeholders.c); never reorder
 * one without the other.
 */
typedef enum authum_placeholder_standard
{
    authum_placeholder_game,
    authum_placeholder_module,
    authum_placeholder_data,
    authum_placeholder_config,
    authum_placeholder_lang,
    authum_placeholder_addons,
    authum_placeholder_authum,
    authum_placeholder_modules,
    authum_placeholder_models,
    authum_placeholder_sprites,
    authum_placeholder_standard_count
} authum_placeholder_standard_t;

/**
 * @brief One module's placeholder scope.
 *
 * The standard values are the module's own well-known directories; they are
 * canonicalized once, when the service is bound.
 */
typedef struct authum_placeholder_owner
{
    authum_placeholder_service_t service;
    const authum_char_t *standard[authum_placeholder_standard_count];
    authum_char_t storage[authum_placeholder_standard_count][authum_placeholder_standard_max];
} authum_placeholder_owner_t;

/**
 * @brief Bind @p service to @p owner.
 *
 * @p standard holds the host-known values for that module (absolute paths,
 * or null when the host does not know them). The paths are copied and
 * canonicalized, so the caller may pass temporary strings.
 */
void
authum_placeholder_host_init(authum_placeholder_owner_t *owner,
                             const authum_char_t *const standard[authum_placeholder_standard_count],
                             authum_placeholder_service_t *service);

/**
 * @brief Bind @p service to @p owner, taking the standard values from @p env.
 *
 * The same directories the host publishes as ::authum_env_t, under their
 * placeholder names.
 */
void
authum_placeholder_host_init_env(authum_placeholder_owner_t *owner, const authum_env_t *env,
                                 authum_placeholder_service_t *service);

/** Remove every name @p owner registered. */
void
authum_placeholder_host_drop_owner(authum_placeholder_owner_t *owner);

/** True when @p name is one of the standard names. */
authum_bool_t
authum_placeholder_is_standard(const authum_char_t *name, authum_usize_t length);

/** The standard name for @p standard, or null when there is none. */
const authum_char_t *
authum_placeholder_standard_name(authum_placeholder_standard_t standard);

/**
 * @brief Canonical absolute form of @p input, written to @p out.
 *
 * Symlinks are resolved and `.`/`..` folded, so the result may be published
 * as a source root. A path that cannot be resolved is written unchanged,
 * including a missing path, on both platforms. Returns false for null pointers,
 * zero capacity or insufficient space; @p out is then unchanged. Input and
 * output may overlap.
 */
authum_bool_t
authum_placeholder_canonical(const authum_char_t *input, authum_char_t *out, authum_usize_t size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_PLACEHOLDERS_H */
