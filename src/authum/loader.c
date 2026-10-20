#include <authum/error/code.h>
#include <authum/loader.h>
#include <authum/log.h>
#include <lh/cast/static.h>
#include <lh/compiler/os.h>
#include <lh/fs/file/mode.h>
#include <lh/fs/kind.h>
#include <lh/fs/path/style.h>
#include <lh/fs/stat.h>
#include <lh/char/map.h>
#include <lh/math/ops.h>
#include <lh/null.h>
#include <lh/os.h>
#include <lh/os/error/code.h>
#include <lh/os/fs/dir.h>
#include <lh/os/fs/file.h>
#include <lh/os/fs/stat.h>
#include <lh/os/loader.h>
#include <lh/os/result.h>
#include <lh/os/system/fs/path.h>
#include <lh/os/system/shared.h>
#include <lh/str.h>
#include <lh/str/line/reader.h>
#include <lh/str/view.h>
#include <lh/str/view/initializer.h>
#include <lh/util/addr.h>
#include <lh/util/array/ptr.h>
#include <lh/util/ptr.h>
#include <lh/util/str/ptr.h>

enum
{
    authum_loader_read_chunk = 4096
};

lh_bool_t
authum_loader_is_shared_name(const lh_str_view_t *name)
{
    lh_str_view_t ext;

    if (lh_str_view_is_empty(name))
    {
        return lh_bool_false;
    }
    lh_os_system_shared_get_ext(lh_addr_of(ext));
    return lh_str_view_ends_with(name, lh_addr_of(ext), lh_bool_true);
}

lh_bool_t
authum_loader_is_host_image(const lh_fs_path_t *path)
{
    lh_str_view_t ext;
    lh_str_view_t part;
    lh_str_view_t stem;
    lh_usize_t n;

    if (lh_fs_path_is_empty(path))
    {
        return lh_bool_false;
    }
    n = lh_fs_path_get_segment_count(path);
    if (lh_math_is_zero(n))
    {
        return lh_bool_false;
    }
    lh_fs_path_get_segment(path, lh_math_sub_one(n), lh_addr_of(part));
    stem = lh_str_view_lit("authum_mm");
    lh_os_system_shared_get_ext(lh_addr_of(ext));
    if (!lh_str_view_starts_with(lh_addr_of(part), lh_addr_of(stem), lh_bool_true))
    {
        return lh_bool_false;
    }
    if (!lh_str_view_ends_with(lh_addr_of(part), lh_addr_of(ext), lh_bool_true))
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t, lh_math_eq(lh_str_view_get_size(lh_addr_of(part)),
                                                lh_str_view_get_size(lh_addr_of(stem)) +
                                                    lh_str_view_get_size(lh_addr_of(ext))));
}

lh_bool_t
authum_loader_path_is_file(const lh_fs_path_t *path)
{
    lh_fs_stat_t st;

    if (!lh_os_fs_stat(path, lh_addr_of(st)))
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t,
                          lh_math_eq(lh_fs_stat_get_kind(lh_addr_of(st)), lh_fs_kind_file));
}

lh_bool_t
authum_loader_path_is_dir(const lh_fs_path_t *path)
{
    lh_fs_stat_t st;

    if (!lh_os_fs_stat(path, lh_addr_of(st)))
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t,
                          lh_math_eq(lh_fs_stat_get_kind(lh_addr_of(st)), lh_fs_kind_dir));
}

lh_bool_t authum_loader_ensure_shared_ext(lh_fs_path_t *path);

/*
 * Resolve an ini entry to a shared image path.
 * Prefer a package folder: modules/qproto/ → modules/qproto/qproto.so|.dll
 * Else a bare file (optional extension appended for this OS).
 */
