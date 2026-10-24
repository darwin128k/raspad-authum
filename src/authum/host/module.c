#include <authum/host/errors.h>
#include <authum/host/loader.h>
#include <authum/host/module.h>
#include <authum/module/api.h>
#include <authum/version.h>
#include <lh/null.h>
#include <lh/os.h>
#include <lh/util/ptr.h>

authum_module_t authum;

/* from child.c */
const authum_module_info_t *
authum_child_peek_info(const void *data);

lh_bool_t
authum_module_take_info(authum_module_t *self)
{
    const authum_module_info_t *info;
    const authum_version_t required = AUTHUM_MODULE_ABI;
    lh_ptr sym;

    sym = lh_os_module_get_sym(self, AUTHUM_MODULE_INFO_SYM);
    if (lh_null_eq(sym))
    {
        lh_os_set_last_error_lit(authum_error_code_missing_info, "missing authum_module_info");
        return lh_bool_false;
    }
    info = lh_ptr_ccast(authum_module_info_t, sym);
    if (lh_null_eq(info->name))
    {
        lh_os_set_last_error_lit(authum_error_code_invalid_info, "invalid authum_module_info");
        return lh_bool_false;
    }
    if (!authum_version_is_abi_compatible(&info->abi, &required))
    {
        lh_os_set_last_error_lit(authum_error_code_incompatible_abi, "incompatible module abi");
        return lh_bool_false;
    }
    lh_os_module_set_data(self, lh_ptr_cast(lh_void, info));
    return lh_bool_true;
}

lh_bool_t
authum_module_start(authum_module_t *self)
{
    if (!authum_module_take_info(self))
    {
        return lh_bool_false;
    }
    if (lh_null_eq(lh_os_module_grant_loader(self, AUTHUM_MODULE_OPS_SYM)))
    {
        return lh_bool_false;
    }
    if (lh_null_ne(lh_os_module_get_owner(self)))
    {
        (void)authum_loader_load_modules(self);
    }
    return lh_bool_true;
}

void
authum_module_stop(authum_module_t *self)
{
    lh_os_module_set_data(self, lh_null);
}

AUTHUM_MODULE_EXPORT const lh_os_module_ops_t authum_module_ops = {
    .start = authum_module_start,
    .stop = authum_module_stop,
};

const authum_module_info_t *
authum_module_get_info(const authum_module_t *self)
{
    lh_ptr data;

    data = lh_os_module_get_data(self);
    if (lh_null_eq(data))
    {
        return lh_null;
    }
    if (!lh_null_eq(lh_os_module_get_sym(self, AUTHUM_MODULE_API_SYM)))
    {
        return authum_child_peek_info(data);
    }
    return lh_ptr_ccast(authum_module_info_t, data);
}
