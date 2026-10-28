#ifndef AUTHUM_HOST_TASKS_H
#define AUTHUM_HOST_TASKS_H

#include <authum/task.h>
#include <lh/util/ptr.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Return the immutable host background-task operations table.
 */
const authum_task_ops_t lh_ptr_deref(authum_tasks_host_ops(void));

/**
 * @brief Deliver a bounded batch of completions on the game thread.
 */
void
authum_tasks_host_pump(void);

/**
 * @brief Join the owner worker and dispose pending and completed job payloads.
 */
void
authum_tasks_host_drop_owner(authum_ptr owner);

/**
 * @brief Report whether completion callbacks or payload destructors are running.
 */
authum_bool_t
authum_tasks_host_dispatching(void);

AUTHUM_EXTERN_C_END

#endif
