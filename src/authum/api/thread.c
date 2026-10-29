#include <authum/thread.h>
#include <authum/null.h>
#include <stdio.h>

authum_thread_t *
authum_thread_start(const authum_threads_t *threads, authum_thread_fn run, authum_thread_fn wake,
                    authum_ptr user, authum_i32_t cpu, authum_u32_t flags, authum_char_t *error,
                    authum_usize_t error_size)
{
    if (error && error_size)
    {
        error[0] = 0;
    }
    if (!threads || !threads->ops || authum_null_eq((threads->ops)->start))
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "host has no managed-thread API on this platform");
        }
        return 0;
    }
    return threads->ops->start(threads->owner, run, wake, user, cpu, flags, error, error_size);
}

authum_bool_t
authum_thread_stop(const authum_threads_t *threads, authum_thread_t *thread)
{
    return threads && threads->ops && !authum_null_eq((threads->ops)->stop)
               ? threads->ops->stop(threads->owner, thread)
               : authum_bool_false;
}
