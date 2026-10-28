#include <authum/net.h>
#include <authum/null.h>
#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/size.h>
#include <lh/cast/static.h>
#include <lh/char/map.h>
#include <lh/math/ops.h>
#include <lh/util/ptr.h>
#include <lh/net/port.h>
#include <lh/char/digit.h>
#include <lh/memory/std.h>
#include <string.h>

authum_bool_t
authum_net_address_host(const authum_char_t *address, authum_char_t *out, authum_usize_t capacity)
{
    const authum_char_t *begin = address, *end, *port = authum_null, *p;
    authum_u32_t colons = 0, number = 0;
    authum_usize_t length;
    if (!out || !capacity)
    {
        return authum_bool_false;
    }

    if (!address || !address[0])
    {
        goto invalid;
    }
    end = lh_math_add(address, strlen(address));
    if (lh_math_eq(lh_ptr_deref(begin), lh_char_map_lbracket))
    {
        begin = lh_math_add_one(begin);
        end = strchr(begin, lh_char_map_rbracket);
        if (!end || lh_math_eq(end, begin) || (end[1] && lh_math_ne(end[1], lh_char_map_colon)))
        {
            goto invalid;
        }
        if (end[1])
        {
            port = lh_math_add(end, 2);
        }
    }
    else
    {
        for (p = begin; lh_math_lt(p, end); p = lh_math_add_one(p))
        {
            if (lh_math_eq(lh_ptr_deref(p), lh_char_map_colon))
            {
                colons = lh_math_add_one(colons);
            }
        }
        if (lh_math_eq(colons, 1))
        {
            end = strchr(begin, lh_char_map_colon);
            port = lh_math_add(end, 1);
        }
    }
    if (port)
    {
        if (!lh_ptr_deref(port))
        {
            goto invalid;
        }
        for (p = port; lh_ptr_deref(p); p = lh_math_add_one(p))
        {
            if (!lh_char_is_digit(lh_ptr_deref(p)) ||
                !lh_char_digit_accumulate(lh_addr_of(number), lh_char_to_digit(lh_ptr_deref(p))))
            {
                goto invalid;
            }
            if (lh_math_gt(number, LH_NET_PORT_MAX))
            {
                goto invalid;
            }
        }
    }
    for (p = begin; lh_math_lt(p, end); p = lh_math_add_one(p))
    {
        if (lh_math_le(lh_cast_static(authum_byte_t, lh_ptr_deref(p)), lh_char_map_space) ||
            lh_math_eq(lh_ptr_deref(p), lh_char_map_lbracket) ||
            lh_math_eq(lh_ptr_deref(p), lh_char_map_rbracket))
        {
            goto invalid;
        }
    }
    length = lh_cast_static(authum_usize_t, (lh_math_sub(end, begin)));
    if (!length || lh_math_ge(length, capacity))
    {
        goto invalid;
    }
    lh_memory_std_move(out, begin, length);
    out[length] = lh_char_map_nul;
    return authum_bool_true;
invalid:
    out[0] = lh_char_map_nul;
    return authum_bool_false;
}