lh_bool_t
authum_loader_resolve_image(const lh_fs_path_t *base, const lh_fs_path_t *entry,
                            lh_fs_path_t *out)
{
    lh_fs_path_t candidate;
    lh_fs_path_t leaf;
    lh_fs_path_t with_ext;
    lh_str_t leaf_text;
    lh_str_view_t name;
    lh_str_view_t ext;
    lh_str_view_t leaf_view;
    lh_usize_t n;

    lh_fs_path_init(lh_addr_of(candidate));
    if (lh_null_ne(base))
    {
        if (!lh_fs_path_join(lh_addr_of(candidate), base, entry))
        {
            lh_fs_path_deinit(lh_addr_of(candidate));
            return lh_bool_false;
        }
    }
    else
    {
        lh_fs_path_assign(lh_addr_of(candidate), entry);
    }

    if (authum_loader_path_is_dir(lh_addr_of(candidate)))
    {
        n = lh_fs_path_get_segment_count(entry);
        if (lh_math_is_zero(n))
        {
            lh_fs_path_deinit(lh_addr_of(candidate));
            return lh_bool_false;
        }
        lh_fs_path_get_segment(entry, lh_math_sub_one(n), lh_addr_of(name));
        lh_os_system_shared_get_ext(lh_addr_of(ext));
        lh_str_init(lh_addr_of(leaf_text));
        lh_str_append(lh_addr_of(leaf_text), lh_str_view_get_data(lh_addr_of(name)),
                      lh_str_view_get_size(lh_addr_of(name)));
        lh_str_append(lh_addr_of(leaf_text), lh_str_view_get_data(lh_addr_of(ext)),
                      lh_str_view_get_size(lh_addr_of(ext)));
        lh_str_as_view(lh_addr_of(leaf_text), lh_addr_of(leaf_view));
        lh_fs_path_init(lh_addr_of(leaf));
        lh_fs_path_set(lh_addr_of(leaf), leaf_view, lh_fs_path_style_posix);
        if (!lh_fs_path_join(out, lh_addr_of(candidate), lh_addr_of(leaf)))
        {
            lh_fs_path_deinit(lh_addr_of(leaf));
            lh_str_deinit(lh_addr_of(leaf_text));
            lh_fs_path_deinit(lh_addr_of(candidate));
            return lh_bool_false;
        }
        lh_fs_path_deinit(lh_addr_of(leaf));
        lh_str_deinit(lh_addr_of(leaf_text));
        lh_fs_path_deinit(lh_addr_of(candidate));
        return lh_bool_true;
    }

    if (authum_loader_path_is_file(lh_addr_of(candidate)))
    {
        lh_fs_path_assign(out, lh_addr_of(candidate));
        lh_fs_path_deinit(lh_addr_of(candidate));
        return lh_bool_true;
    }

    /* Not a dir yet — try native extension on the entry path. */
    lh_fs_path_init(lh_addr_of(with_ext));
    lh_fs_path_assign(lh_addr_of(with_ext), entry);
    (void)authum_loader_ensure_shared_ext(lh_addr_of(with_ext));
    lh_fs_path_deinit(lh_addr_of(candidate));
    lh_fs_path_init(lh_addr_of(candidate));
    if (lh_null_ne(base))
    {
        if (!lh_fs_path_join(lh_addr_of(candidate), base, lh_addr_of(with_ext)))
        {
            lh_fs_path_deinit(lh_addr_of(with_ext));
            lh_fs_path_deinit(lh_addr_of(candidate));
            return lh_bool_false;
        }
    }
    else
    {
        lh_fs_path_assign(lh_addr_of(candidate), lh_addr_of(with_ext));
    }
    lh_fs_path_deinit(lh_addr_of(with_ext));
    lh_fs_path_assign(out, lh_addr_of(candidate));
    lh_fs_path_deinit(lh_addr_of(candidate));
    return lh_bool_true;
}

void
authum_loader_log_attempt(const lh_fs_path_t *path, lh_bool_t ok)
{
    lh_str_t scratch;
    const lh_str_ptr path_text;
    const lh_os_error_desc_ptr desc;

    path_text = lh_fs_path_to_cstr(path, lh_fs_path_style_posix, lh_addr_of(scratch));
    if (ok)
    {
        authum_log_info("module OK: %s", path_text != lh_null ? path_text : "?");
    }
    else
    {
        desc = lh_os_get_last_error_desc();
        authum_log_error("module FAIL: %s (%s)", path_text != lh_null ? path_text : "?",
                         desc != lh_null ? (const char *)desc : "unknown error");
    }
    lh_str_deinit(lh_addr_of(scratch));
}

