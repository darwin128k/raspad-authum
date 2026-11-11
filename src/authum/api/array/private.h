/**
 * @file private.h
 * @brief Private to `array.c`: the array record and the functions below its
 *        public API.
 *
 * Not installed: modules never see these. An ::authum_array_t is an lh array
 * whose allocator is the module's sandbox: the record is a sandbox block, and
 * the items' storage is allocated under it, so freeing the record frees both.
 */

#ifndef AUTHUM_API_ARRAY_PRIVATE_H
#define AUTHUM_API_ARRAY_PRIVATE_H

#include <authum/array.h>
#include <lh/core/array.h>

struct authum_array
{
    /** The sandbox the record and its storage come from. */
    authum_sandbox_t sandbox;
    /** The sandbox as an lh allocator; its context is the record. */
    lh_memory_allocator_t allocator;
    lh_array_t items;
};

AUTHUM_EXTERN_C_BEGIN

/** @brief The sandbox the array lives in (level 1). */
const authum_sandbox_t *
authum_array_get_sandbox(const authum_array_t *self);

/** @brief Store the sandbox the array lives in (level 1). */
void
authum_array_set_sandbox(authum_array_t *self, const authum_sandbox_t *sandbox);

/** @brief The sandbox as an lh allocator (level 1). */
lh_memory_allocator_t *
authum_array_get_allocator_mut(authum_array_t *self);

/** @brief The lh array of the items (level 1). */
const lh_array_t *
authum_array_get_items(const authum_array_t *self);

/** @brief The lh array of the items, for change (level 1). */
lh_array_t *
authum_array_get_items_mut(authum_array_t *self);

/**
 * @brief ::lh_memory_allocator_alloc_fn over the sandbox: a zeroed block under
 *        the record @p self, so it goes when the record does.
 */
lh_ptr
authum_array_alloc(lh_self_ptr self, lh_usize_t size);

/** @brief ::lh_memory_allocator_realloc_fn over the sandbox; the block keeps its owner. */
lh_ptr
authum_array_realloc(lh_self_ptr self, lh_ptr ptr, lh_usize_t old_size, lh_usize_t new_size);

/** @brief ::lh_memory_allocator_dealloc_fn over the sandbox. */
lh_void
authum_array_dealloc(lh_self_ptr self, lh_ptr ptr);

/**
 * @brief Make the record @p self an empty array of @p item_size-byte items in
 *        @p sandbox, its storage allocated under @p self (level 2).
 */
void
authum_array_init(authum_array_t *self, const authum_sandbox_t *sandbox, authum_usize_t item_size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_ARRAY_PRIVATE_H */
