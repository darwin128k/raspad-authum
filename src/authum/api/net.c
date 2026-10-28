#include <authum/net.h>
#include <authum/unused.h>

#include <authum/null.h>

#include <lh/os/net/interface.h>

#include <stdio.h>

authum_bool_t
authum_net_ipv4_is_loopback(authum_u32_t ip)
{
    return lh_math_eq((lh_math_bit_and(ip, 0xff000000u)), 0x7f000000u) ? authum_bool_true
                                                                       : authum_bool_false;
}

authum_bool_t
authum_net_ipv4_is_external(authum_u32_t ip)
{
    static const struct
    {
        authum_u32_t net;
        authum_u32_t mask;
    } reserved[] = {
        {0x00000000u, 0xff000000u}, /* 0.0.0.0/8 */
        {0x0a000000u, 0xff000000u}, /* 10.0.0.0/8 */
        {0x64400000u, 0xffc00000u}, /* 100.64.0.0/10 CGNAT */
        {0x7f000000u, 0xff000000u}, /* 127.0.0.0/8 */
        {0xa9fe0000u, 0xffff0000u}, /* 169.254.0.0/16 link-local */
        {0xac100000u, 0xfff00000u}, /* 172.16.0.0/12 */
        {0xc0000000u, 0xffffff00u}, /* 192.0.0.0/24 */
        {0xc0a80000u, 0xffff0000u}, /* 192.168.0.0/16 */
        {0xc6120000u, 0xfffe0000u}, /* 198.18.0.0/15 benchmark */
        {0xe0000000u, 0xe0000000u}, /* 224.0.0.0/3 multicast + reserved */
    };

    authum_usize_t i;

    for (i = 0u; lh_math_lt(i, lh_math_div(sizeof reserved, sizeof reserved[0]));
         i = lh_math_add_one(i))
    {
        if (lh_math_eq((lh_math_bit_and(ip, reserved[i].mask)), reserved[i].net))
        {
            return authum_bool_false;
        }
    }
    return authum_bool_true;
}

authum_bool_t
authum_net_ipv4_format(authum_u32_t ip, authum_char_t *buf, authum_usize_t buf_size)
{
    authum_i32_t written;

    if (authum_null_eq(buf) || lh_math_eq(buf_size, 0u))
    {
        return authum_bool_false;
    }
    written = snprintf(
        buf, buf_size, "%u.%u.%u.%u", lh_cast_static(authum_u32_t, (lh_math_bit_shr(ip, 24))),
        lh_cast_static(authum_u32_t, (lh_math_bit_and((lh_math_bit_shr(ip, 16)), 0xffu))),
        lh_cast_static(authum_u32_t, (lh_math_bit_and((lh_math_bit_shr(ip, 8)), 0xffu))),
        lh_cast_static(authum_u32_t, (lh_math_bit_and(ip, 0xffu))));
    return lh_math_gt(written, 0) && lh_math_lt(lh_cast_static(authum_usize_t, written), buf_size)
               ? authum_bool_true
               : authum_bool_false;
}

/** What ::authum_net_ipv4_collect gathers while the OS lists addresses. */
typedef struct authum_net_ipv4_list
{
    authum_bool_t external;
    authum_u32_t *out;
    authum_usize_t max;
    authum_usize_t count;
} authum_net_ipv4_list_t;

void
authum_net_ipv4_add(lh_ptr user, const lh_net_ip4_t *address)
{
    authum_net_ipv4_list_t *list = lh_cast_reinterpret(authum_net_ipv4_list_t *, user);
    /* Host byte order, the way the rest of <authum/net.h> holds an address. */
    const authum_u32_t ip = lh_math_bit_or(
        lh_math_bit_or(
            lh_math_bit_or((lh_math_bit_shl(lh_cast_static(authum_u32_t, address->octets[0]), 24)),
                           (lh_math_bit_shl(lh_cast_static(authum_u32_t, address->octets[1]), 16))),
            (lh_math_bit_shl(lh_cast_static(authum_u32_t, address->octets[2]), 8))),
        lh_cast_static(authum_u32_t, address->octets[3]));
    authum_usize_t i;

    if (authum_net_ipv4_is_loopback(ip) ||
        lh_math_ne(authum_net_ipv4_is_external(ip), list->external))
    {
        return;
    }
    /* Aliases repeat an address; count it once (only what fits is checked). */
    for (i = 0u; lh_math_lt(i, list->count) && lh_math_lt(i, list->max); i = lh_math_add_one(i))
    {
        if (lh_math_eq(list->out[i], ip))
        {
            return;
        }
    }
    if (lh_math_lt(list->count, list->max) && !authum_null_eq(list->out))
    {
        list->out[list->count] = ip;
    }
    list->count = lh_math_add_one(list->count);
}

authum_usize_t
authum_net_ipv4_collect(authum_bool_t external, authum_u32_t *out, authum_usize_t max)
{
    authum_net_ipv4_list_t list;

    list.external = external;
    list.out = out;
    list.max = authum_null_eq(out) ? 0u : max;
    list.count = 0u;
    authum_unused(lh_os_net_interface_each_ip4(authum_net_ipv4_add, lh_addr_of(list)));
    return list.count;
}

authum_usize_t
authum_net_ipv4_external(authum_u32_t *out, authum_usize_t max)
{
    return authum_net_ipv4_collect(authum_bool_true, out, max);
}

authum_usize_t
authum_net_ipv4_internal(authum_u32_t *out, authum_usize_t max)
{
    return authum_net_ipv4_collect(authum_bool_false, out, max);
}
