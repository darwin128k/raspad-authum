/**
 * @file symbol.h
 * @brief Find a function or global inside a library the process already loaded.
 *
 * HLDS build 10211 ships `engine_i486.so` with most engine functions as
 * *local* symbols: `dlsym` cannot see them, but the on-disk ELF `.symtab`
 * still lists them. ::authum_symbol_find tries `dlsym` first and falls back to
 * walking `.symtab` of the loaded file. Modules that detour engine internals
 * (qproto, monitoring) resolve their targets through here.
 *
 * Linux only for the `.symtab` fallback; on Windows only exports resolve.
 */

#ifndef AUTHUM_SYMBOL_H
#define AUTHUM_SYMBOL_H

#include <authum/bool.h>
#include <authum/size.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/null.h>

/** File name (no directory) of the HLDS engine library. */
#if defined(_WIN32)
#    define AUTHUM_SYMBOL_ENGINE_LIBRARY "swds.dll"
#else
#    define AUTHUM_SYMBOL_ENGINE_LIBRARY "engine_i486.so"
#endif

AUTHUM_EXTERN_C_BEGIN

/**
 * Address of @p name inside the already-loaded library @p library (file
 * name, matched against the loaded path's last component). Never loads the
 * library. Returns ::authum_null when the library or symbol is missing.
 * When @p out_via_dynsym is non-null it is set to true only for a dynamic
 * export hit (false for a `.symtab` hit or a miss) — older engine builds
 * export what 10211 keeps local, so callers can tell builds apart.
 */
authum_ptr
authum_symbol_find(const authum_char_t *library, const authum_char_t *name,
                   authum_bool_t *out_via_dynsym);

/** ::authum_symbol_find on ::AUTHUM_SYMBOL_ENGINE_LIBRARY. */
authum_ptr
authum_symbol_find_engine(const authum_char_t *name, authum_bool_t *out_via_dynsym);

/**
 * Find code in a loaded @p library by its bytes, for a function that has no
 * symbol at all (Windows: an engine internal is neither exported nor in the
 * image). @p mask says which bits of @p pattern count, byte by byte: 0xFF the
 * whole byte, 0x00 any byte (an address that differs from build to build).
 * Only the image's executable code is searched. Never loads the library.
 *
 * @p out_count (optional) receives how many places match, counted up to 2:
 * a pattern meant to name one function should match exactly once, and a
 * caller should refuse 0 or 2.
 *
 * Returns the first match, or null (library not loaded, no code, no match).
 */
authum_ptr
authum_symbol_find_pattern(const authum_char_t *library, const authum_byte_t *pattern,
                           const authum_byte_t *mask, authum_usize_t size,
                           authum_usize_t *out_count);

/** authum_symbol_find_pattern in AUTHUM_SYMBOL_ENGINE_LIBRARY. */
authum_ptr
authum_symbol_find_engine_pattern(const authum_byte_t *pattern, const authum_byte_t *mask,
                                  authum_usize_t size, authum_usize_t *out_count);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_SYMBOL_H */
