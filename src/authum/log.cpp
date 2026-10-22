#include <authum/log.h>
#include <authum/logger/host.h>

#include <extdll.h>
#include <meta_api.h>

#include <lh/cast/static.h>
#include <lh/config.h>
#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/logger.h>
#include <lh/logger/level.h>
#include <lh/logger/level/flags.h>
#include <lh/null.h>
#include <lh/runtime/logger.h>
#include <lh/self.h>
#include <lh/str.h>
#include <lh/str/format/text.h>
#include <lh/util/addr.h>
#include <lh/util/str/ptr.h>

#include <cstdarg>
#include <cstring>

extern plugin_info_t Plugin_info;
extern meta_globals_t *gpMetaGlobals;
extern mutil_funcs_t *gpMetaUtilFuncs;

namespace
{

enum
{
    authum_log_line_max = 1024
};

/* Console-level lines: Metamod prints them to the server console and the HLDS
 * log, tagged [AUTHUM]; a module's lines get its name too. */
void
authum_log_mirror_metamod(authum_logger_level_t level, const authum_char_t *name,
                          const authum_char_t *text)
{
    const char *tag = std::strcmp(name, "authum") == 0 ? "" : name;

    if (gpMetaUtilFuncs == nullptr || gpMetaGlobals == nullptr)
    {
        return;
    }
    if (level <= authum_logger_level_error)
    {
        LOG_ERROR(PLID, "%s%s%s%s", *tag ? "[" : "", tag, *tag ? "] " : "", text);
    }
    else
    {
        LOG_MESSAGE(PLID, "%s%s%s%s", *tag ? "[" : "", tag, *tag ? "] " : "", text);
    }
}

/* lh's runtime logger (the host's own lines and lh's) feeds the `authum` channel. */
lh_ssize_t
authum_log_emit(lh_self_ptr /* self */, lh_logger_level_t level, lh_str_cptr fmt, va_list args)
{
    char line[authum_log_line_max];
    lh_usize_t n;

    n = lh_str_ptr_format_text_v(line, sizeof(line) - 1u, fmt, args);
    if (n == 0u)
    {
        authum_logs_write(nullptr, static_cast<authum_logger_level_t>(level), fmt);
        return lh_cast_static(lh_ssize_t, lh_str_ptr_len(fmt));
    }
    line[n] = '\0';
    authum_logs_write(nullptr, static_cast<authum_logger_level_t>(level), line);
    return lh_cast_static(lh_ssize_t, n);
}

} // namespace

lh_bool_t
authum_log_init(const lh_fs_path_t *image_dir)
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
    ok = (root != lh_null && authum_logs_init(root, authum_log_mirror_metamod)) ? lh_bool_true
                                                                                : lh_bool_false;
    lh_str_deinit(lh_addr_of(scratch));
    lh_logger_init(lh_runtime_logger(), lh_logger_level_flags_all, authum_log_emit, lh_null);
    return ok;
}

void
authum_log_deinit(void)
{
    authum_logs_deinit();
#if (LH_LIBRARY_OPTION_RUNTIME_LOGGER_USE_STDLIB == LH_LIBRARY_OPTION_ON)
    lh_logger_init(lh_runtime_logger(), lh_logger_level_flags_all, lh_runtime_logger_stderr,
                   lh_null);
#else
    lh_logger_deinit(lh_runtime_logger());
#endif
}

void
authum_log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)lh_logger_log_v(lh_runtime_logger(), lh_logger_level_info, fmt, ap);
    va_end(ap);
}

void
authum_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)lh_logger_log_v(lh_runtime_logger(), lh_logger_level_error, fmt, ap);
    va_end(ap);
}
