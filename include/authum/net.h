/**
 * @file net.h
 * @brief This machine's IPv4 addresses: internal (private) and external (public).
 *
 * HLDS bound to every interface (no `+ip`) does not know its own address —
 * the engine reports `0.0.0.0`. Modules that must name the server's address
 * (monitoring replies, Vellum tickets) read it from the interfaces here.
 *
 * Addresses are `authum_u32_t` in host byte order (`1.2.3.4` = `0x01020304`).
 * Loopback is never listed. "External" means routable on the internet; behind
 * NAT the machine has none, and the caller has to be told the address.
 * Linux (`getifaddrs`) and Windows (`GetAdaptersAddresses`, adapters that are up).
 */

#ifndef AUTHUM_NET_H
#define AUTHUM_NET_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/size.h>

/** Room for "255.255.255.255" and the NUL. */
#define AUTHUM_NET_IPV4_TEXT_SIZE 16

AUTHUM_EXTERN_C_BEGIN

/**
 * True for addresses routable on the internet: not loopback, private
 * (10/8, 172.16/12, 192.168/16), CGNAT (100.64/10), link-local,
 * benchmark (198.18/15), 0/8, 192.0.0/24, multicast or reserved (224/3).
 */
authum_bool_t
authum_net_ipv4_is_external(authum_u32_t ip);

/** True for 127.0.0.0/8. */
authum_bool_t
authum_net_ipv4_is_loopback(authum_u32_t ip);

/**
 * External (public) IPv4 addresses on this machine's interfaces, each once,
 * in interface order. Writes up to @p max into @p out and returns how many
 * exist (may be more than @p max). 0 = none (e.g. behind NAT).
 */
authum_usize_t
authum_net_ipv4_external(authum_u32_t *out, authum_usize_t max);

/** Same for internal addresses: non-loopback and not external (LAN / NAT side). */
authum_usize_t
authum_net_ipv4_internal(authum_u32_t *out, authum_usize_t max);

/** Dotted text into @p buf (at least ::AUTHUM_NET_IPV4_TEXT_SIZE). */
authum_bool_t
authum_net_ipv4_format(authum_u32_t ip, authum_char_t *buf, authum_usize_t buf_size);

/** Shared walk behind the two lists: @p external selects which half. */
authum_usize_t
authum_net_ipv4_collect(authum_bool_t external, authum_u32_t *out, authum_usize_t max);

/** Extract host from IPv4:port, hostname:port, [IPv6]:port or a bare host/IPv6.
 * No DNS lookup. False (and empty output) for malformed/truncated input.
 */
authum_bool_t authum_net_address_host(const authum_char_t *address,
                                     authum_char_t *out, authum_usize_t capacity);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_NET_H */
