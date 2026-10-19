#include <authum/error/code.h>
#include <authum/loader.h>
#include <lh/cast/static.h>
#include <lh/compiler/os.h>
#include <lh/fs/file/mode.h>
#include <lh/fs/kind.h>
#include <lh/fs/path/style.h>
#include <lh/fs/stat.h>
#include <lh/ini/entry.h>
#include <lh/ini/entry/kind.h>
#include <lh/ini/parser.h>
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
#include <lh/str/view.h>
#include <lh/str/view/initializer.h>
#include <lh/util/addr.h>

enum
{
    authum_loader_read_chunk = 4096
};

static lh_bool_t
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

static lh_bool_t
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

static lh_bool_t
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

static lh_bool_t
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

static lh_bool_t
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

static lh_bool_t
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
        (void)authum_loader_load(self, lh_addr_of(full));
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
    return lh_cast_static(lh_bool_t, lh_null_ne(lh_os_loader_load(loader, path)));
}

lh_bool_t
authum_loader_load_dir(authum_module_t *self, const lh_fs_path_t *dir)
{
    return authum_loader_scan(self, dir, lh_null);
}

lh_bool_t
authum_loader_load_ini(authum_module_t *self, const lh_fs_path_t *path)
{
    lh_bool_t have_modules;
    lh_bool_t ok;
    lh_fs_path_t full;
    lh_fs_path_t item;
    lh_fs_path_t modules;
    lh_ini_entry_t entry;
    lh_ini_parser_t parser;
    lh_str_t text;
    lh_str_view_t os_section;
    lh_str_view_t section;
    lh_str_view_t value;
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
    lh_ini_parser_init(lh_addr_of(parser), view);
    while (lh_ini_parser_next(lh_addr_of(parser), lh_addr_of(entry)))
    {
        if (!lh_ini_entry_has_kind(lh_addr_of(entry), lh_ini_entry_kind_pair))
        {
            continue;
        }
        section = lh_ini_entry_get_section(lh_addr_of(entry));
        if (!lh_str_view_equals(lh_addr_of(section), lh_addr_of(os_section), lh_bool_true))
        {
            continue;
        }
        value = lh_ini_entry_get_value(lh_addr_of(entry));
        if (lh_str_view_is_empty(lh_addr_of(value)))
        {
            continue;
        }
        lh_fs_path_set(lh_addr_of(item), value, lh_os_system_fs_path_style_native());
        if (lh_fs_path_is_absolute(lh_addr_of(item)))
        {
            (void)authum_loader_load(self, lh_addr_of(item));
            continue;
        }
        if (!have_modules)
        {
            continue;
        }
        if (!lh_fs_path_join(lh_addr_of(full), lh_addr_of(modules), lh_addr_of(item)))
        {
            continue;
        }
        (void)authum_loader_load(self, lh_addr_of(full));
    }

    ok = lh_bool_true;
    if (lh_ini_parser_has_error(lh_addr_of(parser)))
    {
        lh_os_set_last_error_lit(authum_error_code_bad_modules_ini, "bad modules.ini");
        ok = lh_bool_false;
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

    lh_fs_path_set(lh_addr_of(sub), lh_str_view_lit(AUTHUM_MODULES_INI), lh_fs_path_style_posix);
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
        ok = authum_loader_load_ini(self, lh_addr_of(ini));
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
