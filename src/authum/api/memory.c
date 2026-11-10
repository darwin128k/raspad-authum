#include <authum/memory.h>
#include <authum/null.h>
#include <lh/core/cast/const.h>
#include <lh/core/memory/std.h>
#include <lh/core/runtime/allocator.h>

authum_ptr
authum_memory_alloc(authum_usize_t size)
{
    /* Authum's own policy: a zero size is no block. */
    return lh_math_is_zero(size) ? authum_null : lh_runtime_allocator_try_alloc(size);
}

authum_ptr
authum_memory_alloc_zero(authum_usize_t count, authum_usize_t item_size)
{
    /* lh checks the product for overflow and zeroes the block. */
    return lh_math_is_zero(count) || lh_math_is_zero(item_size)
               ? authum_null
               : lh_runtime_allocator_try_alloc_zero(count, item_size);
}

authum_ptr
authum_memory_realloc(authum_ptr block, authum_usize_t old_size, authum_usize_t new_size)
{
    if (lh_math_is_zero(new_size))
    {
        authum_memory_free(block);
        return authum_null;
    }
    return lh_runtime_allocator_try_realloc(block, old_size, new_size);
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