lh_bool_t
authum_loader_load_logged(authum_module_t *self, const lh_fs_path_t *path)
{
    lh_bool_t ok;

    ok = authum_loader_load(self, path);
    authum_loader_log_attempt(path, ok);
    return ok;
}

lh_bool_t
authum_loader_section_applies(const lh_str_view_t *section, const lh_str_view_t *os_section)
{
    lh_str_view_t modules;
    lh_str_view_t any;

    if (lh_str_view_is_empty(section))
    {
        return lh_bool_true;
    }
    if (lh_str_view_equals(section, os_section, lh_bool_true))
    {
        return lh_bool_true;
    }
    modules = lh_str_view_lit("modules");
    any = lh_str_view_lit("any");
    if (lh_str_view_equals(section, lh_addr_of(modules), lh_bool_true) ||
        lh_str_view_equals(section, lh_addr_of(any), lh_bool_true))
    {
        return lh_bool_true;
    }
    return lh_bool_false;
}

lh_bool_t
authum_loader_path_has_shared_ext(const lh_fs_path_t *path)
{
    lh_str_view_t part;
    lh_str_view_t ext;
    lh_usize_t n;

    n = lh_fs_path_get_segment_count(path);
    if (lh_math_is_zero(n))
    {
        return lh_bool_false;
    }
    lh_fs_path_get_segment(path, lh_math_sub_one(n), lh_addr_of(part));
    lh_os_system_shared_get_ext(lh_addr_of(ext));
    return lh_str_view_ends_with(lh_addr_of(part), lh_addr_of(ext), lh_bool_true);
}

/* If the path has no native shared suffix (.so / .dll / …), append it. */
lh_bool_t
authum_loader_ensure_shared_ext(lh_fs_path_t *path)
{
    lh_fs_path_style_t style;
    lh_str_t text;
    lh_str_view_t ext;
    lh_str_view_t view;

    if (authum_loader_path_has_shared_ext(path))
    {
        return lh_bool_true;
    }
    style = lh_os_system_fs_path_style_native();
    lh_os_system_shared_get_ext(lh_addr_of(ext));
    lh_str_init(lh_addr_of(text));
    lh_fs_path_to_str(path, style, lh_addr_of(text));
    lh_str_append(lh_addr_of(text), lh_str_view_get_data(lh_addr_of(ext)),
                  lh_str_view_get_size(lh_addr_of(ext)));
    lh_str_as_view(lh_addr_of(text), lh_addr_of(view));
    lh_fs_path_set(path, view, style);
    lh_str_deinit(lh_addr_of(text));
    return lh_bool_true;
}

lh_bool_t
authum_loader_modules_dir(const authum_module_t *self, lh_fs_path_t *out)
{
    const lh_fs_path_t *image;
    lh_fs_path_t parent;
    lh_fs_path_t sub;
    lh_bool_t ok;

    image = lh_os_module_get_path_as_const(self);
    if (lh_fs_path_is_empty(image))
    {
        return lh_bool_false;
    }
    lh_fs_path_init(lh_addr_of(parent));
    lh_fs_path_init(lh_addr_of(sub));
    lh_fs_path_set(lh_addr_of(sub), lh_str_view_lit(AUTHUM_MODULES_DIR), lh_fs_path_style_posix);
    ok = lh_cast_static(lh_bool_t, lh_fs_path_parent(lh_addr_of(parent), image) &&
                                       lh_fs_path_join(out, lh_addr_of(parent), lh_addr_of(sub)));
    lh_fs_path_deinit(lh_addr_of(parent));
    lh_fs_path_deinit(lh_addr_of(sub));
    return ok;
}

