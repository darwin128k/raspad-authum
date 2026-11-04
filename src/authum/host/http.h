#ifndef AUTHUM_HOST_HTTP_H
#define AUTHUM_HOST_HTTP_H

#include <authum/http.h>
#include <authum/thread.h>

typedef struct authum_http_owner
{
    authum_http_server_t *head;
    /** The module's mark: it names the owner in the access log. */
    const authum_char_t *tag;
} authum_http_owner_t;

AUTHUM_EXTERN_C_BEGIN

void
authum_http_host_init(authum_http_owner_t *owner, authum_http_service_t *service);

void
authum_http_host_drop_owner(authum_http_owner_t *owner);

/**
 * Joins HTTP workers that a stop gave up waiting for once they have finished
 * on their own (their memory they free themselves). Never blocks: a worker is
 * joined only after it has signalled that it is about to exit. Called on the
 * game thread, each frame. Returns how many detached workers remain.
 */
authum_usize_t
authum_http_host_reap(void);

AUTHUM_EXTERN_C_END

#endif
