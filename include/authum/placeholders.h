/**
 * @file placeholders.h
 * @brief Path placeholders for module configuration (no lh).
 *
 * The host owns the placeholder vocabulary. Every module sees the standard
 * names — `{game}`, `{module}`, `{data}`, … — resolved against its own
 * environment, and may register extra names of its own. Modules never learn
 * about each other: a name is either standard or was registered through this
 * API, and a module may use any name another module registered.
 *
 *     authum_placeholder_service_t *service = authum_host_placeholders(host);
 *     authum_char_t path[512];
 *     if (!authum_placeholder_expand(service, "{game}/maps", path, sizeof path))
 *     {
 *         // unknown placeholder, or the result does not fit
 *     }
 *
 * A template is plain text with `{name}` tokens. Expansion substitutes the
 * registered value of each token and returns an absolute path: a template
 * that stays relative is taken against the calling module's own folder.
 * Unknown names, stray braces and oversized results fail — nothing is
 * guessed.
 *
 * Values are paths, not patterns: registering one is a promise that the
 * path may be published. Keep them inside what the module owns.
 *
 * Owner-thread only (like the HTTP service): register and expand from the
 * game thread, not from a worker.
 */

#ifndef AUTHUM_PLACEHOLDERS_H
#define AUTHUM_PLACEHOLDERS_H

#include <authum/abi.h>
#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/ptr.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

typedef struct authum_placeholder_ops
{
    authum_usize_t size;
    /** Expand @p template into @p out (absolute path). */
    authum_bool_t (*expand)(authum_ptr owner, const authum_char_t *template,
                            authum_char_t *out, authum_usize_t out_size);
    /** Register @p name for @p owner. Names are unique host-wide. */
    authum_bool_t (*register_placeholder)(authum_ptr owner, const authum_char_t *name,
                                          const authum_char_t *value);
    /** Remove the name @p owner registered. */
    authum_bool_t (*unregister_placeholder)(authum_ptr owner, const authum_char_t *name);
} authum_placeholder_ops_t;

typedef struct authum_placeholder_service
{
    const authum_placeholder_ops_t *ops;
    authum_ptr owner;
} authum_placeholder_service_t;

/**
 * Expand @p template into @p out.
 *
 * @p out receives an absolute path: every `{name}` is replaced by the value
 * registered for it, and a result that is still relative is taken against
 * the calling module's folder. False when a name is unknown, the braces do
 * not pair, the value does not fit in @p out_size, or the template is empty.
 */
authum_bool_t
authum_placeholder_expand(const authum_placeholder_service_t *service,
                          const authum_char_t *template, authum_char_t *out,
                          authum_usize_t out_size);

/**
 * Register @p name (letters, digits and `_`) with @p value (an absolute path).
 *
 * The name becomes available to every module through
 * ::authum_placeholder_expand. False when the name is a standard one, is
 * already taken, or the value is not an absolute path. The registry entry
 * lives until the registering module is unloaded.
 */
authum_bool_t
authum_placeholder_register(const authum_placeholder_service_t *service,
                            const authum_char_t *name, const authum_char_t *value);

/**
 * Remove the name @p owner registered. False when @p owner never registered
 * that name.
 */
authum_bool_t
authum_placeholder_unregister(const authum_placeholder_service_t *service,
                              const authum_char_t *name);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_PLACEHOLDERS_H */
