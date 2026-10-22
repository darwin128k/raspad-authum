/**
 * @file symbol_internal.h
 * @brief Bricks behind authum_symbol_find (not public ABI).
 */

#ifndef AUTHUM_SYMBOL_INTERNAL_H
#define AUTHUM_SYMBOL_INTERNAL_H

#include <authum/symbol.h>

#include <stdint.h>

#if !defined(_WIN32)

#include <link.h>

/** Where the library is mapped and the file it came from. */
typedef struct authum_symbol_library
{
    const authum_char_t *name; /* in: file name to look for */
    uintptr_t base;            /* out: load bias (dlpi_addr) */
    authum_char_t path[512];   /* out: dlpi_name */
} authum_symbol_library_t;

/** dl_iterate_phdr callback: fills ::authum_symbol_library_t on a name match. */
int
authum_symbol_match_library(struct dl_phdr_info *info, size_t size, void *data);

authum_bool_t
authum_symbol_read_all(int fd, void *buf, size_t n);

/** Walk the on-disk ELF32 `.symtab` of @p library for @p name. */
authum_ptr
authum_symbol_find_in_symtab(const authum_symbol_library_t *library, const authum_char_t *name);

#endif

#endif /* AUTHUM_SYMBOL_INTERNAL_H */
