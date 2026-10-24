/**
 * @file library.h
 * @brief ELF bricks behind authum_os_library_symtab (POSIX only, not public ABI).
 */

#ifndef AUTHUM_CLIENT_OS_POSIX_LIBRARY_H
#define AUTHUM_CLIENT_OS_POSIX_LIBRARY_H

#include "../os.h"

#include <link.h>
#include <stdint.h>

/** Where the library is mapped and the file it came from. */
typedef struct authum_os_library
{
    const authum_char_t *name; /* in: file name to look for */
    uintptr_t base;            /* out: load bias (dlpi_addr) */
    authum_char_t path[512];   /* out: dlpi_name */
} authum_os_library_t;

/** dl_iterate_phdr callback: fills ::authum_os_library_t on a name match. */
int
authum_os_library_match(struct dl_phdr_info *info, size_t size, void *data);

authum_bool_t
authum_os_library_read_all(int fd, void *buf, size_t n);

/** Walk the on-disk ELF32 `.symtab` of @p library for @p name. */
authum_ptr
authum_os_library_find_in_symtab(const authum_os_library_t *library, const authum_char_t *name);

#endif /* AUTHUM_CLIENT_OS_POSIX_LIBRARY_H */
