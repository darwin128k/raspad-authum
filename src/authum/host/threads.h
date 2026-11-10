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
#include <lh/core/assert/static.h>
#include <lh/os/system.h>
#include <lh/core/str/format/text.h>
#include <lh/core/util/return.h>

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

/**
 * @brief Write @p text as the refusal, when the caller gave room for one.
 */
void
authum_threads_refuse(authum_char_t *error, authum_usize_t error_size, const authum_char_t *text);

/**
 * @brief * Validate owner limits and register a worker only after LH startup succeeds.
 */
authum_thread_t *
authum_threads_host_start(authum_ptr context, authum_thread_fn run, authum_thread_fn wake,
                          authum_ptr user, authum_i32_t cpu, authum_u32_t flags,
                          authum_char_t *error, authum_usize_t error_size);

/**
 * @brief * Stop, join and release a handle belonging to the requesting owner.
 */
authum_bool_t
authum_threads_host_stop(authum_ptr context, authum_thread_t *thread);

AUTHUM_EXTERN_C_END

#endif
