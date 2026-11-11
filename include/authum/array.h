/**
 * @file array.h
 * @brief Growable array of fixed-size items living in an ::authum_sandbox_t.
 *
 * The array is opaque: ::authum_array_create makes it as a block of the
 * sandbox, and its storage hangs under that block, so an array a module
 * forgets to destroy is still released at unload. Items are copied in by
 * value; pointers returned by ::authum_array_get_at / ::authum_array_push stay
 * valid until the next call that grows or shrinks the array.
 *
 * @code
 * authum_array_t *names =
 *     authum_array_create(authum_host_get_sandbox(host), sizeof(mymod_name_t));
 * mymod_name_t *slot = authum_array_push(names, authum_null);  // zeroed item
 * @endcode
 */

#ifndef AUTHUM_ARRAY_H
#define AUTHUM_ARRAY_H

#include <authum/sandbox.h>

/** @brief Opaque growable array; made by ::authum_array_create. */
typedef struct authum_array authum_array_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief An empty array of @p item_size-byte items in @p sandbox; its storage
 *        is not allocated yet.
 *
 * @return The array, or null for an invalid sandbox or a zero @p item_size.
 */
authum_array_t *
authum_array_create(const authum_sandbox_t *sandbox, authum_usize_t item_size);

/** @brief Free the array and its storage now (optional — the sandbox frees both at unload). */
void
authum_array_destroy(authum_array_t *self);

/** @brief Items in the array; 0 for null. */
authum_usize_t
authum_array_get_size(const authum_array_t *self);

/** @brief Items the storage holds before it grows again; 0 for null. */
authum_usize_t
authum_array_get_capacity(const authum_array_t *self);

/** @brief Whether the array has no items (a null array has none). */
authum_bool_t
authum_array_is_empty(const authum_array_t *self);

/** @brief Ensure room for @p capacity items without further allocation. */
authum_bool_t
authum_array_reserve(authum_array_t *self, authum_usize_t capacity);

/**
 * @brief Append a copy of @p item (null @p item → zeroed item).
 * @return The new item, or null when out of memory.
 */
authum_ptr
authum_array_push(authum_array_t *self, authum_cptr item);

/** @brief Item @p index, or null when out of range. */
authum_ptr
authum_array_get_at(const authum_array_t *self, authum_usize_t index);

/** @brief Remove item @p index, keeping the order of the rest. */
authum_bool_t
authum_array_remove_at(authum_array_t *self, authum_usize_t index);

/** @brief Drop all items, keep capacity. */
void
authum_array_clear(authum_array_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_ARRAY_H */
