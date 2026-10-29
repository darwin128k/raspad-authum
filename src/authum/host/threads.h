/**
 * @file threads.h
 * @brief A module's threads, over the LH thread group (`lh/os/thread/group.h`).
 *
 * Authum adds the module ABI and its policy: at most 64 threads a module, and an
 * error text for a refused start.
 */

#ifndef AUTHUM_HOST_THREADS_H
#define AUTHUM_HOST_THREADS_H

#include <authum/thread.h>
#include <lh/os/thread/group.h>

/** @brief The threads one module owns. */
typedef struct authum_thread_owner
{
    lh_os_thread_group_t group; /**< its threads */
} authum_thread_owner_t;

AUTHUM_EXTERN_C_BEGIN

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
