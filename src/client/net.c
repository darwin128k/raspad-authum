#include <authum/net.h>

#include <authum/null.h>

#include "os/os.h"

#include <stdio.h>

authum_bool_t
authum_net_ipv4_is_loopback(authum_u32_t ip)
{
    return (ip & 0xff000000u) == 0x7f000000u ? authum_bool_true : authum_bool_false;
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

    for (i = 0u; i < sizeof reserved / sizeof reserved[0]; i++)
    {
        if ((ip & reserved[i].mask) == reserved[i].net)
        {
            return authum_bool_false;
        }
    }
    return authum_bool_true;
}

authum_bool_t
authum_net_ipv4_format(authum_u32_t ip, authum_char_t *buf, authum_usize_t buf_size)
{
    int written;

    if (authum_null_eq(buf) || buf_size == 0u)
    {
        return authum_bool_false;
    }
    written = snprintf(buf, buf_size, "%u.%u.%u.%u", (unsigned)(ip >> 24),
                       (unsigned)((ip >> 16) & 0xffu), (unsigned)((ip >> 8) & 0xffu),
                       (unsigned)(ip & 0xffu));
    return written > 0 && (authum_usize_t)written < buf_size ? authum_bool_true
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
authum_net_ipv4_add(authum_u32_t ip, authum_ptr user)
{
    authum_net_ipv4_list_t *list = (authum_net_ipv4_list_t *)user;
    authum_usize_t i;

    if (authum_net_ipv4_is_loopback(ip) || authum_net_ipv4_is_external(ip) != list->external)
    {
        return;
    }
    /* Aliases repeat an address; count it once (only what fits is checked). */
    for (i = 0u; i < list->count && i < list->max; i++)
    {
        if (list->out[i] == ip)
        {
            return;
        }
    }
    if (list->count < list->max && !authum_null_eq(list->out))
    {
        list->out[list->count] = ip;
    }
    list->count++;
}

authum_usize_t
authum_net_ipv4_collect(authum_bool_t external, authum_u32_t *out, authum_usize_t max)
{
    authum_net_ipv4_list_t list;

    list.external = external;
    list.out = out;
    list.max = authum_null_eq(out) ? 0u : max;
    list.count = 0u;
    authum_os_ipv4_each(authum_net_ipv4_add, &list);
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
