/**
 * @file sandbox.c
 * @brief ::authum_sandbox_ops_t on lh's ownership tree (`lh_memory_tree`).
 */

#include <authum/host/sandbox.h>
#include <authum/memory.h>
#include <lh/memory/std.h>
#include <authum/unused.h>

#include <lh/memory/allocator/initializer.h>
#include <lh/memory/tree.h>
#include <lh/runtime/allocator.h>
#include <lh/size/limits.h>

/* lh treats a failed allocation as fatal and ends the process. A size no heap
 * could ever give (half the address space or more — a module's arithmetic gone
 * wrong) must not take the server down: refuse it with null instead. */
#define AUTHUM_SANDBOX_SIZE_MAX (LH_USIZE_T_MAX / 2u)

lh_ptr
authum_sandbox_heap_alloc(lh_self_ptr self, lh_usize_t size)
{
    authum_unused(self);
    return authum_memory_alloc(lh_cast_static(authum_usize_t, size));
}

lh_void
authum_sandbox_heap_dealloc(lh_self_ptr self, lh_ptr ptr)
{
    authum_unused(self);
    authum_memory_free(ptr);
}

lh_memory_sized_allocator_t authum_sandbox_heap =
    lh_memory_allocator_initializer(authum_sandbox_heap_alloc, authum_sandbox_heap_dealloc);

authum_ptr
authum_sandbox_host_alloc(authum_ptr parent, authum_usize_t size)
{
    authum_ptr ptr;

    if (lh_null_eq(parent) || lh_math_ge(lh_cast_static(lh_usize_t, size), AUTHUM_SANDBOX_SIZE_MAX))
    {
        return lh_null;
    }
    ptr = lh_memory_tree_alloc_child(parent, lh_cast_static(lh_usize_t, size));
    if (!lh_null_eq(ptr) && lh_math_gt(size, 0u))
    {
        lh_memory_std_set(ptr, 0, lh_cast_static(authum_usize_t, size));
    }
    return ptr;
}

authum_ptr
authum_sandbox_host_realloc(authum_ptr ptr, authum_usize_t size)
{
    if (lh_null_eq(ptr) || lh_math_ge(lh_cast_static(lh_usize_t, size), AUTHUM_SANDBOX_SIZE_MAX))
    {
        return lh_null;
    }
    return lh_memory_tree_realloc(ptr, lh_cast_static(lh_usize_t, size));
}

void
authum_sandbox_host_free(authum_ptr ptr)
{
    lh_memory_tree_free(ptr);
}

void
authum_sandbox_host_set_destructor(authum_ptr ptr, authum_sandbox_destructor_fn fn)
{
    if (lh_null_eq(ptr))
    {
        return;
    }
    lh_memory_tree_set_destructor(ptr, lh_cast_static(lh_memory_tree_destructor_cb, fn));
}

const authum_sandbox_ops_t authum_sandbox_host_ops_table = {
    .alloc = authum_sandbox_host_alloc,
    .realloc = authum_sandbox_host_realloc,
    .free = authum_sandbox_host_free,
    .set_destructor = authum_sandbox_host_set_destructor,
};

const authum_sandbox_ops_t *
authum_sandbox_host_ops(void)
{
    return lh_addr_of(authum_sandbox_host_ops_table);
}

authum_ptr
authum_sandbox_host_root_create(void)
{
    return lh_memory_tree_alloc(lh_addr_of(authum_sandbox_heap), 0u);
}

void
authum_sandbox_host_root_destroy(authum_ptr root)
{
    lh_memory_tree_free(root);
}
