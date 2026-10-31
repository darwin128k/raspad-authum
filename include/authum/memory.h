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

#include <authum/bool.h>
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

/**
 * @brief Copy @p size bytes from @p src to @p dst.
 *
 * A null pointer on either side copies nothing.
 *
 * @param dst  Destination.
 * @param src  Source.
 * @param size Number of bytes.
 */
void
authum_memory_copy(authum_ptr dst, authum_cptr src, authum_usize_t size);

/**
 * @brief Write @p size bytes of @p value into @p dst.
 *
 * A null @p dst writes nothing.
 *
 * @param dst   Destination.
 * @param value Byte to write.
 * @param size  Number of bytes.
 */
void
authum_memory_set(authum_ptr dst, authum_byte_t value, authum_usize_t size);

/**
 * @brief Whether the first @p size bytes of @p a and @p b are the same.
 *
 * @return ::authum_bool_false when a pointer is null.
 */
authum_bool_t
authum_memory_is_equal(authum_cptr a, authum_cptr b, authum_usize_t size);

/**
 * @brief The place where @p needle first appears in @p haystack.
 *
 * @param haystack     Bytes to look in.
 * @param size         Number of bytes in @p haystack.
 * @param needle       Bytes to look for.
 * @param needle_size  Number of bytes in @p needle.
 *
 * @return Null when there is none, or when a pointer is null.
 */
const authum_byte_t *
authum_memory_find(authum_cptr haystack, authum_usize_t size, authum_cptr needle,
                   authum_usize_t needle_size);

AUTHUM_EXTERN_C_END

#endif
