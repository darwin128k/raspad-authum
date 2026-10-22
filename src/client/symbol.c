#include <authum/symbol.h>

#include "os/os.h"

authum_ptr
authum_symbol_find(const authum_char_t *library, const authum_char_t *name,
                   authum_bool_t *out_via_dynsym)
{
    authum_ptr address;

    if (!authum_null_eq(out_via_dynsym))
    {
        *out_via_dynsym = authum_bool_false;
    }
    if (authum_null_eq(library) || authum_null_eq(name))
    {
        return authum_null;
    }
    address = authum_os_library_export(library, name);
    if (!authum_null_eq(address))
    {
        if (!authum_null_eq(out_via_dynsym))
        {
            *out_via_dynsym = authum_bool_true;
        }
        return address;
    }
    return authum_os_library_symtab(library, name);
}

authum_ptr
authum_symbol_find_engine(const authum_char_t *name, authum_bool_t *out_via_dynsym)
{
    return authum_symbol_find(AUTHUM_SYMBOL_ENGINE_LIBRARY, name, out_via_dynsym);
}
