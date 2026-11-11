/**
 * @file array.c
 * @brief ::authum_array_t — an lh array over the module's region.
 */

#include <authum/api/array/private.h>
#include <lh/core/memory/std.h>

#include <authum/null.h>
#include <authum/unused.h>

/* ── accessors ───────────────────────────────────────────────────────────── */

const authum_region_t *
authum_array_get_region(const authum_array_t *self)
{
    return lh_addr_of(self->region);
}

void
authum_array_set_region(authum_array_t *self, const authum_region_t *region)
{
    self->region = lh_ptr_deref(region);
}

lh_memory_allocator_t *
authum_array_get_allocator_mut(authum_array_t *self)
{
    return lh_addr_of(self->allocator);
}

const lh_array_t *
authum_array_get_items(const authum_array_t *self)
{
    return lh_addr_of(self->items);
}

lh_array_t *
authum_array_get_items_mut(authum_array_t *self)
{
    return lh_addr_of(self->items);
}

lh_ptr
authum_array_alloc(lh_self_ptr self, lh_usize_t size)
{
    authum_array_t *array = lh_ptr_cast(authum_array_t, self);
    return authum_region_alloc_under(authum_array_get_region(array), array, size);
}

lh_ptr
authum_array_realloc(lh_self_ptr self, lh_ptr ptr, lh_usize_t old_size, lh_usize_t new_size)
{
    authum_unused(old_size);
    return authum_region_realloc(authum_array_get_region(lh_ptr_cast(authum_array_t, self)), ptr,
                                 new_size);
}

lh_void
authum_array_dealloc(lh_self_ptr self, lh_ptr ptr)
{
    authum_region_free(authum_array_get_region(lh_ptr_cast(authum_array_t, self)), ptr);
}

void
authum_array_destroy(authum_array_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    lh_array_deinit(authum_array_get_items_mut(self));
    /* A copy: the record that holds the region goes with it. */
    authum_region_t region = lh_ptr_deref(authum_array_get_region(self));
    authum_region_free(lh_addr_of(region), self);
}

authum_usize_t
authum_array_get_size(const authum_array_t *self)
{
    return authum_null_eq(self) ? 0u : lh_array_get_size(authum_array_get_items(self));
}

authum_usize_t
authum_array_get_capacity(const authum_array_t *self)
{
    return authum_null_eq(self) ? 0u : lh_array_get_capacity(authum_array_get_items(self));
}

authum_bool_t
authum_array_reserve(authum_array_t *self, authum_usize_t capacity)
{
    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    return lh_array_try_reserve(authum_array_get_items_mut(self), capacity);
}

authum_ptr
authum_array_push(authum_array_t *self, authum_cptr item)
{
    if (authum_null_eq(self))
    {
        return authum_null;
    }
    lh_array_t *items = authum_array_get_items_mut(self);
    const lh_usize_t size = lh_array_get_size(items);
    /* lh's growth policy, without lh_array_push_back's stop on a failed growth. */
    if (!lh_array_try_reserve(items, lh_array_get_grown_capacity(lh_array_get_capacity(items),
                                                                 lh_math_add_one(size))))
    {
        return authum_null;
    }
    authum_ptr slot = lh_array_get_end(items);
    if (authum_null_eq(item))
    {
        lh_memory_std_set(slot, 0, lh_array_get_type_size(items));
    }
    else
    {
        lh_memory_std_copy(slot, item, lh_array_get_type_size(items));
    }
    lh_array_set_size(items, lh_math_add_one(size));
    return slot;
}

authum_ptr
authum_array_get_at(const authum_array_t *self, authum_usize_t index)
{
    if (authum_null_eq(self) || !lh_array_is_valid_index(authum_array_get_items(self), index))
    {
        return authum_null;
    }
    return lh_array_get_ptr(authum_array_get_items(self), index);
}

authum_bool_t
authum_array_remove_at(authum_array_t *self, authum_usize_t index)
{
    if (authum_null_eq(self) || !lh_array_is_valid_index(authum_array_get_items(self), index))
    {
        return authum_bool_false;
    }
    lh_array_erase_of(authum_array_get_items_mut(self), index, 1u);
    return authum_bool_true;
}

void
authum_array_clear(authum_array_t *self)
{
    if (!authum_null_eq(self))
    {
        lh_array_clear(authum_array_get_items_mut(self));
    }
}

void
authum_array_init(authum_array_t *self, const authum_region_t *region, authum_usize_t item_size)
{
    authum_array_set_region(self, region);
    lh_memory_allocator_t *allocator = authum_array_get_allocator_mut(self);
    lh_memory_allocator_init(allocator, authum_array_alloc, authum_array_dealloc);
    lh_memory_allocator_set_realloc_cb(allocator, authum_array_realloc);
    lh_memory_allocator_set_context(allocator, self);
    lh_array_init_by(authum_array_get_items_mut(self), item_size, allocator);
}

authum_bool_t
authum_array_is_empty(const authum_array_t *self)
{
    return lh_math_is_zero(authum_array_get_size(self));
}

authum_array_t *
authum_array_create(const authum_region_t *region, authum_usize_t item_size)
{
    if (!authum_region_is_valid(region) || lh_math_eq(item_size, 0u))
    {
        return authum_null;
    }
    authum_array_t *self =
        lh_ptr_cast(authum_array_t, authum_region_alloc(region, sizeof(authum_array_t)));
    if (authum_null_eq(self))
    {
        return authum_null;
    }
    authum_array_init(self, region, item_size);
    return self;
}
