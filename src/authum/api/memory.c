#include <authum/memory.h>
#include <authum/null.h>
#include <lh/math/ops.h>
#include <lh/config.h>
#include <lh/memory/std.h>
#include <lh/runtime/allocator.h>
#include <lh/numeric/fixed/limits.h>

authum_ptr
authum_memory_alloc(authum_usize_t size)
{
    if (lh_math_is_zero(size))
    {
        return authum_null;
    }
    return lh_memory_allocator_try_alloc(lh_runtime_allocator(), size);
}

authum_ptr
authum_memory_alloc_zero(authum_usize_t count, authum_usize_t item_size)
{
    if (lh_math_is_zero(count) || lh_math_is_zero(item_size) ||
        lh_math_gt(count, lh_math_div(LH_U32_T_MAX, item_size)))
    {
        return authum_null;
    }
    const authum_usize_t size = lh_math_mul(count, item_size);
    authum_ptr block = authum_memory_alloc(size);
#if LH_LIBRARY_OPTION_MEMORY_ALLOCATOR_INIT_ALLOCATED != LH_LIBRARY_OPTION_ON
    if (!authum_null_eq(block))
    {
        lh_memory_std_set(block, 0, size);
    }
#endif
    return block;
}

authum_ptr
authum_memory_realloc(authum_ptr block, authum_usize_t old_size, authum_usize_t new_size)
{
    if (lh_math_is_zero(new_size))
    {
        authum_memory_free(block);
        return authum_null;
    }
    return lh_memory_allocator_try_realloc(lh_runtime_allocator(), block, old_size, new_size);
}

void
authum_memory_free(authum_ptr block)
{
    lh_runtime_allocator_free(block);
}
