#ifndef AUTHUM_HOST_THREADS_H
#define AUTHUM_HOST_THREADS_H
#include <authum/thread.h>
AUTHUM_EXTERN_C_BEGIN
typedef struct authum_thread_owner {
    authum_thread_t *head;
    unsigned count;
    int closing;
} authum_thread_owner_t;
void authum_threads_host_init(authum_thread_owner_t *owner,authum_threads_t *facade);
void authum_threads_host_stop_owner(authum_thread_owner_t *owner);
void authum_threads_host_drop_owner(authum_thread_owner_t *owner);
AUTHUM_EXTERN_C_END
#endif
