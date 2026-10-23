#include "../os.h"

#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

authum_bool_t
authum_os_memory_unprotect(authum_ptr address, authum_usize_t size)
{
    uintptr_t page_size = (uintptr_t)sysconf(_SC_PAGESIZE);
    uintptr_t start = (uintptr_t)address & ~(page_size - 1u);
    uintptr_t end = (uintptr_t)address + (uintptr_t)size;

    return mprotect((void *)start, (size_t)(end - start), PROT_READ | PROT_WRITE | PROT_EXEC) == 0
               ? authum_bool_true
               : authum_bool_false;
}
