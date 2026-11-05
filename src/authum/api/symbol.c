#include <authum/symbol.h>

#include <lh/core/cast/const.h>
#include <lh/core/memory.h>
#include <lh/os/shared.h>

authum_ptr
authum_symbol_find(const authum_char_t *library, const authum_char_t *name,
                   authum_bool_t *out_via_dynsym)
{
    if (!authum_null_eq(out_via_dynsym))
    {
        lh_ptr_deref(out_via_dynsym) = authum_bool_false;
    }
    if (authum_null_eq(library) || authum_null_eq(name))
    {
        return authum_null;
    }
    /* Only an image already loaded: lh never loads it here. */
    lh_os_shared_t image;
    lh_os_shared_init(lh_addr_of(image));
    if (!lh_os_shared_bind_name(lh_addr_of(image), library))
    {
        lh_os_shared_deinit(lh_addr_of(image));
        return authum_null;
    }
    authum_ptr address = lh_os_shared_get_sym(lh_addr_of(image), name);
    if (!authum_null_eq(address))
    {
        if (!authum_null_eq(out_via_dynsym))
        {
            lh_ptr_deref(out_via_dynsym) = authum_bool_true;
        }
    }
    else
    {
        address = lh_os_shared_get_local_sym(lh_addr_of(image), name);
    }
    lh_os_shared_close(lh_addr_of(image));
    lh_os_shared_deinit(lh_addr_of(image));
    return address;
}

authum_ptr
authum_symbol_find_pattern(const authum_char_t *library, const authum_byte_t *pattern,
                           const authum_byte_t *mask, authum_usize_t size,
                           authum_usize_t *out_count)
{
    if (!authum_null_eq(out_count))
    {
        lh_ptr_deref(out_count) = 0;
    }
    if (authum_null_eq(library) || authum_null_eq(pattern) || authum_null_eq(mask) ||
        lh_math_is_zero(size))
    {
        return authum_null;
    }
    lh_os_shared_t image;
    lh_os_shared_init(lh_addr_of(image));
    if (!lh_os_shared_bind_name(lh_addr_of(image), library))
    {
        lh_os_shared_deinit(lh_addr_of(image));
        return authum_null;
    }
    lh_ptr code = authum_null;
    lh_usize_t code_size = 0;
    authum_ptr first = authum_null;
    authum_usize_t count = 0;
    if (lh_os_shared_try_get_code(lh_addr_of(image), lh_addr_of(code), lh_addr_of(code_size)))
    {
        const authum_byte_t *at = code;
        const authum_byte_t *end = lh_math_add(at, code_size);
        /* Count up to two: one says which, two says the pattern is ambiguous. */
        while (lh_math_lt(count, 2u) && lh_math_ge(lh_cast_static(lh_usize_t, end - at), size))
        {
            const authum_byte_t *hit = lh_memory_find_masked(
                at, lh_cast_static(lh_usize_t, end - at), pattern, mask, size);
            if (authum_null_eq(hit))
            {
                break;
            }
            if (authum_null_eq(first))
            {
                first = lh_cast_const(authum_byte_t *, hit);
            }
            count = lh_math_add_one(count);
            at = lh_math_add_one(hit);
        }
    }
    lh_os_shared_close(lh_addr_of(image));
    lh_os_shared_deinit(lh_addr_of(image));
    if (!authum_null_eq(out_count))
    {
        lh_ptr_deref(out_count) = count;
    }
    return first;
}

authum_ptr
authum_symbol_find_engine(const authum_char_t *name, authum_bool_t *out_via_dynsym)
{
    return authum_symbol_find(AUTHUM_SYMBOL_ENGINE_LIBRARY, name, out_via_dynsym);
}

authum_ptr
authum_symbol_find_engine_pattern(const authum_byte_t *pattern, const authum_byte_t *mask,
                                  authum_usize_t size, authum_usize_t *out_count)
{
    return authum_symbol_find_pattern(AUTHUM_SYMBOL_ENGINE_LIBRARY, pattern, mask, size, out_count);
}
