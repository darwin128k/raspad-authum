/**
 * @file buffer.c
 * @brief Worker log lines held for the game thread, over lh's log capture — see
 *        <authum/logger/buffer.h>.
 *
 * Each line keeps the channel it was written to (its ops and owner), so a
 * temporary logger handle may be gone by the time the lines are flushed.
 */

#include <authum/api/logger/buffer/private.h>
#include <authum/memory.h>
#include <authum/unused.h>
#include <lh/core/util/offset.h>

authum_logger_buffer_impl_t *
authum_logger_buffer_get_impl(const authum_logger_buffer_t *self)
{
    return lh_cast_static(authum_logger_buffer_impl_t *, self->impl);
}

void
authum_logger_buffer_set_impl(authum_logger_buffer_t *self, authum_logger_buffer_impl_t *impl)
{
    self->impl = impl;
}

authum_logger_buffer_t *
authum_logger_buffer_impl_get_handle(const authum_logger_buffer_impl_t *self)
{
    return self->handle;
}

void
authum_logger_buffer_impl_set_handle(authum_logger_buffer_impl_t *self,
                                     authum_logger_buffer_t *handle)
{
    self->handle = handle;
}

authum_logger_t *
authum_logger_buffer_impl_get_overflow(const authum_logger_buffer_impl_t *self)
{
    return self->overflow;
}

void
authum_logger_buffer_impl_set_overflow(authum_logger_buffer_impl_t *self, authum_logger_t *overflow)
{
    self->overflow = overflow;
}

const lh_os_logger_capture_t *
authum_logger_buffer_impl_get_capture(const authum_logger_buffer_impl_t *self)
{
    return lh_addr_of(self->capture);
}

lh_os_logger_capture_t *
authum_logger_buffer_impl_get_capture_mut(authum_logger_buffer_impl_t *self)
{
    return lh_addr_of(self->capture);
}

lh_os_logger_capture_line_t *
authum_logger_buffer_impl_get_lines_mut(authum_logger_buffer_impl_t *self)
{
    return self->lines;
}

authum_logger_buffer_impl_t *
authum_logger_buffer_impl_of(lh_os_logger_capture_t *capture)
{
    return authum_null_eq(capture)
               ? authum_null
               : lh_ptr_sub_by_offset(authum_logger_buffer_impl_t, capture,
                                      lh_offset_of(authum_logger_buffer_impl_t, capture));
}

lh_void
authum_logger_buffer_replay(lh_ptr user, const lh_os_logger_capture_line_t *line)
{
    const lh_os_logger_record_t *record = lh_os_logger_capture_line_get_record(line);
    authum_logger_channel_t channel;

    authum_unused(user);
    authum_logger_channel_set_ops(
        lh_addr_of(channel),
        lh_cast_static(const authum_logger_ops_t *, lh_os_logger_capture_line_get_target(line)));
    authum_logger_channel_set_owner(lh_addr_of(channel),
                                    lh_os_logger_capture_line_get_context(line));
    if (authum_logger_channel_is_enabled(lh_addr_of(channel),
                                         lh_os_logger_record_get_level(record)))
    {
        authum_logger_channel_write(lh_addr_of(channel), lh_os_logger_record_get_level(record),
                                    lh_os_logger_record_get_text(record));
    }
}

authum_bool_t
authum_logger_buffer_is_active(void)
{
    return !authum_null_eq(lh_os_logger_capture_get_current());
}

authum_bool_t
authum_logger_buffer_log_v(authum_logger_t *logger, authum_logger_level_t level,
                           const authum_char_t *fmt, va_list ap)
{
    lh_os_logger_capture_t *capture = lh_os_logger_capture_get_current();

    if (authum_null_eq(capture))
    {
        return authum_bool_false;
    }

    /* Captured or not, the line is the buffer's: it never reaches the host here. */
    const authum_logger_channel_t *channel = authum_logger_get_channel(logger);

    if (authum_null_eq(channel) || authum_null_eq(fmt) ||
        lh_math_gt(level, authum_logger_level_debug))
    {
        return authum_bool_true;
    }
    /* The channel's ops and owner, not the logger handle (authum_host_log uses a
       temporary one); they must outlive the flush. */
    authum_unused(lh_os_logger_capture_try_push_v(capture, authum_logger_channel_get_ops(channel),
                                                  authum_logger_channel_get_owner(channel), level,
                                                  fmt, ap));
    return authum_bool_true;
}

