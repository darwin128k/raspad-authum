#include <authum/logger/buffer.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct authum_logger_buffer_line
{
    authum_logger_t *target;
    authum_logger_level_t level;
    authum_char_t text[authum_logger_line_max];
} authum_logger_buffer_line_t;
typedef struct authum_logger_buffer_impl
{
    authum_logger_t *overflow;
    authum_usize_t count, capacity, dropped;
    authum_logger_buffer_line_t lines[];
} authum_logger_buffer_impl_t;
static _Thread_local authum_logger_buffer_t *m_capture;

authum_bool_t
authum_logger_buffer_init(authum_logger_buffer_t *self, authum_logger_t *overflow,
                          authum_usize_t capacity)
{
    authum_logger_buffer_impl_t *impl;
    if (!self || self->impl || !capacity || capacity > 256)
    {
        return authum_bool_false;
    }
    impl = calloc(1, sizeof *impl + capacity * sizeof impl->lines[0]);
    if (!impl)
    {
        return authum_bool_false;
    }
    impl->capacity = capacity;
    impl->overflow = overflow;
    self->impl = impl;
    return authum_bool_true;
}
void
authum_logger_buffer_destroy(authum_logger_buffer_t *self)
{
    if (!self)
    {
        return;
    }
    if (m_capture == self)
    {
        m_capture = NULL;
    }
    free(self->impl);
    self->impl = NULL;
}
authum_logger_buffer_t *
authum_logger_buffer_begin(authum_logger_buffer_t *self)
{
    authum_logger_buffer_t *previous = m_capture;
    m_capture = self && self->impl ? self : NULL;
    return previous;
}
void
authum_logger_buffer_end(authum_logger_buffer_t *previous)
{
    m_capture = previous;
}
authum_bool_t
authum_logger_buffer_active(void)
{
    return m_capture != NULL;
}
authum_bool_t
authum_logger_buffer_log_v(authum_logger_t *logger, authum_logger_level_t level,
                           const authum_char_t *fmt, va_list ap)
{
    authum_logger_buffer_impl_t *impl;
    authum_logger_buffer_line_t *line;
    if (!m_capture)
    {
        return authum_bool_false;
    }
    if (!logger || !fmt || level > authum_logger_level_debug)
    {
        return authum_bool_true;
    }
    impl = m_capture->impl;
    if (impl->count == impl->capacity)
    {
        ++impl->dropped;
        return authum_bool_true;
    }
    line = &impl->lines[impl->count++];
    line->target = logger;
    line->level = level;
    vsnprintf(line->text, sizeof line->text, fmt, ap);
    return authum_bool_true;
}
void
authum_logger_buffer_flush(authum_logger_buffer_t *self)
{
    authum_logger_buffer_impl_t *impl;
    authum_logger_buffer_t *previous;
    authum_usize_t i;
    if (!self || !(impl = self->impl))
    {
        return;
    }
    previous = authum_logger_buffer_begin(NULL);
    for (i = 0; i < impl->count; ++i)
    {
        authum_logger_log(impl->lines[i].target, impl->lines[i].level, "%s", impl->lines[i].text);
    }
    if (impl->dropped)
    {
        authum_logger_warning(impl->overflow, "%u background log messages dropped",
                              (unsigned)impl->dropped);
    }
    impl->count = impl->dropped = 0;
    authum_logger_buffer_end(previous);
}
authum_usize_t
authum_logger_buffer_dropped(const authum_logger_buffer_t *self)
{
    const authum_logger_buffer_impl_t *impl = self ? self->impl : NULL;
    return impl ? impl->dropped : 0;
}
