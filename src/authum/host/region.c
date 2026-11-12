/**
 * @file region.c
 * @brief ::authum_region_ops_t on lh's ownership tree (`lh_memory_tree`).
 */

#include <authum/host/region.h>
#include <authum/memory.h>
#include <lh/core/memory/std.h>
#include <authum/unused.h>

#include <lh/core/memory/allocator/initializer.h>
#include <lh/core/memory/tree.h>
#include <lh/core/runtime/allocator.h>
#include <lh/core/size/limits.h>

/* lh treats a failed allocation as fatal and ends the process. A size no heap
 * could ever give (half the address space or more — a module's arithmetic gone
 * wrong) must not take the server down: refuse it with null instead. */
#define AUTHUM_REGION_SIZE_MAX (LH_USIZE_T_MAX / 2u)

lh_ptr
authum_region_heap_alloc(lh_self_ptr self, lh_usize_t size)
{
    authum_unused(self);
    return authum_memory_alloc(lh_cast_static(authum_usize_t, size));
}

lh_void
authum_region_heap_dealloc(lh_self_ptr self, lh_ptr ptr)
{
    authum_unused(self);
    authum_memory_free(ptr);
}

lh_memory_sized_allocator_t authum_region_heap =
    lh_memory_allocator_initializer(authum_region_heap_alloc, authum_region_heap_dealloc);

authum_ptr
authum_region_host_alloc(authum_ptr parent, authum_usize_t size)
{
    if (lh_null_eq(parent) || lh_math_ge(lh_cast_static(lh_usize_t, size), AUTHUM_REGION_SIZE_MAX))
    {
        return lh_null;
    }
    authum_ptr ptr = lh_memory_tree_alloc_child(parent, lh_cast_static(lh_usize_t, size));
    if (!lh_null_eq(ptr) && lh_math_gt(size, 0u))
    {
        lh_memory_std_set(ptr, 0, lh_cast_static(authum_usize_t, size));
    }
    return ptr;
}

authum_ptr
authum_region_host_realloc(authum_ptr ptr, authum_usize_t size)
{
    if (lh_null_eq(ptr) || lh_math_ge(lh_cast_static(lh_usize_t, size), AUTHUM_REGION_SIZE_MAX))
    {
        return lh_null;
    }
    return lh_memory_tree_realloc(ptr, lh_cast_static(lh_usize_t, size));
}

void
authum_region_host_free(authum_ptr ptr)
{
    lh_memory_tree_free(ptr);
}

void
authum_region_host_set_destructor(authum_ptr ptr, authum_region_destructor_fn fn)
{
    if (lh_null_eq(ptr))
    {
        return;
    }
    lh_memory_tree_set_destructor(ptr, lh_cast_static(lh_memory_tree_destructor_cb, fn));
}

const authum_region_ops_t authum_region_host_ops_table = {
    .alloc = authum_region_host_alloc,
    .realloc = authum_region_host_realloc,
    .free = authum_region_host_free,
    .set_destructor = authum_region_host_set_destructor,
};

const authum_region_ops_t *
authum_region_host_ops(void)
{
    return lh_addr_of(authum_region_host_ops_table);
}

authum_ptr
authum_region_host_root_create(void)
{
    return lh_memory_tree_alloc(lh_addr_of(authum_region_heap), 0u);
}

void
authum_region_host_root_destroy(authum_ptr root)
{
    lh_memory_tree_free(root);
}
