#include <authum/host/errors.h>
#include <authum/unused.h>
#include <authum/host/loader.h>
#include <authum/host/tasks.h>
#include <authum/host/log.h>
#include <lh/os.h>
#include <lh/os/error/code.h>
#include <lh/os/loader/image.h>
#include <lh/os/loader/list.h>
#include <lh/os/loader/scan.h>
#include <lh/os/system/fs/path.h>

void
authum_loader_log_attempt(const lh_fs_path_t *path, lh_bool_t ok)
{
    lh_str_t scratch;
    const lh_str_ptr path_text =
        lh_fs_path_to_cstr(path, lh_fs_path_style_posix, lh_addr_of(scratch));
    if (ok)
    {
        authum_log_info("module OK: %s", lh_math_ne(path_text, lh_null) ? path_text : "?");
    }
    else
    {
        const lh_os_error_desc_ptr desc = lh_os_get_last_error_desc();
        authum_log_error("module FAIL: %s (%s)", lh_math_ne(path_text, lh_null) ? path_text : "?",
                         lh_math_ne(desc, lh_null)
                             ? lh_cast_reinterpret(const authum_char_t *, desc)
                             : "unknown error");
    }
    lh_str_deinit(lh_addr_of(scratch));
}

lh_bool_t
authum_loader_load_logged(authum_module_t *self, const lh_fs_path_t *path)
{
    const lh_bool_t ok = authum_loader_load(self, path);

    authum_loader_log_attempt(path, ok);
    return ok;
}

lh_bool_t
authum_loader_modules_dir(const authum_module_t *self, lh_fs_path_t *out)
{
    const lh_fs_path_t *image = lh_os_module_get_path(self);

    if (lh_fs_path_is_empty(image))
    {
        return lh_bool_false;
    }

    lh_fs_path_t parent;

    lh_fs_path_init(lh_addr_of(parent));

    lh_fs_path_t sub;

    lh_fs_path_init(lh_addr_of(sub));
    lh_fs_path_set(lh_addr_of(sub), lh_str_view_lit(AUTHUM_MODULES_DIR), lh_fs_path_style_posix);

    const lh_bool_t ok =
        lh_cast_static(lh_bool_t, lh_fs_path_parent(lh_addr_of(parent), image) &&
                                      lh_fs_path_join(out, lh_addr_of(parent), lh_addr_of(sub)));

    lh_fs_path_deinit(lh_addr_of(parent));
    lh_fs_path_deinit(lh_addr_of(sub));
    return ok;
}

/*
 * A modules.list entry or a console name: an absolute path is taken as it is,
 * anything else lies under modules/ beside the Authum image. Either names the
 * image itself or its module folder (::lh_os_loader_image_try_resolve).
 */
lh_bool_t
authum_loader_try_resolve_entry(const authum_module_t *self, const lh_str_view_t *entry,
                                lh_fs_path_t *out)
{
    lh_fs_path_t item;

    lh_fs_path_init(lh_addr_of(item));
    lh_fs_path_set(lh_addr_of(item), lh_ptr_deref(entry), lh_os_system_fs_path_style_native());

    lh_fs_path_t modules;

    lh_fs_path_init(lh_addr_of(modules));

    lh_bool_t ok = lh_bool_false;

    if (lh_fs_path_is_absolute(lh_addr_of(item)))
    {
        ok = lh_os_loader_image_try_resolve(lh_null, lh_addr_of(item), out);
    }
    else if (!authum_loader_modules_dir(self, lh_addr_of(modules)))
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty,
                                 "no modules/ beside the Authum image");
    }
    else if (lh_os_loader_image_try_resolve(lh_addr_of(modules), lh_addr_of(item), out))
    {
        ok = lh_bool_true;
    }
    else
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty,
                                 "module image not found under modules/");
    }
    lh_fs_path_deinit(lh_addr_of(modules));
    lh_fs_path_deinit(lh_addr_of(item));
    return ok;
}

/* ::lh_os_loader_scan_fn: load each image the scan of modules/ finds. */
void
authum_loader_scan_take(lh_ptr user, const lh_fs_path_t *image)
{
    authum_unused(authum_loader_load_logged(lh_cast_static(authum_module_t *, user), image));
}

