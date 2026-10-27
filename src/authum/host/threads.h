#ifndef AUTHUM_HOST_THREADS_H
#define AUTHUM_HOST_THREADS_H

#include <authum/thread.h>

AUTHUM_EXTERN_C_BEGIN

typedef struct authum_thread_owner
{
    authum_thread_t *head;
    unsigned count;
    int closing;
} authum_thread_owner_t;

/**
 * @brief Initialize owner state and bind the public thread facade before first use.
 */
void
authum_threads_host_init(authum_thread_owner_t *owner, authum_threads_t *facade);

/**
 * @brief Request stop for every worker before joining any; null owners are ignored.
 */
void
authum_threads_host_stop_owner(authum_thread_owner_t *owner);

/**
 * @brief Stop all owner workers and release their remaining host handles.
 */
void
authum_threads_host_drop_owner(authum_thread_owner_t *owner);

AUTHUM_EXTERN_C_END

#endif
