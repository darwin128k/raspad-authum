#include <authum/loader.h>
#include <lh/assert.h>
#include <lh/cast/const.h>
#include <lh/null.h>
#include <lh/os/fs.h>
#include <lh/os/shared.h>
#include <lh/os/shared/module.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/util/str/ptr.h>

#define AUTHUM_PATH_MAX 4096U
#define AUTHUM_MODULE_SELF_STEM "authum_mm"

static lh_os_shared_module_t *
authum_loader_shared(authum_module_t *self)
{
    return lh_ptr_cast(lh_os_shared_module_t, self);
}

static const lh_os_shared_module_t *
authum_loader_shared_c(const authum_module_t *self)
{
    return lh_ptr_ccast(lh_os_shared_module_t, self);
}

void
authum_loader_init(void)
{
    authum_module_init(&authum);
    (void)authum_module_bind(&authum, &authum_module_info);
}

void
authum_loader_deinit(void)
{
    authum_module_deinit(&authum);
}

lh_bool_t
authum_loader_load(authum_module_t *self, lh_str_cptr path)
{
    authum_module_t child;

    lh_assert_runtime_ref(self);
    authum_module_init(&child);
    if (!authum_module_load(&child, path))
    {
        authum_module_deinit(&child);
        return lh_bool_false;
    }
    lh_os_shared_module_push(authum_loader_shared(self), authum_loader_shared(&child));
    return lh_bool_true;
}

static lh_bool_t
authum_loader_is_module_name(lh_str_cptr name)
{
    lh_str_ptr text;

    if (!lh_os_shared_is(name))
    {
        return lh_bool_false;
    }
    text = lh_cast_const(lh_str_ptr, name);
    if (lh_str_ptr_ends_with(text, AUTHUM_MODULE_SELF_STEM ".dll", lh_bool_true) ||
        lh_str_ptr_ends_with(text, AUTHUM_MODULE_SELF_STEM ".so", lh_bool_true))
    {
        return lh_bool_false;
    }
    return lh_bool_true;
}

lh_bool_t
authum_loader_load_dir(authum_module_t *self, lh_str_cptr dir)
{
    lh_os_fs_dir_t listing;
    lh_char_t name[LH_OS_FS_DIR_NAME_MAX + 1U];
    lh_char_t full[AUTHUM_PATH_MAX];
    lh_ssize_t n;

    lh_assert_runtime_ref(self);

    if (lh_null_eq(dir) || dir[0] == '\0')
    {
        return lh_bool_false;
    }
    if (!lh_os_fs_path_is(dir, lh_os_fs_kind_dir))
    {
        return lh_bool_true;
    }

    lh_os_fs_dir_init(lh_addr_of(listing));
    if (!lh_os_fs_dir_open(lh_addr_of(listing), dir))
    {
        return lh_bool_false;
    }

    for (;;)
    {
        n = lh_os_fs_dir_read(lh_addr_of(listing), name, sizeof(name), lh_null);
        if (n < 0)
        {
            lh_os_fs_dir_close(lh_addr_of(listing));
            return lh_bool_false;
        }
        if (n == 0)
        {
            break;
        }
        if (!authum_loader_is_module_name(name))
        {
            continue;
        }
        if (!lh_os_fs_path_join(full, sizeof(full), dir, name))
        {
            continue;
        }
        if (!lh_os_shared_path_is(full))
        {
            continue;
        }
        (void)authum_loader_load(self, full);
    }

    lh_os_fs_dir_close(lh_addr_of(listing));
    return lh_bool_true;
}

lh_bool_t
authum_loader_load_modules(authum_module_t *self, lh_str_cptr plugin_path)
{
    lh_char_t plugin_dir[AUTHUM_PATH_MAX];
    lh_char_t modules_dir[AUTHUM_PATH_MAX];

    lh_assert_runtime_ref(self);

    if (!lh_os_fs_path_dir(plugin_path, plugin_dir, sizeof(plugin_dir)))
    {
        return lh_bool_false;
    }
    if (!lh_os_fs_path_join(modules_dir, sizeof(modules_dir), plugin_dir, AUTHUM_MODULES_DIR))
    {
        return lh_bool_false;
    }
    return authum_loader_load_dir(self, modules_dir);
}

void
authum_loader_unload_all(authum_module_t *self)
{
    lh_os_shared_module_unload_all(authum_loader_shared(self));
}

lh_usize_t
authum_loader_get_loaded(const authum_module_t *self)
{
    return lh_os_shared_module_get_loaded(authum_loader_shared_c(self));
}

authum_module_t *
authum_loader_get(authum_module_t *self, lh_uindex_t index)
{
    return lh_ptr_cast(authum_module_t,
                       lh_os_shared_module_get(authum_loader_shared(self), index));
}