/* ::lh_os_loader_list_fn: load each modules.list entry for this system. */
void
authum_loader_list_take(lh_ptr user, const lh_str_view_t *entry)
{
    authum_module_t *self = lh_cast_static(authum_module_t *, user);
    lh_fs_path_t image;

    lh_fs_path_init(lh_addr_of(image));
    if (authum_loader_try_resolve_entry(self, entry, lh_addr_of(image)))
    {
        authum_unused(authum_loader_load_logged(self, lh_addr_of(image)));
    }
    else
    {
        /* The entry is a view into the list: copied to be shown. */
        lh_str_t shown;

        lh_str_init(lh_addr_of(shown));
        lh_str_append_view(lh_addr_of(shown), lh_ptr_deref(entry));

        const lh_os_error_desc_ptr desc = lh_os_get_last_error_desc();

        authum_log_error("modules.list entry skipped: %s (%s)", lh_str_get_data(lh_addr_of(shown)),
                         lh_math_ne(desc, lh_null)
                             ? lh_cast_reinterpret(const authum_char_t *, desc)
                             : "unknown error");
        lh_str_deinit(lh_addr_of(shown));
    }
    lh_fs_path_deinit(lh_addr_of(image));
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
    lh_os_loader_t *loader = lh_os_module_get_loader(self);

    if (lh_null_eq(loader))
    {
        lh_os_set_last_error_lit(lh_os_error_code_not_open, "loader is not granted");
        return lh_bool_false;
    }

    /* The host image (authum.so / authum.dll) is never a child of itself. */
    const lh_str_view_t host = lh_str_view_lit("authum");

    if (lh_os_loader_image_is_named(path, lh_addr_of(host)))
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t, lh_null_ne(authum_child_load(loader, path)));
}

lh_bool_t
authum_loader_load_dir(authum_module_t *self, const lh_fs_path_t *dir)
{
    return lh_os_loader_scan(dir, authum_loader_scan_take, self);
}

lh_bool_t
authum_loader_load_list(authum_module_t *self, const lh_fs_path_t *path)
{
    if (lh_fs_path_is_empty(path))
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty, "path is empty");
        return lh_bool_false;
    }

    lh_str_t text;

    lh_str_init(lh_addr_of(text));
    if (!lh_os_fs_file_read_all_by_path(path, lh_addr_of(text)))
    {
        lh_str_deinit(lh_addr_of(text));
        return lh_bool_false;
    }

    lh_str_view_t view;

    lh_str_as_view(lh_addr_of(text), lh_addr_of(view));

    lh_usize_t bad_line = 0U;
    const lh_bool_t ok = lh_os_loader_list_try_read(lh_addr_of(view), authum_loader_list_take, self,
                                                    lh_addr_of(bad_line));

    if (!ok)
    {
        lh_os_set_last_error_lit(authum_error_code_bad_modules_list, "bad modules.list");
        authum_log_error("modules.list parse error at line %lu",
                         lh_cast_static(lh_ulong_t, bad_line));
    }
    lh_str_deinit(lh_addr_of(text));
    return ok;
}

/* Log where the modules come from: the list, or the folder scanned. */
void
authum_loader_log_source(const authum_char_t *what, const lh_fs_path_t *path)
{
    lh_str_t scratch;
    const lh_str_ptr text = lh_fs_path_to_cstr(path, lh_fs_path_style_posix, lh_addr_of(scratch));

    authum_log_info("%s %s", what, lh_math_ne(text, lh_null) ? text : "?");
    lh_str_deinit(lh_addr_of(scratch));
}

