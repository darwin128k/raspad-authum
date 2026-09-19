#include <authum/module.h>
#include <lh/assert.h>
#include <lh/cast/static.h>
#include <lh/null.h>
#include <lh/os.h>
#include <lh/os/shared.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/version.h>

void
authum_module_init(authum_module_t *self)
{
    lh_assert_runtime_ref(self);
    self->handle = lh_null;
    self->info = lh_null;
}

lh_bool_t
authum_module_is_loaded(const authum_module_t *self)
{
    lh_assert_runtime_ref(self);
    return lh_cast_static(lh_bool_t, lh_null_ne(self->handle));
}

lh_bool_t
authum_module_load(authum_module_t *self, lh_str_cptr path)
{
    lh_os_shared_handle_t handle;
    lh_ptr sym;
    const authum_module_info_t *info;
    const lh_version_t required = AUTHUM_MODULE_ABI;

    if (authum_module_is_loaded(self))
    {
        lh_os_set_last_error(1, lh_os_error_desc_lit("already loaded"));
        return lh_bool_false;
    }

    handle = lh_os_shared_open(path);
    if (lh_null_eq(handle))
    {
        return lh_bool_false;
    }

    sym = lh_os_shared_get_sym(handle, AUTHUM_MODULE_INFO_SYM);
    if (lh_null_eq(sym))
    {
        (void)lh_os_shared_close(handle);
        lh_os_set_last_error(1, lh_os_error_desc_lit("missing authum_module_info"));
        return lh_bool_false;
    }

    info = lh_ptr_ccast(authum_module_info_t, sym);
    if (lh_null_eq(info->name))
    {
        (void)lh_os_shared_close(handle);
        lh_os_set_last_error(1, lh_os_error_desc_lit("invalid authum_module_info"));
        return lh_bool_false;
    }

    if (!lh_version_is_compatible(lh_addr_of(info->abi), lh_addr_of(required)))
    {
        (void)lh_os_shared_close(handle);
        lh_os_set_last_error(1, lh_os_error_desc_lit("incompatible module abi"));
        return lh_bool_false;
    }

    self->handle = handle;
    self->info = info;
    return lh_bool_true;
}

lh_bool_t
authum_module_unload(authum_module_t *self)
{
    if (!authum_module_is_loaded(self))
    {
        return lh_bool_true;
    }
    if (!lh_os_shared_close(self->handle))
    {
        return lh_bool_false;
    }
    self->handle = lh_null;
    self->info = lh_null;
    return lh_bool_true;
}

lh_ptr
authum_module_get_sym(const authum_module_t *self, lh_str_cptr name)
{
    if (!authum_module_is_loaded(self))
    {
        lh_os_set_last_error(1, lh_os_error_desc_lit("not loaded"));
        return lh_null;
    }
    return lh_os_shared_get_sym(self->handle, name);
}

lh_bool_t
authum_module_has_sym(const authum_module_t *self, lh_str_cptr name)
{
    if (!authum_module_is_loaded(self))
    {
        lh_os_set_last_error(1, lh_os_error_desc_lit("not loaded"));
        return lh_bool_false;
    }
    return lh_os_shared_has_sym(self->handle, name);
}

const authum_module_info_t *
authum_module_get_info(const authum_module_t *self)
{
    lh_assert_runtime_ref(self);
    return self->info;
}