lh_bool_t
authum_loader_read_file(const lh_fs_path_t *path, lh_str_t *out)
{
    lh_char_t buf[authum_loader_read_chunk];
    lh_os_fs_file_t file;
    lh_ssize_t n;

    lh_os_fs_file_init(lh_addr_of(file));
    if (!lh_os_fs_file_open(lh_addr_of(file), path, lh_fs_file_mode_read))
    {
        lh_os_fs_file_deinit(lh_addr_of(file));
        return lh_bool_false;
    }
    for (;;)
    {
        n = lh_os_fs_file_read(lh_addr_of(file), buf, sizeof(buf));
        if (lh_math_is_negative(n))
        {
            lh_os_fs_file_deinit(lh_addr_of(file));
            return lh_bool_false;
        }
        if (lh_math_is_zero(n))
        {
            break;
        }
        lh_str_append(out, buf, lh_cast_static(lh_usize_t, n));
    }
    lh_os_fs_file_deinit(lh_addr_of(file));
    return lh_bool_true;
}

lh_bool_t
authum_loader_scan(authum_module_t *self, const lh_fs_path_t *dir, const lh_fs_path_t *skip)
{
    lh_os_fs_dir_entry_kind_t kind;
    lh_fs_path_t full;
    lh_fs_path_t name;
    lh_fs_stat_t st;
    lh_os_fs_dir_t listing;
    lh_ssize_t n;
    lh_str_view_t entry;

    if (lh_fs_path_is_empty(dir))
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty, "path is empty");
        return lh_bool_false;
    }
    if (!lh_os_fs_stat(dir, lh_addr_of(st)) ||
        lh_math_ne(lh_fs_stat_get_kind(lh_addr_of(st)), lh_fs_kind_dir))
    {
        return lh_bool_true;
    }

    lh_os_fs_dir_init(lh_addr_of(listing));
    if (!lh_os_fs_dir_open(lh_addr_of(listing), dir))
    {
        return lh_bool_false;
    }

    lh_fs_path_init(lh_addr_of(name));
    lh_fs_path_init(lh_addr_of(full));
    for (;;)
    {
        n = lh_os_fs_dir_read(lh_addr_of(listing), lh_addr_of(entry), lh_addr_of(kind));
        if (lh_math_is_negative(n))
        {
            lh_os_fs_dir_close(lh_addr_of(listing));
            lh_fs_path_deinit(lh_addr_of(name));
            lh_fs_path_deinit(lh_addr_of(full));
            return lh_bool_false;
        }
        if (lh_math_is_zero(n))
        {
            break;
        }
        if (lh_math_eq(kind, lh_fs_kind_dir) || lh_math_eq(kind, lh_fs_kind_symlink))
        {
            continue;
        }
        if (!authum_loader_is_shared_name(lh_addr_of(entry)))
        {
            continue;
        }
        lh_fs_path_set(lh_addr_of(name), entry, lh_fs_path_style_posix);
        if (!lh_fs_path_join(lh_addr_of(full), dir, lh_addr_of(name)))
        {
            continue;
        }
        if (lh_null_ne(skip) && lh_fs_path_equals(lh_addr_of(full), skip, lh_bool_false))
        {
            continue;
        }
        (void)authum_loader_load_logged(self, lh_addr_of(full));
    }

    lh_os_fs_dir_close(lh_addr_of(listing));
    lh_fs_path_deinit(lh_addr_of(name));
    lh_fs_path_deinit(lh_addr_of(full));
    return lh_bool_true;
}

lh_bool_t
authum_loader_init(void)
{
    lh_os_module_init(lh_addr_of(authum));
    if (!lh_os_module_bind(lh_addr_of(authum), lh_addr_of(authum_module_info)))
    {
        lh_os_module_deinit(lh_addr_of(authum));
        return lh_bool_false;
    }
    lh_os_module_set_ops(lh_addr_of(authum), lh_addr_of(authum_module_ops));
    if (!lh_os_module_start(lh_addr_of(authum)))
    {
        lh_os_module_deinit(lh_addr_of(authum));
        return lh_bool_false;
    }
    return lh_bool_true;
}

void
authum_loader_deinit(void)
{
    lh_os_module_deinit(lh_addr_of(authum));
}