lh_bool_t
authum_loader_load_modules(authum_module_t *self)
{
    const lh_fs_path_t *image = lh_os_module_get_path(self);

    if (lh_fs_path_is_empty(image))
    {
        lh_os_set_last_error_lit(lh_os_error_code_path_empty, "path is empty");
        return lh_bool_false;
    }

    lh_fs_path_t parent;

    lh_fs_path_init(lh_addr_of(parent));
    if (!lh_fs_path_parent(lh_addr_of(parent), image))
    {
        lh_fs_path_deinit(lh_addr_of(parent));
        return lh_bool_true;
    }

    lh_fs_path_t list;

    lh_fs_path_init(lh_addr_of(list));

    lh_fs_path_t sub;

    lh_fs_path_init(lh_addr_of(sub));
    lh_fs_path_set(lh_addr_of(sub), lh_str_view_lit("configs/" AUTHUM_MODULES_LIST),
                   lh_fs_path_style_posix);

    lh_bool_t ok = lh_fs_path_join(lh_addr_of(list), lh_addr_of(parent), lh_addr_of(sub));

    if (ok && lh_os_fs_is_file(lh_addr_of(list)))
    {
        authum_loader_log_source("loading modules from", lh_addr_of(list));
        ok = authum_loader_load_list(self, lh_addr_of(list));
    }
    else if (ok)
    {
        lh_fs_path_t dir;

        lh_fs_path_init(lh_addr_of(dir));
        ok = authum_loader_modules_dir(self, lh_addr_of(dir));
        if (ok)
        {
            authum_loader_log_source("no modules.list — scanning", lh_addr_of(dir));
            ok = authum_loader_load_dir(self, lh_addr_of(dir));
        }
        lh_fs_path_deinit(lh_addr_of(dir));
    }
    lh_fs_path_deinit(lh_addr_of(sub));
    lh_fs_path_deinit(lh_addr_of(list));
    lh_fs_path_deinit(lh_addr_of(parent));
    return ok;
}

void
authum_loader_unload_all(authum_module_t *self)
{
    lh_os_loader_t *loader;

    if (authum_tasks_host_dispatching())
    {
        return;
    }

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
authum_loader_find(authum_module_t *self, const authum_char_t *name)
{
    authum_module_t *child;
    const authum_module_info_t *info;

    if (lh_null_eq(name) || lh_math_eq(name[0], lh_char_map_nul))
    {
        return lh_null;
    }
    lh_str_view_t want =
        lh_initializer_of(lh_str_view_t, lh_str_view_initializer, name, lh_str_ptr_len(name));
    for (child = authum_loader_get_first(self); lh_null_ne(child);
         child = authum_loader_get_next(self, child))
    {
        info = authum_module_get_info(child);
        if (lh_null_eq(info) || lh_null_eq(info->name))
        {
            continue;
        }
        lh_str_view_t have = lh_initializer_of(lh_str_view_t, lh_str_view_initializer, info->name,
                                               lh_str_ptr_len(info->name));
        if (lh_str_view_equals(lh_addr_of(have), lh_addr_of(want), lh_bool_true))
        {
            return child;
        }
    }
    return lh_null;
}

lh_bool_t
authum_loader_resolve_name(authum_module_t *self, const authum_char_t *name, lh_fs_path_t *out)
{
    if (lh_null_eq(name) || lh_math_eq(name[0], lh_char_map_nul) || lh_null_eq(out))
    {
        return lh_bool_false;
    }

    const lh_str_view_t entry =
        lh_initializer_of(lh_str_view_t, lh_str_view_initializer, name, lh_str_ptr_len(name));

    return authum_loader_try_resolve_entry(self, lh_addr_of(entry), out);
}

lh_bool_t
authum_loader_load_name(authum_module_t *self, const authum_char_t *name)
{
    if (lh_null_ne(authum_loader_find(self, name)))
    {
        lh_os_set_last_error_lit(lh_os_error_code_already_open, "module already loaded");
        return lh_bool_false;
    }
    lh_fs_path_t path;
    lh_fs_path_init(lh_addr_of(path));
    if (!authum_loader_resolve_name(self, name, lh_addr_of(path)))
    {
        lh_fs_path_deinit(lh_addr_of(path));
        return lh_bool_false;
    }
    lh_bool_t ok = authum_loader_load_logged(self, lh_addr_of(path));
    lh_fs_path_deinit(lh_addr_of(path));
    return ok;
}

lh_bool_t
authum_loader_unload_name(authum_module_t *self, const authum_char_t *name)
{
    lh_os_loader_t *loader;
    authum_module_t *child;

    if (authum_tasks_host_dispatching())
    {
        lh_os_set_last_error_lit(lh_os_error_code_not_open,
                                 "cannot unload modules inside a task callback");
        return lh_bool_false;
    }

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
authum_loader_reload_name(authum_module_t *self, const authum_char_t *name)
{
    authum_module_t *child;
    const lh_fs_path_t *image;

    child = authum_loader_find(self, name);
    lh_fs_path_t path;
    lh_fs_path_init(lh_addr_of(path));
    if (lh_null_ne(child))
    {
        image = lh_os_module_get_path(child);
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
    lh_bool_t ok = authum_loader_load_logged(self, lh_addr_of(path));
    lh_fs_path_deinit(lh_addr_of(path));
    return ok;
}
