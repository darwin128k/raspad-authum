#ifndef AUTHUM_CLIENT_REF_H
#define AUTHUM_CLIENT_REF_H

#include <authum/host.h>

AUTHUM_EXTERN_C_BEGIN
typedef struct authum_clients
{
    authum_ptr impl;
} authum_clients_t;
typedef struct authum_client_ref
{
    authum_i32_t index, userid;
    authum_u64_t generation, lifetime;
    authum_ptr tracker;
} authum_client_ref_t;

/** Main-thread tracker; initialize before subscribing your own player handlers.
 * Lifecycle events update generations automatically, including map boundaries.
 * Zero initialize, destroy before module stop; jobs must already be drained.
 */
authum_bool_t
authum_clients_init(authum_clients_t *self, const authum_host_t *host, authum_usize_t max_clients);

void
authum_clients_destroy(authum_clients_t *self);

authum_bool_t
authum_clients_capture(const authum_clients_t *self, authum_i32_t index, authum_client_ref_t *out);

/** Game thread only. Rejects old slots, same-userid reuse, old maps and trackers. */
authum_bool_t
authum_client_ref_is_current(const authum_clients_t *self, const authum_client_ref_t *ref);

AUTHUM_EXTERN_C_END
#endif
