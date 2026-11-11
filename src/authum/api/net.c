#include <authum/net.h>
#include <authum/unused.h>

#include <authum/null.h>

#include <lh/os/net/interface.h>
#include <lh/core/char/map.h>
#include <lh/core/util/str/ptr.h>

authum_bool_t
authum_net_ipv4_is_loopback(authum_u32_t ip)
{
    lh_net_ip4_t address;

    lh_net_ip4_init_by_u32(lh_addr_of(address), ip);
    return lh_net_ip4_is_loopback(lh_addr_of(address));
}

authum_bool_t
authum_net_ipv4_is_external(authum_u32_t ip)
{
    lh_net_ip4_t address;

    lh_net_ip4_init_by_u32(lh_addr_of(address), ip);
    return lh_net_ip4_is_public(lh_addr_of(address));
}

authum_bool_t
authum_net_ipv4_format(authum_u32_t ip, authum_char_t *buf, authum_usize_t buf_size)
{
    if (authum_null_eq(buf) || lh_math_eq(buf_size, 0u))
    {
        return authum_bool_false;
    }

    lh_net_ip4_t address;

    lh_net_ip4_init_by_u32(lh_addr_of(address), ip);

    const lh_usize_t written =
        lh_net_ip4_format(lh_addr_of(address), buf, lh_math_sub(buf_size, 1u));

    lh_str_ptr_erase(buf, written);
    return lh_math_gt(written, 0u) ? authum_bool_true : authum_bool_false;
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
    const authum_u32_t ip = lh_net_ip4_to_u32(address);
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
authum_net_ipv4_collect_external(authum_u32_t *out, authum_usize_t max)
{
    return authum_net_ipv4_collect(authum_bool_true, out, max);
}

authum_usize_t
authum_net_ipv4_collect_internal(authum_u32_t *out, authum_usize_t max)
{
    return authum_net_ipv4_collect(authum_bool_false, out, max);
}