/* child.c */
lh_os_module_t *
authum_child_load(lh_os_loader_t *loader, const lh_fs_path_t *path);

lh_bool_t
authum_loader_load(authum_module_t *self, const lh_fs_path_t *path)
{
    lh_os_loader_t *loader;

    loader = lh_os_module_get_loader(self);
    if (lh_null_eq(loader))
    {
        lh_os_set_last_error_lit(lh_os_error_code_not_open, "loader is not granted");
        return lh_bool_false;
    }
    if (authum_loader_is_host_image(path))
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t, lh_null_ne(authum_child_load(loader, path)));
}

lh_bool_t
authum_loader_load_dir(authum_module_t *self, const lh_fs_path_t *dir)
{
    return authum_loader_scan(self, dir, lh_null);
}

/* Whole-line comments in modules.list. */
static const lh_char_t authum_loader_list_comments[] = {lh_char_map_semicolon, lh_char_map_hash};

lh_bool_t
authum_loader_list_is_skipped(const lh_str_view_t *line)
{
    return lh_cast_static(lh_bool_t,
                          lh_str_view_is_empty(line) ||
                              lh_str_ptr_contains_char(authum_loader_list_comments,
                                                       lh_array_ptr_get_size(authum_loader_list_comments),
                                                       lh_str_view_get_first_char(line)));
}

/* `[name]` → write trimmed name into @p section. False = bad header. */
lh_bool_t
authum_loader_list_read_section(const lh_str_view_t *line, lh_str_view_t *section)
{
    const lh_usize_t size = lh_str_view_get_size(line);
    lh_str_view_t name;
    const lh_str_view_t brackets = lh_str_view_lit("[]");

    if (lh_math_lt(size, 2U) || lh_math_ne(lh_str_view_get_last_char(line), lh_char_map_rbracket))
    {
        return lh_bool_false;
    }
    lh_str_view_from_offset(line, 1U, lh_math_sub(size, 2U), lh_addr_of(name));
    lh_str_view_trim(lh_addr_of(name));
    if (lh_str_view_is_empty(lh_addr_of(name)))
    {
        return lh_bool_false;
    }
    if (lh_str_view_contains_of(lh_addr_of(name), lh_addr_of(brackets)))
    {
        return lh_bool_false;
    }
    lh_ptr_deref(section) = name;
    return lh_bool_true;
}

void
authum_loader_list_load_path(authum_module_t *self, const lh_fs_path_t *modules,
                             lh_bool_t have_modules, const lh_str_view_t *path_text,
                             lh_fs_path_t *item, lh_fs_path_t *full)
{
    lh_fs_path_set(item, *path_text, lh_os_system_fs_path_style_native());
    if (lh_fs_path_is_absolute(item))
    {
        if (!authum_loader_resolve_image(lh_null, item, full))
        {
            authum_log_error("modules.list resolve failed");
            return;
        }
        (void)authum_loader_load_logged(self, full);
        return;
    }
    if (!have_modules)
    {
        authum_log_error("modules.list entry skipped (no modules/ beside image): %.*s",
                         (int)lh_str_view_get_size(path_text),
                         lh_str_view_get_data(path_text));
        return;
    }
    if (!authum_loader_resolve_image(modules, item, full))
    {
        authum_log_error("modules.list resolve failed");
        return;
    }
    (void)authum_loader_load_logged(self, full);
}

lh_bool_t
authum_loader_load_list(authum_module_t *self, const lh_fs_path_t *path)
{
    lh_bool_t have_modules;
    lh_bool_t ok;
    lh_fs_path_t full;
    lh_fs_path_t item;
    lh_fs_path_t modules;
    lh_str_line_reader_t reader;
    lh_str_t text;
    lh_str_view_t line;
    lh_str_view_t os_section;
    lh_str_view_t section;
    lh_str_view_t view;

    if (lh_fs_path_is_empty(path))
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty, "path is empty");
        return lh_bool_false;
    }

#if LH_COMPILER_OS == LH_COMPILER_OS_WINDOWS
    os_section = lh_str_view_lit("win32");
#else
    os_section = lh_str_view_lit("linux");
