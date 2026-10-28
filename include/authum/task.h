#ifndef AUTHUM_TASK_H
#define AUTHUM_TASK_H

#include <authum/abi.h>
#include <authum/bool.h>
#include <authum/extern.h>
#include <authum/ptr.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN
typedef void (*authum_task_fn)(authum_ptr user);

struct authum_task_ops
{
    authum_usize_t size;
    authum_bool_t (*submit)(authum_ptr owner, authum_task_fn work, authum_task_fn complete,
                            authum_task_fn destroy, authum_ptr user);
};
typedef struct authum_task_ops authum_task_ops_t;
struct authum_tasks
{
    const authum_task_ops_t *ops;
    authum_ptr owner;
};
typedef struct authum_tasks authum_tasks_t;

/** Main-thread submission; FIFO worker per module, at most 64 outstanding jobs.
 * work must not call engine, events, commands or sandbox APIs, or share
 * mutable state with the game thread. Logger calls require an initialized
 * authum_logger_buffer capture scope; direct host logging is not thread-safe.
 * complete and destroy run on the game
 * thread. destroy runs exactly once after completion or cancellation. On false,
 * ownership stays with the caller (destroy is not called).
 * Unload cancels queued callbacks and joins active work BEFORE module stop.
 * Thus work must use bounded I/O; unload can wait for its timeout.
 */
authum_bool_t
authum_tasks_submit(const authum_tasks_t *tasks, authum_task_fn work, authum_task_fn complete,
                    authum_task_fn destroy, authum_ptr user);
AUTHUM_EXTERN_C_END
#endif
