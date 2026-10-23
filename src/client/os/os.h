/**
 * @file os.h
 * @brief What authum_client needs from the operating system (not public ABI).
 *
 * One implementation per platform: `os/posix/` and `os/win/`. CMake compiles
 * the one for the target, so the rest of src/client has no `#if _WIN32`.
 */

#ifndef AUTHUM_CLIENT_OS_H
#define AUTHUM_CLIENT_OS_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

/** Create @p dir (one level). True when it is a directory afterwards. */
authum_bool_t
authum_os_dir_ensure(const authum_char_t *dir);

/** Make the pages covering [@p address, @p address + @p size) read/write/execute. */
authum_bool_t
authum_os_memory_unprotect(authum_ptr address, authum_usize_t size);

/**
 * Exported symbol @p name of the already loaded @p library (file name, no
 * directory). Never loads the library. ::authum_null when either is missing.
 */
authum_ptr
authum_os_library_export(const authum_char_t *library, const authum_char_t *name);

/**
 * Symbol @p name from the on-disk symbol table of the already loaded
 * @p library — finds what the library keeps local. ::authum_null when missing
 * or where the platform has no such table to read (Windows).
 */
authum_ptr
authum_os_library_symtab(const authum_char_t *library, const authum_char_t *name);

typedef void (*authum_os_ipv4_fn)(authum_u32_t ip, authum_ptr user);

/**
 * Call @p fn for every IPv4 address on this machine's interfaces, host byte
 * order, loopback and aliases included (filtering is the caller's job).
 */
void
authum_os_ipv4_each(authum_os_ipv4_fn fn, authum_ptr user);

#endif /* AUTHUM_CLIENT_OS_H */