#endif

    lh_str_init(lh_addr_of(text));
    if (!authum_loader_read_file(path, lh_addr_of(text)))
    {
        lh_str_deinit(lh_addr_of(text));
        return lh_bool_false;
    }

    lh_fs_path_init(lh_addr_of(modules));
    lh_fs_path_init(lh_addr_of(item));
    lh_fs_path_init(lh_addr_of(full));
    have_modules = authum_loader_modules_dir(self, lh_addr_of(modules));

    lh_str_as_view(lh_addr_of(text), lh_addr_of(view));
    lh_str_view_skip_bom(lh_addr_of(view));
    lh_str_line_reader_init(lh_addr_of(reader), view);
    lh_str_view_init_empty(lh_addr_of(section));
    ok = lh_bool_true;

    while (lh_str_line_reader_next(lh_addr_of(reader), lh_addr_of(line)))
    {
        lh_str_view_trim(lh_addr_of(line));
        if (authum_loader_list_is_skipped(lh_addr_of(line)))
        {
            continue;
        }
        if (lh_math_eq(lh_str_view_get_first_char(lh_addr_of(line)), lh_char_map_lbracket))
        {
            if (!authum_loader_list_read_section(lh_addr_of(line), lh_addr_of(section)))
            {
                lh_os_set_last_error_lit(authum_error_code_bad_modules_list, "bad modules.list");
                authum_log_error("modules.list parse error at line %lu",
                                 (unsigned long)lh_str_line_reader_get_number(lh_addr_of(reader)));
                ok = lh_bool_false;
                break;
            }
            continue;
        }
        if (!authum_loader_section_applies(lh_addr_of(section), lh_addr_of(os_section)))
        {
            continue;
        }
        authum_loader_list_load_path(self, lh_addr_of(modules), have_modules, lh_addr_of(line),
                                     lh_addr_of(item), lh_addr_of(full));
    }

    lh_fs_path_deinit(lh_addr_of(full));
    lh_fs_path_deinit(lh_addr_of(item));
    lh_fs_path_deinit(lh_addr_of(modules));
    lh_str_deinit(lh_addr_of(text));
    return ok;
}

lh_bool_t
authum_loader_load_modules(authum_module_t *self)
{
    const lh_fs_path_t *image;
    lh_bool_t ok;
    lh_fs_path_t dir;
    lh_fs_path_t ini;
    lh_fs_path_t parent;
    lh_fs_path_t sub;

    image = lh_os_module_get_path_as_const(self);
    if (lh_fs_path_is_empty(image))
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty, "path is empty");
        return lh_bool_false;
    }
    lh_fs_path_init(lh_addr_of(sub));
    lh_fs_path_init(lh_addr_of(parent));
    lh_fs_path_init(lh_addr_of(dir));
    lh_fs_path_init(lh_addr_of(ini));
    if (!lh_fs_path_parent(lh_addr_of(parent), image))
    {
        lh_fs_path_deinit(lh_addr_of(sub));
        lh_fs_path_deinit(lh_addr_of(parent));
        lh_fs_path_deinit(lh_addr_of(dir));
        lh_fs_path_deinit(lh_addr_of(ini));
        return lh_bool_true;
    }

    lh_fs_path_set(lh_addr_of(sub), lh_str_view_lit(AUTHUM_MODULES_LIST), lh_fs_path_style_posix);
    if (!lh_fs_path_join(lh_addr_of(ini), lh_addr_of(parent), lh_addr_of(sub)))
    {
        lh_fs_path_deinit(lh_addr_of(sub));
        lh_fs_path_deinit(lh_addr_of(parent));
        lh_fs_path_deinit(lh_addr_of(dir));
        lh_fs_path_deinit(lh_addr_of(ini));
        return lh_bool_false;
    }
    if (authum_loader_path_is_file(lh_addr_of(ini)))
    {
        {
            lh_str_t scratch;
            const lh_str_ptr list_text;

            list_text =
                lh_fs_path_to_cstr(lh_addr_of(ini), lh_fs_path_style_posix, lh_addr_of(scratch));
            authum_log_info("loading modules from %s",
                            list_text != lh_null ? list_text : "modules.list");
            lh_str_deinit(lh_addr_of(scratch));
        }
        ok = authum_loader_load_list(self, lh_addr_of(ini));
        lh_fs_path_deinit(lh_addr_of(sub));
        lh_fs_path_deinit(lh_addr_of(parent));
        lh_fs_path_deinit(lh_addr_of(dir));
        lh_fs_path_deinit(lh_addr_of(ini));
        return ok;
    }

    lh_fs_path_set(lh_addr_of(sub), lh_str_view_lit(AUTHUM_MODULES_DIR), lh_fs_path_style_posix);
    if (!lh_fs_path_join(lh_addr_of(dir), lh_addr_of(parent), lh_addr_of(sub)))
    {
        lh_fs_path_deinit(lh_addr_of(sub));
        lh_fs_path_deinit(lh_addr_of(parent));
        lh_fs_path_deinit(lh_addr_of(dir));
        lh_fs_path_deinit(lh_addr_of(ini));
        return lh_bool_false;
    }
    {
        lh_str_t scratch;
        const lh_str_ptr dir_text;

        dir_text = lh_fs_path_to_cstr(lh_addr_of(dir), lh_fs_path_style_posix, lh_addr_of(scratch));
        authum_log_info("no modules.list — scanning %s",
                        dir_text != lh_null ? dir_text : "modules/");
        lh_str_deinit(lh_addr_of(scratch));
    }
    ok = authum_loader_scan(self, lh_addr_of(dir), image);
    lh_fs_path_deinit(lh_addr_of(sub));
    lh_fs_path_deinit(lh_addr_of(parent));
    lh_fs_path_deinit(lh_addr_of(dir));
    lh_fs_path_deinit(lh_addr_of(ini));
    return ok;
}

