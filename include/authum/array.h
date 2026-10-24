/**
 * @file array.h
 * @brief Growable array of fixed-size items living in an ::authum_sandbox_t.
 *
 * Storage is a sandbox block, so an array a module forgets to deinit is still
 * released at unload. Items are copied in by value; pointers returned by
 * ::authum_array_at / ::authum_array_push stay valid until the next call that
 * grows or shrinks the array.
 *
 * @code
 * authum_array_t names;
 * authum_array_init(&names, authum_host_sandbox(host), sizeof(mymod_name_t));
 * mymod_name_t *slot = authum_array_push(&names, authum_null);  // zeroed item
 * @endcode
 */

#ifndef AUTHUM_ARRAY_H
#define AUTHUM_ARRAY_H

#include <authum/bool.h>
#include <authum/extern.h>
#include <authum/ptr.h>
#include <authum/sandbox.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

struct authum_array
{
    authum_sandbox_t sandbox;
    authum_ptr data;
    authum_usize_t size;
    authum_usize_t capacity;
    authum_usize_t item_size;
};
typedef struct authum_array authum_array_t;

/** Empty array of @p item_size-byte items; allocates nothing yet. */
authum_bool_t
authum_array_init(authum_array_t *self, const authum_sandbox_t *sandbox,
                  authum_usize_t item_size);

/** Free storage now (optional — the sandbox frees it at unload anyway). */
void
authum_array_deinit(authum_array_t *self);

authum_usize_t
authum_array_size(const authum_array_t *self);

authum_bool_t
authum_array_is_empty(const authum_array_t *self);

/** Ensure room for @p capacity items without further allocation. */
authum_bool_t
authum_array_reserve(authum_array_t *self, authum_usize_t capacity);

/**
 * Append a copy of @p item (null @p item → zeroed item).
 * @return The new item, or null when out of memory.
 */
authum_ptr
authum_array_push(authum_array_t *self, authum_cptr item);

/** Item @p index, or null when out of range. */
authum_ptr
authum_array_at(const authum_array_t *self, authum_usize_t index);

/** Remove item @p index, keeping the order of the rest. */
authum_bool_t
authum_array_remove_at(authum_array_t *self, authum_usize_t index);

/** Drop all items, keep capacity. */
void
authum_array_clear(authum_array_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_ARRAY_H */
