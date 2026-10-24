#include "../os.h"

#include <windows.h>

authum_ptr
authum_os_library_export(const authum_char_t *library, const authum_char_t *name)
{
    HMODULE module = GetModuleHandleA(library);

    return module ? (authum_ptr)GetProcAddress(module, name) : authum_null;
}

authum_ptr
authum_os_library_symtab(const authum_char_t *library, const authum_char_t *name)
{
    /* PE images carry no symbol table of their own; locals live in the .pdb. */
    (void)library;
    (void)name;
    return authum_null;
}
