/**
 * @file address.h
 * @brief The host part of a network address (`host:port`).
 */

#ifndef AUTHUM_NET_ADDRESS_H
#define AUTHUM_NET_ADDRESS_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Copy the host of @p address into @p out.
 *
 * Takes IPv4:port, hostname:port, [IPv6]:port, or a bare host or IPv6 address. No
 * DNS lookup. @p out may be @p address itself.
 *
 * @param address  NUL-terminated address text.
 * @param out      Where the host goes, NUL-terminated.
 * @param capacity Size of @p out in characters, the NUL included.
 * @return False, with @p out empty, for malformed input or a host that does not fit.
 */
authum_bool_t
authum_net_address_copy_host(const authum_char_t *address, authum_char_t *out,
                             authum_usize_t capacity);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_NET_ADDRESS_H */
