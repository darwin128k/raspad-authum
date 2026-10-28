/**
 * @file memory.h
 * @brief Authum heap operations, backed by the image's LH runtime allocator.
 *
 * These blocks have explicit lifetime. For module-owned objects that must be
 * reclaimed at unload, use the sandbox API. Never mix heap and sandbox blocks,
 * libc blocks or blocks from another module image. The active LH allocator
 * and its context must remain compatible while its blocks exist; threads
 * exchanging blocks must use compatible runtime allocators.
 */

#ifndef AUTHUM_MEMORY_H
#define AUTHUM_MEMORY_H

#include <authum/extern.h>
#include <authum/ptr.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/** Allocate @p size bytes. Zero size or allocation failure returns authum_null.
 * The caller owns the block and must call authum_memory_free in this image. */
authum_ptr
authum_memory_alloc(authum_usize_t size);

/** Allocate and zero @p count items of @p item_size bytes.
 * Zero dimensions, overflow or allocation failure return authum_null. */
authum_ptr
authum_memory_alloc_zero(authum_usize_t count, authum_usize_t item_size);

/** Resize a block from this image; @p old_size must be its current byte size.
 * Null input allocates. Zero @p new_size releases the block and returns null.
 * Failure returns null, preserving the original block and its contents. */
authum_ptr
authum_memory_realloc(authum_ptr block, authum_usize_t old_size, authum_usize_t new_size);

/** Release an Authum heap block; null is ignored. */
void
authum_memory_free(authum_ptr block);

AUTHUM_EXTERN_C_END
#endif
