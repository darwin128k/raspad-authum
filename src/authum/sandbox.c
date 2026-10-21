/**
 * @file sandbox.c
 * @brief ::authum_sandbox_ops_t on lh's ownership tree (`lh_memory_tree`).
 */

#include <authum/sandbox/host.h>

#include <lh/memory/allocator/initializer.h>
#include <lh/memory/tree.h>
#include <lh/null.h>
#include <lh/runtime/allocator.h>

#include <string.h>

lh_ptr
authum_sandbox_heap_alloc(lh_self_ptr self, lh_usize_t size)
{
    (void)self;
    return lh_runtime_allocator_alloc(size);
}

lh_void
authum_sandbox_heap_dealloc(lh_self_ptr self, lh_ptr ptr)
{
    (void)self;
    lh_runtime_allocator_free(ptr);
}

lh_memory_sized_allocator_t authum_sandbox_heap =
    lh_memory_allocator_initializer(authum_sandbox_heap_alloc, authum_sandbox_heap_dealloc);

authum_ptr
authum_sandbox_host_alloc(authum_ptr parent, authum_usize_t size)
{
    authum_ptr ptr;

    if (lh_null_eq(parent))
    {
        return lh_null;
    }
    ptr = lh_memory_tree_alloc_child(parent, (lh_usize_t)size);
    if (!lh_null_eq(ptr) && size > 0u)
    {
        memset(ptr, 0, (size_t)size);
    }
    return ptr;
}

authum_ptr
authum_sandbox_host_realloc(authum_ptr ptr, authum_usize_t size)
{
    if (lh_null_eq(ptr))
    {
        return lh_null;
    }
    return lh_memory_tree_realloc(ptr, (lh_usize_t)size);
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
    lh_memory_tree_set_destructor(ptr, (lh_memory_tree_destructor_cb)fn);
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
    return &authum_sandbox_host_ops_table;
}

authum_ptr
authum_sandbox_host_root_create(void)
{
    return lh_memory_tree_alloc(&authum_sandbox_heap, 0u);
}

void
authum_sandbox_host_root_destroy(authum_ptr root)
{
    lh_memory_tree_free(root);
}
