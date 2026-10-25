#ifndef AUTHUM_HOST_HTTP_H
#define AUTHUM_HOST_HTTP_H
#include <authum/http.h>
#include <authum/thread.h>
AUTHUM_EXTERN_C_BEGIN

typedef struct authum_http_owner
{
    authum_http_server_t *head;
    const authum_threads_t *threads;
} authum_http_owner_t;

void
authum_http_host_init(authum_http_owner_t *owner, const authum_threads_t *threads,
                      authum_http_service_t *service);
void
authum_http_host_drop_owner(authum_http_owner_t *owner);
AUTHUM_EXTERN_C_END
#endif
