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
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/null.h>
#include <authum/ptr.h>

AUTHUM_EXTERN_C_BEGIN

/** File name (no directory) of the HLDS engine library. */
#if defined(_WIN32)
#define AUTHUM_SYMBOL_ENGINE_LIBRARY "swds.dll"
#else
#define AUTHUM_SYMBOL_ENGINE_LIBRARY "engine_i486.so"
#endif

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

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_SYMBOL_H */
