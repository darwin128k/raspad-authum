/**
 * @file private.h
 * @brief Private to `logger.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_LOGGER_PRIVATE_H
#define AUTHUM_API_LOGGER_PRIVATE_H

#include <authum/logger.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Make @p self log nowhere: no module channel, no journal (level 2).
 *        Nothing is closed.
 */
void
authum_logger_clear(authum_logger_t *self);

/**
 * @brief Keep @p journal, opened through @p ops, as the channel @p self logs
 *        to; ::authum_logger_close closes it (level 2).
 */
void
authum_logger_adopt_journal(authum_logger_t *self, const authum_logger_ops_t *ops,
                            authum_ptr journal);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_LOGGER_PRIVATE_H */
