#include <authum/task.h>
#include <authum/null.h>

authum_bool_t
authum_tasks_submit(const authum_tasks_t *tasks, authum_task_fn work, authum_task_fn complete,
                    authum_task_fn destroy, authum_ptr user)
{
    return !authum_null_eq(tasks) && !authum_null_eq(tasks->ops) &&
                   !authum_null_eq((tasks->ops)->submit) && !authum_null_eq(work)
               ? tasks->ops->submit(tasks->owner, work, complete, destroy, user)
               : authum_bool_false;
}

authum_bool_t
authum_tasks_at_exit(const authum_tasks_t *tasks, authum_task_fn fn, authum_ptr user)
{
    return !authum_null_eq(tasks) && !authum_null_eq(tasks->ops) &&
                   !authum_null_eq((tasks->ops)->at_exit) && !authum_null_eq(fn)
               ? tasks->ops->at_exit(tasks->owner, fn, user)
               : authum_bool_false;
}
