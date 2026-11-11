/**
 * @file private.h
 * @brief Private to `logger/buffer.c`: what a log buffer holds behind its handle —
 *        lh's capture, its lines and the logger that hears of drops.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_LOGGER_BUFFER_PRIVATE_H
#define AUTHUM_API_LOGGER_BUFFER_PRIVATE_H

#include <authum/logger/buffer.h>
#include <lh/os/logger/capture.h>

/** @brief The storage behind an ::authum_logger_buffer_t, one block. */
typedef struct authum_logger_buffer_impl
{
    authum_logger_buffer_t *handle;      /**< the handle this storage is behind */
    authum_logger_t *overflow;           /**< told how many lines were dropped */
    lh_os_logger_capture_t capture;      /**< lh's capture over @c lines */
    lh_os_logger_capture_line_t lines[]; /**< the capture's room */
} authum_logger_buffer_impl_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The storage behind the handle; ::authum_null before init (level 1).
 *
 * @param self The handle; must not be ::authum_null.
 */
authum_logger_buffer_impl_t *
authum_logger_buffer_get_impl(const authum_logger_buffer_t *self);

/**
 * @brief Store the storage behind the handle (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The handle; must not be ::authum_null.
 * @param impl The new value.
 */
void
authum_logger_buffer_set_impl(authum_logger_buffer_t *self, authum_logger_buffer_impl_t *impl);

/**
 * @brief The handle this storage is behind (level 1).
 *
 * @param self The storage; must not be ::authum_null.
 */
authum_logger_buffer_t *
authum_logger_buffer_impl_get_handle(const authum_logger_buffer_impl_t *self);

/**
 * @brief Store the handle this storage is behind (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self   The storage; must not be ::authum_null.
 * @param handle The new value.
 */
void
authum_logger_buffer_impl_set_handle(authum_logger_buffer_impl_t *self,
                                     authum_logger_buffer_t *handle);

/**
 * @brief The logger told how many lines were dropped (level 1).
 *
 * @param self The storage; must not be ::authum_null.
 */
authum_logger_t *
authum_logger_buffer_impl_get_overflow(const authum_logger_buffer_impl_t *self);

/**
 * @brief Store the logger told how many lines were dropped (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self     The storage; must not be ::authum_null.
 * @param overflow The new value.
 */
void
authum_logger_buffer_impl_set_overflow(authum_logger_buffer_impl_t *self,
                                       authum_logger_t *overflow);

/**
 * @brief lh's capture (level 1).
 *
 * @param self The storage; must not be ::authum_null.
 * @return The member itself, never ::authum_null.
 */
const lh_os_logger_capture_t *
authum_logger_buffer_impl_get_capture(const authum_logger_buffer_impl_t *self);

/**
 * @brief lh's capture, to change in place (level 1).
 *
 * @param self The storage; must not be ::authum_null.
 * @return The member itself, never ::authum_null.
 */
lh_os_logger_capture_t *
authum_logger_buffer_impl_get_capture_mut(authum_logger_buffer_impl_t *self);

/**
 * @brief The capture's room (level 1).
 *
 * @param self The storage; must not be ::authum_null.
 * @return The member itself, never ::authum_null.
 */
lh_os_logger_capture_line_t *
authum_logger_buffer_impl_get_lines_mut(authum_logger_buffer_impl_t *self);

/**
 * @brief The storage @p capture is part of (level 2).
 *
 * @param capture A capture inside an ::authum_logger_buffer_impl_t, or ::authum_null.
 * @return ::authum_null for ::authum_null.
 */
authum_logger_buffer_impl_t *
authum_logger_buffer_impl_of(lh_os_logger_capture_t *capture);

/**
 * @brief lh's replay callback: write one held line to the channel it was meant for.
 *
 * The line's target is the channel's ops, its context the channel's owner.
 */
lh_void
authum_logger_buffer_replay(lh_ptr user, const lh_os_logger_capture_line_t *line);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_LOGGER_BUFFER_PRIVATE_H */
