#include <authum/memory.h>
#include <authum/null.h>
#include <lh/config.h>
#include <lh/cast/const.h>
#include <lh/memory/std.h>
#include <lh/runtime/allocator.h>

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

void
authum_memory_copy(authum_ptr dst, authum_cptr src, authum_usize_t size)
{
    if (authum_null_eq(dst) || authum_null_eq(src))
    {
        return;
    }
    lh_memory_std_copy(dst, src, size);
}

void
authum_memory_set(authum_ptr dst, authum_byte_t value, authum_usize_t size)
{
    if (authum_null_eq(dst))
    {
        return;
    }
    lh_memory_std_set(dst, value, size);
}

authum_bool_t
authum_memory_is_equal(authum_cptr a, authum_cptr b, authum_usize_t size)
{
    if (authum_null_eq(a) || authum_null_eq(b))
    {
        return authum_bool_false;
    }
    return lh_null_eq(lh_memory_std_compare(a, b, size)) ? authum_bool_true : authum_bool_false;
}

const authum_byte_t *
authum_memory_find(authum_cptr haystack, authum_usize_t size, authum_cptr needle,
                   authum_usize_t needle_size)
{
    if (authum_null_eq(haystack) || authum_null_eq(needle))
    {
        return authum_null;
    }
    return lh_cast_const(authum_byte_t *, lh_memory_std_find(haystack, size, needle, needle_size));
}
