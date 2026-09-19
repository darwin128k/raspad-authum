#include <authum/loader.h>
#include <authum/module.h>
#include <lh/assert.h>
#include <lh/null.h>
#include <lh/os.h>
#include <lh/os/shared/module.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/version.h>

authum_module_t authum;

static lh_os_shared_module_t *
authum_module_shared(authum_module_t *self)
{
    return lh_ptr_cast(lh_os_shared_module_t, self);
}

static const lh_os_shared_module_t *
authum_module_shared_c(const authum_module_t *self)
{
    return lh_ptr_ccast(lh_os_shared_module_t, self);
}

void
authum_module_init(authum_module_t *self)
{
    lh_assert_runtime_ref(self);
    lh_os_shared_module_init(authum_module_shared(self), sizeof(authum_module_t));
    self->info = lh_null;
}

void
authum_module_deinit(authum_module_t *self)
{
    lh_assert_runtime_ref(self);
    lh_os_shared_module_deinit(authum_module_shared(self));
    self->info = lh_null;
}

lh_bool_t
authum_module_is_loaded(const authum_module_t *self)
{
    return lh_os_shared_module_is_loaded(authum_module_shared_c(self));
}

lh_bool_t
authum_module_load(authum_module_t *self, lh_str_cptr path)
{
    lh_ptr sym;
    const authum_module_info_t *info;
    const lh_version_t required = AUTHUM_MODULE_ABI;

    lh_assert_runtime_ref(self);
    if (!lh_os_shared_module_open(authum_module_shared(self), path))
    {
        return lh_bool_false;
    }

    sym = lh_os_shared_module_get_sym(authum_module_shared(self), AUTHUM_MODULE_INFO_SYM);
    if (lh_null_eq(sym))
    {
        (void)lh_os_shared_module_close(authum_module_shared(self));
        lh_os_set_last_error(1, lh_os_error_desc_lit("missing authum_module_info"));
        return lh_bool_false;
    }

    info = lh_ptr_ccast(authum_module_info_t, sym);
    if (lh_null_eq(info->name))
    {
        (void)lh_os_shared_module_close(authum_module_shared(self));
        lh_os_set_last_error(1, lh_os_error_desc_lit("invalid authum_module_info"));
        return lh_bool_false;
    }

    if (!lh_version_is_compatible(lh_addr_of(info->abi), lh_addr_of(required)))
    {
        (void)lh_os_shared_module_close(authum_module_shared(self));
        lh_os_set_last_error(1, lh_os_error_desc_lit("incompatible module abi"));
        return lh_bool_false;
    }

    self->info = info;
    return lh_bool_true;
}

lh_bool_t
authum_module_bind(authum_module_t *self, const authum_module_info_t *info)
{
    const lh_version_t required = AUTHUM_MODULE_ABI;

    lh_assert_runtime_ref(self);

    if (lh_null_eq(info) || lh_null_eq(info->name))
    {
        lh_os_set_last_error(1, lh_os_error_desc_lit("invalid authum_module_info"));
        return lh_bool_false;
    }
    if (!lh_version_is_compatible(lh_addr_of(info->abi), lh_addr_of(required)))
    {
        lh_os_set_last_error(1, lh_os_error_desc_lit("incompatible module abi"));
        return lh_bool_false;
    }
    if (!lh_os_shared_module_bind(authum_module_shared(self), lh_ptr_cast(lh_void, info)))
    {
        return lh_bool_false;
    }

    self->info = info;
    return lh_bool_true;
}

lh_bool_t
authum_module_unload(authum_module_t *self)
{
    if (!lh_os_shared_module_unload(authum_module_shared(self)))
    {
        return lh_bool_false;
    }
    self->info = lh_null;
    return lh_bool_true;
}

lh_ptr
authum_module_get_sym(const authum_module_t *self, lh_str_cptr name)
{
    return lh_os_shared_module_get_sym(authum_module_shared_c(self), name);
}

lh_bool_t
authum_module_has_sym(const authum_module_t *self, lh_str_cptr name)
{
    return lh_os_shared_module_has_sym(authum_module_shared_c(self), name);
}

const authum_module_info_t *
authum_module_get_info(const authum_module_t *self)
{
    lh_assert_runtime_ref(self);
    return self->info;
}
