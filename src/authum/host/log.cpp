#include <authum/host/log.h>
#include <authum/null.h>
#include <authum/unused.h>
#include <lh/char/map.h>

#include <extdll.h>

#include <lh/config.h>
#include <lh/runtime/logger.h>

#include <cstdarg>
#include <cstring>

namespace
{

enum
{
    authum_log_line_max = 1024
};

/* lh's runtime logger (the host's own lines and lh's) feeds the `authum` channel. */
lh_ssize_t
authum_log_emit(lh_self_ptr /* self */, lh_logger_level_t level, lh_str_cptr fmt, va_list args)
{
    authum_char_t line[authum_log_line_max];
    lh_usize_t n;

    n = lh_str_ptr_format_text_v(line, lh_math_sub(sizeof(line), 1u), fmt, args);
    if (lh_math_eq(n, 0u))
    {
        authum_logs_write(authum_null, lh_cast_static(authum_logger_level_t, level), fmt);
        return lh_cast_static(lh_ssize_t, lh_str_ptr_len(fmt));
    }
    line[n] = lh_char_map_nul;
    authum_logs_write(authum_null, lh_cast_static(authum_logger_level_t, level), line);
    return lh_cast_static(lh_ssize_t, n);
}

} // namespace

lh_bool_t
authum_log_init_sink(const lh_fs_path_t *image_dir, authum_logs_mirror_fn sink)
{
    lh_str_t scratch;
    lh_str_cptr root;
    lh_bool_t ok;

    if (lh_null_eq(image_dir) || lh_fs_path_is_empty(image_dir))
    {
        return lh_bool_false;
    }
    authum_log_deinit();

    root = lh_fs_path_to_cstr(image_dir, lh_fs_path_style_posix, lh_addr_of(scratch));
    ok = (lh_math_ne(root, lh_null) && authum_logs_init(root, sink)) ? lh_bool_true : lh_bool_false;
    lh_str_deinit(lh_addr_of(scratch));
    lh_logger_init(lh_runtime_logger(), lh_logger_level_flags_all, authum_log_emit, lh_null);
    return ok;
}

lh_bool_t
authum_log_init(const lh_fs_path_t *image_dir)
{
    return authum_log_init_sink(image_dir, lh_null);
}

void
authum_log_deinit(void)
{
    authum_logs_deinit();
#if (lh_math_eq(LH_LIBRARY_OPTION_RUNTIME_LOGGER_USE_STDLIB, LH_LIBRARY_OPTION_ON))
    lh_logger_init(lh_runtime_logger(), lh_logger_level_flags_all, lh_runtime_logger_stderr,
                   lh_null);
#else
    lh_logger_deinit(lh_runtime_logger());
#endif
}

void
authum_log_info(const authum_char_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    authum_unused(lh_logger_log_v(lh_runtime_logger(), lh_logger_level_info, fmt, ap));
    va_end(ap);
}

void
authum_log_warning(const authum_char_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    authum_unused(lh_logger_log_v(lh_runtime_logger(), lh_logger_level_warning, fmt, ap));
    va_end(ap);
}

void
authum_log_error(const authum_char_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    authum_unused(lh_logger_log_v(lh_runtime_logger(), lh_logger_level_error, fmt, ap));
    va_end(ap);
}
