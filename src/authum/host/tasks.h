#ifndef AUTHUM_HOST_TASKS_H
#define AUTHUM_HOST_TASKS_H
#include <authum/task.h>
AUTHUM_EXTERN_C_BEGIN
const authum_task_ops_t *
authum_tasks_host_ops(void);
void
authum_tasks_host_pump(void);
void
authum_tasks_host_drop_owner(authum_ptr owner);
authum_bool_t
authum_tasks_host_dispatching(void);
AUTHUM_EXTERN_C_END
#endif
