/**
 * @file address.c
 * @brief The host of a network address, over ::lh_net_authority_try_split.
 */

#include <authum/net/address.h>
#include <authum/null.h>
#include <lh/core/char/map.h>
#include <lh/core/memory/std.h>
#include <lh/core/net/authority.h>

authum_bool_t
authum_net_address_copy_host(const authum_char_t *address, authum_char_t *out,
                             authum_usize_t capacity)
{
    if (authum_null_eq(out) || lh_math_eq(capacity, 0u))
    {
        return authum_bool_false;
    }
    lh_str_view_t host;
    lh_net_port_t port;
    if (authum_null_eq(address) ||
        !lh_net_authority_try_split(address, lh_str_ptr_len(address), lh_addr_of(host),
                                    lh_addr_of(port), authum_null) ||
        lh_math_ge(lh_str_view_get_size(lh_addr_of(host)), capacity))
    {
        lh_str_ptr_clear(out);
        return authum_bool_false;
    }
    /* @p out may be @p address itself: the host moves left, never right. */
    lh_memory_std_move(out, lh_str_view_get_data(lh_addr_of(host)),
                       lh_str_view_get_size(lh_addr_of(host)));
    lh_str_ptr_erase(out, lh_str_view_get_size(lh_addr_of(host)));
    return authum_bool_true;
}
