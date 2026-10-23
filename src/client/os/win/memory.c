#include "../os.h"

#include <windows.h>

authum_bool_t
authum_os_memory_unprotect(authum_ptr address, authum_usize_t size)
{
    DWORD old_protect;

    return VirtualProtect(address, (SIZE_T)size, PAGE_EXECUTE_READWRITE, &old_protect)
               ? authum_bool_true
               : authum_bool_false;
}
