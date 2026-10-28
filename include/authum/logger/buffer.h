#ifndef AUTHUM_LOGGER_BUFFER_H
#define AUTHUM_LOGGER_BUFFER_H

#include <authum/logger.h>

typedef struct authum_logger_buffer
{
    authum_ptr impl;
} authum_logger_buffer_t;

AUTHUM_EXTERN_C_BEGIN

/** Initialize a zeroed buffer (1..256 lines). Owns its storage; no host calls.
 * Capture on one worker, then flush on the game thread after work completes.
 * Logger handles used during capture must remain alive until flush/discard.
 */
authum_bool_t
authum_logger_buffer_init(authum_logger_buffer_t *self, authum_logger_t *overflow_logger,
                          authum_usize_t capacity);
void
authum_logger_buffer_destroy(authum_logger_buffer_t *self);

/** Thread-local capture scope; save the previous value and restore it with end.
 * Ordinary authum_logger_* calls are buffered without touching host log ops.
 */
authum_logger_buffer_t *
authum_logger_buffer_begin(authum_logger_buffer_t *self);

void
authum_logger_buffer_end(authum_logger_buffer_t *previous);

void
authum_logger_buffer_flush(authum_logger_buffer_t *self);

authum_usize_t
authum_logger_buffer_dropped(const authum_logger_buffer_t *self);

/** Internal logger interception; exported for the client-library implementation. */
authum_bool_t
authum_logger_buffer_active(void);

authum_bool_t
authum_logger_buffer_log_v(authum_logger_t *logger, authum_logger_level_t level,
                           const authum_char_t *fmt, va_list ap);

AUTHUM_EXTERN_C_END

#endif
