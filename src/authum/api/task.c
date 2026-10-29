#include <authum/task.h>
#include <authum/null.h>

authum_bool_t
authum_tasks_submit(const authum_tasks_t *tasks, authum_task_fn work, authum_task_fn complete,
                    authum_task_fn destroy, authum_ptr user)
{
    return tasks && tasks->ops && !authum_null_eq((tasks->ops)->submit) && work
               ? tasks->ops->submit(tasks->owner, work, complete, destroy, user)
               : authum_bool_false;
}
