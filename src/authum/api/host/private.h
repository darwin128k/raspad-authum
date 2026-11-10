/**
 * @file private.h
 * @brief Private to `host.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_HOST_PRIVATE_H
#define AUTHUM_API_HOST_PRIVATE_H

#include <authum/host.h>
#include <lh/core/util/ptr.h>
#include <lh/core/memory/std.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief * Write @p text with the module's mark in front, so a line says who wrote it.  A host that
 * is not a module, or a module that named no mark, writes the text as it is.
 */
void
authum_host_marked_text(const authum_host_t *host, const authum_char_t *text, authum_char_t *buf,
                        authum_usize_t buf_size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_HOST_PRIVATE_H */
