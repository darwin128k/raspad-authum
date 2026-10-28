#include <authum/symbol.h>
#include <lh/util/ptr.h>

#include <lh/os/shared.h>

authum_ptr
authum_symbol_find(const authum_char_t *library, const authum_char_t *name,
                   authum_bool_t *out_via_dynsym)
{
    lh_os_shared_t image;
    authum_ptr address;

    if (!authum_null_eq(out_via_dynsym))
    {
        lh_ptr_deref(out_via_dynsym) = authum_bool_false;
    }
    if (authum_null_eq(library) || authum_null_eq(name))
    {
        return authum_null;
    }
    /* Only an image already loaded: lh never loads it here. */
    lh_os_shared_init(lh_addr_of(image));
    if (!lh_os_shared_bind_name(lh_addr_of(image), library))
    {
        lh_os_shared_deinit(lh_addr_of(image));
        return authum_null;
    }
    address = lh_os_shared_get_sym(lh_addr_of(image), name);
    if (!authum_null_eq(address))
    {
        if (!authum_null_eq(out_via_dynsym))
        {
            lh_ptr_deref(out_via_dynsym) = authum_bool_true;
        }
    }
    else
    {
        address = lh_os_shared_get_local_sym(lh_addr_of(image), name);
    }
    lh_os_shared_close(lh_addr_of(image));
    lh_os_shared_deinit(lh_addr_of(image));
    return address;
}

authum_ptr
authum_symbol_find_engine(const authum_char_t *name, authum_bool_t *out_via_dynsym)
{
    return authum_symbol_find(AUTHUM_SYMBOL_ENGINE_LIBRARY, name, out_via_dynsym);
}
