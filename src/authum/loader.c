#include <authum/loader.h>
#include <lh/assert.h>
#include <lh/cast/const.h>
#include <lh/compiler/os.h>
#include <lh/foreach.h>
#include <lh/null.h>
#include <lh/os/fs.h>
#include <lh/util/addr.h>
#include <lh/util/str/ptr.h>

authum_loader_t authum_loader;

void
authum_loader_init(void)
{
    lh_vector_init(lh_addr_of(authum_loader.modules), sizeof(authum_module_t));
}

void
authum_loader_deinit(void)
{
    authum_loader_unload_all(&authum_loader);
    lh_vector_deinit(lh_addr_of(authum_loader.modules));
}

lh_bool_t
authum_loader_load(authum_loader_t *self, lh_str_cptr path)
{
    authum_module_t module;

    lh_assert_runtime_ref(self);
    authum_module_init(&module);
    if (!authum_module_load(&module, path))
    {
        return lh_bool_false;
    }
    lh_vector_push_back(lh_addr_of(self->modules), lh_addr_of(module));
    return lh_bool_true;
}

#define AUTHUM_PATH_MAX 4096U

#if LH_COMPILER_OS == LH_COMPILER_OS_WINDOWS
#    define AUTHUM_MODULE_EXT ".dll"
#    define AUTHUM_MODULE_SELF "authum_mm.dll"
#else
#    define AUTHUM_MODULE_EXT ".so"
#    define AUTHUM_MODULE_SELF "authum_mm.so"
#endif

static lh_bool_t
authum_loader_is_module_name(lh_str_cptr name)
{
    lh_str_ptr text = lh_cast_const(lh_str_ptr, name);

    if (!lh_str_ptr_ends_with(text, AUTHUM_MODULE_EXT, lh_bool_true))
    {
        return lh_bool_false;
    }
    if (lh_str_ptr_ends_with(text, AUTHUM_MODULE_SELF, lh_bool_true))
    {
        return lh_bool_false;
    }
    return lh_bool_true;
}

lh_bool_t
authum_loader_load_dir(authum_loader_t *self, lh_str_cptr dir)
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
        if (lh_os_fs_path_is(full, lh_os_fs_kind_dir) ||
            lh_os_fs_path_is(full, lh_os_fs_kind_shortcut) ||
            !lh_os_fs_path_is(full, lh_os_fs_kind_file))
        {
            continue;
        }
        (void)authum_loader_load(self, full);
    }

    lh_os_fs_dir_close(lh_addr_of(listing));
    return lh_bool_true;
}

lh_bool_t
authum_loader_load_modules(authum_loader_t *self, lh_str_cptr plugin_path)
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
authum_loader_unload_all(authum_loader_t *self)
{
    lh_assert_runtime_ref(self);
    lh_foreach(authum_module_t, module, lh_vector_get_data(lh_addr_of(self->modules)),
               lh_vector_get_size(lh_addr_of(self->modules)))
    {
        authum_module_unload(module);
    }
    lh_vector_clear(lh_addr_of(self->modules));
}

lh_usize_t
authum_loader_get_loaded(const authum_loader_t *self)
{
    lh_assert_runtime_ref(self);
    return lh_vector_get_size(lh_addr_of(self->modules));
}

authum_module_t *
authum_loader_get(authum_loader_t *self, lh_uindex_t index)
{
    lh_assert_runtime_ref(self);
    if (!lh_vector_is_valid_index(lh_addr_of(self->modules), index))
    {
        return lh_null;
    }
    return lh_ptr_cast(authum_module_t, lh_vector_get_ptr(lh_addr_of(self->modules), index));
}