void
authum_loader_unload_all(authum_module_t *self)
{
    lh_os_loader_t *loader;

    loader = lh_os_module_get_loader(self);
    if (lh_null_ne(loader))
    {
        lh_os_loader_unload(loader);
    }
}

lh_usize_t
authum_loader_get_loaded(const authum_module_t *self)
{
    const lh_os_loader_t *loader;

    loader = lh_os_module_get_loader(self);
    if (lh_null_eq(loader))
    {
        return 0U;
    }
    return lh_os_loader_get_loaded(loader);
}

authum_module_t *
authum_loader_get_first(const authum_module_t *self)
{
    const lh_os_loader_t *loader;

    loader = lh_os_module_get_loader(self);
    if (lh_null_eq(loader))
    {
        return lh_null;
    }
    return lh_os_loader_get_first(loader);
}

authum_module_t *
authum_loader_get_next(const authum_module_t *self, authum_module_t *child)
{
    const lh_os_loader_t *loader;

    loader = lh_os_module_get_loader(self);
    if (lh_null_eq(loader))
    {
        return lh_null;
    }
    return lh_os_loader_get_next(loader, child);
}

authum_module_t *
authum_loader_get(authum_module_t *self, lh_uindex_t index)
{
    lh_os_loader_t *loader;

    loader = lh_os_module_get_loader(self);
    if (lh_null_eq(loader))
    {
        return lh_null;
    }
    return lh_os_loader_get_at(loader, index);
}

authum_module_t *
authum_loader_find(authum_module_t *self, const char *name)
{
    authum_module_t *child;
    const authum_module_info_t *info;
    lh_str_view_t want;
    lh_str_view_t have;

    if (lh_null_eq(name) || name[0] == '\0')
    {
        return lh_null;
    }
    want = lh_initializer_of(lh_str_view_t, lh_str_view_initializer, name, lh_str_ptr_len(name));
    for (child = authum_loader_get_first(self); lh_null_ne(child);
         child = authum_loader_get_next(self, child))
    {
        info = authum_module_get_info(child);
        if (lh_null_eq(info) || lh_null_eq(info->name))
        {
            continue;
        }
        have = lh_initializer_of(lh_str_view_t, lh_str_view_initializer, info->name,
                                 lh_str_ptr_len(info->name));
        if (lh_str_view_equals(lh_addr_of(have), lh_addr_of(want), lh_bool_true))
        {
            return child;
        }
    }
    return lh_null;
}

