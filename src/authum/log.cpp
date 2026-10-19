#include <authum/log.h>

#include <extdll.h>
#include <meta_api.h>

#include <lh/cast/static.h>
#include <lh/config.h>
#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/logger.h>
#include <lh/logger/level.h>
#include <lh/logger/level/flags.h>
#include <lh/math/ops.h>
#include <lh/null.h>
#include <lh/runtime/logger.h>
#include <lh/self.h>
#include <lh/str.h>
#include <lh/str/format/text.h>
#include <lh/str/view/initializer.h>
#include <lh/util/addr.h>
#include <lh/util/str/ptr.h>

#include <cstdarg>
#include <cstdio>
#include <ctime>

extern plugin_info_t Plugin_info;
extern meta_globals_t *gpMetaGlobals;
extern mutil_funcs_t *gpMetaUtilFuncs;

namespace {

FILE *g_file = nullptr;

enum
{
    authum_log_line_max = 1024
};

void
mirror_metamod(int is_error, const char *text)
{
    if (gpMetaUtilFuncs == nullptr || gpMetaGlobals == nullptr)
    {
        return;
    }
    if (is_error != 0)
    {
        LOG_ERROR(PLID, "%s", text);
    }
    else
    {
        LOG_MESSAGE(PLID, "%s", text);
    }
}

lh_ssize_t
authum_logger_emit(lh_self_ptr self, lh_logger_level_t level, lh_str_cptr fmt, va_list args)
{
    char line[authum_log_line_max];
    lh_usize_t n;
    const char *text;
    FILE *out;
    int is_error;

    out = static_cast<FILE *>(self);
    if (out == nullptr)
    {
        out = g_file;
    }

    n = lh_str_ptr_format_text_v(line, sizeof(line) - 1u, fmt, args);
    text = line;
    if (n == 0u)
    {
        text = fmt;
        n = lh_str_ptr_len(fmt);
    }

    is_error = lh_math_le(level, lh_logger_level_error) ? 1 : 0;

    if (out != nullptr)
    {
        time_t t = time(nullptr);
        struct tm *lt = localtime(&t);
        if (lt != nullptr)
        {
            fprintf(out, "[%02d:%02d:%02d] ", lt->tm_hour, lt->tm_min, lt->tm_sec);
        }
        fwrite(text, 1u, static_cast<size_t>(n), out);
        fputc('\n', out);
        fflush(out);
    }
    else
    {
        fwrite(text, 1u, static_cast<size_t>(n), stderr);
        fputc('\n', stderr);
    }

    mirror_metamod(is_error, text);
    return lh_cast_static(lh_ssize_t, n + 1u);
}

void
wire_runtime_logger(FILE *file)
{
    lh_logger_init(lh_runtime_logger(), lh_logger_level_flags_all, authum_logger_emit, file);
}

} // namespace

lh_bool_t
authum_log_init(const lh_fs_path_t *image_dir)
{
    lh_fs_path_t log_path;
    lh_fs_path_t leaf;
    lh_str_t scratch;
    const lh_str_ptr path_text;

    if (lh_null_eq(image_dir) || lh_fs_path_is_empty(image_dir))
    {
        return lh_bool_false;
    }

    authum_log_deinit();

    lh_fs_path_init(lh_addr_of(leaf));
    lh_fs_path_set(lh_addr_of(leaf), lh_str_view_lit("authum.log"), lh_fs_path_style_posix);
    lh_fs_path_init(lh_addr_of(log_path));
    if (!lh_fs_path_join(lh_addr_of(log_path), image_dir, lh_addr_of(leaf)))
    {
        lh_fs_path_deinit(lh_addr_of(leaf));
        lh_fs_path_deinit(lh_addr_of(log_path));
        return lh_bool_false;
    }
    lh_fs_path_deinit(lh_addr_of(leaf));

    path_text =
        lh_fs_path_to_cstr(lh_addr_of(log_path), lh_fs_path_style_posix, lh_addr_of(scratch));
    if (path_text == lh_null)
    {
        lh_fs_path_deinit(lh_addr_of(log_path));
        lh_str_deinit(lh_addr_of(scratch));
        return lh_bool_false;
    }

    g_file = fopen(path_text, "a");
    lh_fs_path_deinit(lh_addr_of(log_path));
    lh_str_deinit(lh_addr_of(scratch));
    if (g_file == nullptr)
    {
        wire_runtime_logger(nullptr);
        return lh_bool_false;
    }
    setvbuf(g_file, nullptr, _IONBF, 0);
    {
        time_t t = time(nullptr);
        fprintf(g_file, "\n---- authum log opened %s", ctime(&t));
    }
    wire_runtime_logger(g_file);
    return lh_bool_true;
}

void
authum_log_deinit(void)
{
    if (g_file != nullptr)
    {
        fclose(g_file);
        g_file = nullptr;
    }
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

void
authum_host_log(int is_error, const char *line)
{
    if (line == nullptr)
    {
        return;
    }
    if (is_error != 0)
    {
        authum_log_error("%s", line);
    }
    else
    {
        authum_log_info("%s", line);
    }
}
