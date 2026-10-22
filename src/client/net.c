#include <authum/net.h>

#include <authum/null.h>

#include <stdio.h>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#endif

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

authum_usize_t
authum_net_ipv4_collect(authum_bool_t external, authum_u32_t *out, authum_usize_t max)
{
#if defined(_WIN32)
    (void)external;
    (void)out;
    (void)max;
    return 0u;
#else
    struct ifaddrs *list;
    struct ifaddrs *it;
    authum_usize_t count = 0u;

    if (getifaddrs(&list) != 0)
    {
        return 0u;
    }
    for (it = list; it; it = it->ifa_next)
    {
        authum_u32_t ip;
        authum_usize_t i;
        authum_bool_t seen = authum_bool_false;

        if (!it->ifa_addr || it->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }
        ip = (authum_u32_t)ntohl(((const struct sockaddr_in *)it->ifa_addr)->sin_addr.s_addr);
        if (authum_net_ipv4_is_loopback(ip) || authum_net_ipv4_is_external(ip) != external)
        {
            continue;
        }
        /* Aliases repeat an address; count it once (only what fits is checked). */
        for (i = 0u; i < count && i < max; i++)
        {
            if (out[i] == ip)
            {
                seen = authum_bool_true;
                break;
            }
        }
        if (seen)
        {
            continue;
        }
        if (count < max && !authum_null_eq(out))
        {
            out[count] = ip;
        }
        count++;
    }
    freeifaddrs(list);
    return count;
#endif
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