lh_bool_t
authum_loader_resolve_name(authum_module_t *self, const char *name, lh_fs_path_t *out)
{
    lh_fs_path_t modules;
    lh_fs_path_t entry;
    lh_str_view_t name_view;
    lh_bool_t ok;

    if (lh_null_eq(name) || name[0] == '\0' || lh_null_eq(out))
    {
        return lh_bool_false;
    }
    lh_fs_path_init(lh_addr_of(modules));
    lh_fs_path_init(lh_addr_of(entry));
    if (!authum_loader_modules_dir(self, lh_addr_of(modules)))
    {
        lh_fs_path_deinit(lh_addr_of(modules));
        lh_fs_path_deinit(lh_addr_of(entry));
        lh_os_set_last_error_lit(lh_os_error_code_path_empty, "no modules/ beside authum_mm");
        return lh_bool_false;
    }
    name_view = lh_initializer_of(lh_str_view_t, lh_str_view_initializer, name, lh_str_ptr_len(name));
    lh_fs_path_set(lh_addr_of(entry), name_view, lh_os_system_fs_path_style_native());
    ok = authum_loader_resolve_image(lh_addr_of(modules), lh_addr_of(entry), out);
    lh_fs_path_deinit(lh_addr_of(modules));
    lh_fs_path_deinit(lh_addr_of(entry));
    if (!ok)
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty, "module image not found under modules/");
    }
    return ok;
}

lh_bool_t
authum_loader_load_name(authum_module_t *self, const char *name)
{
    lh_fs_path_t path;
    lh_bool_t ok;

    if (lh_null_ne(authum_loader_find(self, name)))
    {
        lh_os_set_last_error_lit(lh_os_error_code_already_open, "module already loaded");
        return lh_bool_false;
    }
    lh_fs_path_init(lh_addr_of(path));
    if (!authum_loader_resolve_name(self, name, lh_addr_of(path)))
    {
        lh_fs_path_deinit(lh_addr_of(path));
        return lh_bool_false;
    }
    ok = authum_loader_load_logged(self, lh_addr_of(path));
    lh_fs_path_deinit(lh_addr_of(path));
    return ok;
}

lh_bool_t
authum_loader_unload_name(authum_module_t *self, const char *name)
{
    lh_os_loader_t *loader;
    authum_module_t *child;

    loader = lh_os_module_get_loader(self);
    child = authum_loader_find(self, name);
    if (lh_null_eq(loader) || lh_null_eq(child))
    {
        lh_os_set_last_error_lit(lh_os_error_code_not_open, "module not loaded");
        return lh_bool_false;
    }
    lh_os_loader_unload_child(loader, child);
    return lh_bool_true;
}

lh_bool_t
authum_loader_reload_name(authum_module_t *self, const char *name)
{
    authum_module_t *child;
    lh_fs_path_t path;
    const lh_fs_path_t *image;
    lh_bool_t ok;

    child = authum_loader_find(self, name);
    lh_fs_path_init(lh_addr_of(path));
    if (lh_null_ne(child))
    {
        image = lh_os_module_get_path_as_const(child);
        if (lh_null_ne(image) && !lh_fs_path_is_empty(image))
        {
            lh_fs_path_assign(lh_addr_of(path), image);
        }
        if (!authum_loader_unload_name(self, name))
        {
            lh_fs_path_deinit(lh_addr_of(path));
            return lh_bool_false;
        }
    }
    if (lh_fs_path_is_empty(lh_addr_of(path)) &&
        !authum_loader_resolve_name(self, name, lh_addr_of(path)))
    {
        lh_fs_path_deinit(lh_addr_of(path));
        return lh_bool_false;
    }
    ok = authum_loader_load_logged(self, lh_addr_of(path));
    lh_fs_path_deinit(lh_addr_of(path));
    return ok;
}
