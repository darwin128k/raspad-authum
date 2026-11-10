/**
 * @file private.h
 * @brief Private to `events.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_EVENTS_PRIVATE_H
#define AUTHUM_API_EVENTS_PRIVATE_H

#include <authum/events.h>
#include <lh/core/str.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Whether @p name is one of the module's own names (`<prefix>.<what>`).
 */
authum_bool_t
authum_events_name_is_own(const authum_events_t *self, const authum_char_t *name);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_EVENTS_PRIVATE_H */