authum_bool_t
authum_logger_buffer_init(authum_logger_buffer_t *self, authum_logger_t *overflow,
                          authum_usize_t capacity)
{
    if (authum_null_eq(self) || !authum_null_eq(authum_logger_buffer_get_impl(self)) ||
        lh_math_eq(capacity, 0u) || lh_math_gt(capacity, 256u))
    {
        return authum_bool_false;
    }

    authum_logger_buffer_impl_t *impl = authum_memory_alloc_zero(
        1u, lh_math_add(sizeof lh_ptr_deref(impl),
                        lh_math_mul(capacity, sizeof(lh_os_logger_capture_line_t))));

    if (authum_null_eq(impl))
    {
        return authum_bool_false;
    }
    authum_logger_buffer_impl_set_handle(impl, self);
    authum_logger_buffer_impl_set_overflow(impl, overflow);
    lh_os_logger_capture_init(authum_logger_buffer_impl_get_capture_mut(impl),
                              authum_logger_buffer_impl_get_lines_mut(impl), capacity);
    authum_logger_buffer_set_impl(self, impl);
    return authum_bool_true;
}

void
authum_logger_buffer_destroy(authum_logger_buffer_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }

    authum_logger_buffer_impl_t *impl = authum_logger_buffer_get_impl(self);

    if (!authum_null_eq(impl) && lh_math_eq(lh_os_logger_capture_get_current(),
                                            authum_logger_buffer_impl_get_capture_mut(impl)))
    {
        lh_os_logger_capture_end(authum_null);
    }
    authum_memory_free(impl);
    authum_logger_buffer_set_impl(self, authum_null);
}

authum_logger_buffer_t *
authum_logger_buffer_begin(authum_logger_buffer_t *self)
{
    authum_logger_buffer_impl_t *impl =
        !authum_null_eq(self) ? authum_logger_buffer_get_impl(self) : authum_null;
    authum_logger_buffer_impl_t *previous = authum_logger_buffer_impl_of(lh_os_logger_capture_begin(
        !authum_null_eq(impl) ? authum_logger_buffer_impl_get_capture_mut(impl) : authum_null));

    return !authum_null_eq(previous) ? authum_logger_buffer_impl_get_handle(previous) : authum_null;
}

void
authum_logger_buffer_end(authum_logger_buffer_t *previous)
{
    authum_logger_buffer_impl_t *impl =
        !authum_null_eq(previous) ? authum_logger_buffer_get_impl(previous) : authum_null;

    lh_os_logger_capture_end(!authum_null_eq(impl) ? authum_logger_buffer_impl_get_capture_mut(impl)
                                                   : authum_null);
}

void
authum_logger_buffer_flush(authum_logger_buffer_t *self)
{
    authum_logger_buffer_impl_t *impl =
        !authum_null_eq(self) ? authum_logger_buffer_get_impl(self) : authum_null;

    if (authum_null_eq(impl))
    {
        return;
    }

    lh_os_logger_capture_t *capture = authum_logger_buffer_impl_get_capture_mut(impl);
    const authum_usize_t dropped = lh_os_logger_capture_get_dropped(capture);

    lh_os_logger_capture_replay(capture, authum_logger_buffer_replay, authum_null);
    lh_os_logger_capture_clear(capture);
    if (lh_math_ne(dropped, 0u))
    {
        /* Captured lines are suspended for this one too: it goes out directly. */
        lh_os_logger_capture_t *previous = lh_os_logger_capture_begin(authum_null);

        authum_logger_warning(authum_logger_buffer_impl_get_overflow(impl),
                              "%u background log messages dropped",
                              lh_cast_static(authum_u32_t, dropped));
        lh_os_logger_capture_end(previous);
    }
}

authum_usize_t
authum_logger_buffer_get_dropped(const authum_logger_buffer_t *self)
{
    const authum_logger_buffer_impl_t *impl =
        !authum_null_eq(self) ? authum_logger_buffer_get_impl(self) : authum_null;

    return !authum_null_eq(impl)
               ? lh_os_logger_capture_get_dropped(authum_logger_buffer_impl_get_capture(impl))
               : 0u;
}
