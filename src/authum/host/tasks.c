#include <authum/host/tasks.h>
#include <stdlib.h>
#include <lh/os/thread.h>
#include <lh/os/mutex.h>
#include <lh/os/condition.h>

static void
authum_tasks_require(lh_bool_t ok)
{
    /* Continuing with an invalid lock/join would unload live module code. */
    if (!ok)
    {
        abort();
    }
}

typedef struct authum_task_job
{
    struct authum_task_job *next;
    authum_task_fn work, complete, destroy;
    authum_ptr user;
} authum_task_job_t;

typedef struct authum_task_worker
{
    struct authum_task_worker *next;
    authum_ptr owner;
    authum_task_job_t *pending, **pending_tail, *done, **done_tail;
    unsigned count;
    int stop;
    lh_os_thread_t thread;
    lh_os_mutex_t mutex;
    lh_os_condition_t wake;
} authum_task_worker_t;

static authum_task_worker_t *m_workers;
static unsigned m_dispatching;

void
authum_tasks_lock(authum_task_worker_t *w)
{
    authum_tasks_require(lh_os_mutex_lock(&w->mutex));
}

void
authum_tasks_unlock(authum_task_worker_t *w)
{
    authum_tasks_require(lh_os_mutex_unlock(&w->mutex));
}

void
authum_tasks_wake(authum_task_worker_t *w)
{
    authum_tasks_require(lh_os_condition_signal(&w->wake));
}

void
authum_tasks_run(void *arg)
{
    authum_task_worker_t *w = arg;
    for (;;)
    {
        authum_task_job_t *job;
        authum_tasks_lock(w);
        while (!w->stop && !w->pending)
        {
            authum_tasks_require(lh_os_condition_wait(&w->wake, &w->mutex));
        }
        if (w->stop)
        {
            authum_tasks_unlock(w);
            break;
        }
        job = w->pending;
        w->pending = job->next;
        if (!w->pending)
        {
            w->pending_tail = &w->pending;
        }
        authum_tasks_unlock(w);
        job->work(job->user);
        job->next = NULL;
        authum_tasks_lock(w);
        *w->done_tail = job;
        w->done_tail = &job->next;
        authum_tasks_unlock(w);
    }
}

authum_task_worker_t *
authum_tasks_create(authum_ptr owner)
{
    authum_task_worker_t *w = calloc(1, sizeof *w);
    if (!w)
    {
        return NULL;
    }
    w->owner = owner;
    w->pending_tail = &w->pending;
    w->done_tail = &w->done;
    lh_os_thread_init(&w->thread);
    lh_os_mutex_init(&w->mutex);
    lh_os_condition_init(&w->wake);
    if (!lh_os_mutex_open(&w->mutex) || !lh_os_condition_open(&w->wake) ||
        !lh_os_thread_start(&w->thread, authum_tasks_run, w, -1, 0))
    {
        authum_tasks_require(lh_os_condition_close(&w->wake));
        authum_tasks_require(lh_os_mutex_close(&w->mutex));
        free(w);
        return NULL;
    }
    w->next = m_workers;
    m_workers = w;
    return w;
}

authum_bool_t
authum_tasks_host_submit(authum_ptr owner, authum_task_fn work, authum_task_fn complete,
                         authum_task_fn destroy, authum_ptr user)
{
    authum_task_worker_t *w;
    authum_task_job_t *job;
    if (!owner || !work)
    {
        return authum_bool_false;
    }
    for (w = m_workers; w && w->owner != owner; w = w->next)
    {
    }
    if (!w && !(w = authum_tasks_create(owner)))
    {
        return authum_bool_false;
    }
    job = calloc(1, sizeof *job);
    if (!job)
    {
        return authum_bool_false;
    }
    job->work = work;
    job->complete = complete;
    job->destroy = destroy;
    job->user = user;
    authum_tasks_lock(w);
    if (w->stop || w->count >= 64)
    {
        authum_tasks_unlock(w);
        free(job);
        return authum_bool_false;
    }
    ++w->count;
    *w->pending_tail = job;
    w->pending_tail = &job->next;
    authum_tasks_wake(w);
    authum_tasks_unlock(w);
    return authum_bool_true;
}

void
authum_tasks_dispose(authum_task_job_t *job)
{
    if (job->destroy)
    {
        job->destroy(job->user);
    }
    free(job);
}

void
authum_tasks_host_pump(void)
{
    unsigned budget = 16;
    authum_task_worker_t *w;
    /* Rescan after each callback; it can enqueue work for another owner. */
    while (budget--)
    {
        authum_task_job_t *job = NULL;
        for (w = m_workers; w; w = w->next)
        {
            authum_tasks_lock(w);
            job = w->done;
            if (job)
            {
                w->done = job->next;
                if (!w->done)
                {
                    w->done_tail = &w->done;
                }
                --w->count;
            }
            authum_tasks_unlock(w);
            if (job)
            {
                break;
            }
        }
        if (!job)
        {
            break;
        }
        /* Round robin across modules, including when a completion immediately
         * submits its next task. A busy owner must not starve other modules. */
        if (w->next)
        {
            authum_task_worker_t **link = &m_workers, **tail;
            while (*link != w)
            {
                link = &(*link)->next;
            }
            *link = w->next;
            w->next = NULL;
            tail = &m_workers;
            while (*tail)
            {
                tail = &(*tail)->next;
            }
            *tail = w;
        }
        ++m_dispatching;
        if (job->complete)
        {
            job->complete(job->user);
        }
        authum_tasks_dispose(job);
        --m_dispatching;
    }
}

authum_bool_t
authum_tasks_host_dispatching(void)
{
    return m_dispatching ? authum_bool_true : authum_bool_false;
}

void
authum_tasks_host_drop_owner(authum_ptr owner)
{
    authum_task_worker_t **link = &m_workers, *w;
    authum_task_job_t *job;
    while (*link && (*link)->owner != owner)
    {
        link = &(*link)->next;
    }
    if (!(w = *link))
    {
        return;
    }
    authum_tasks_lock(w);
    w->stop = 1;
    authum_tasks_wake(w);
    authum_tasks_unlock(w);
    authum_tasks_require(lh_os_thread_join(&w->thread));
    ++m_dispatching; /* destructors may only release their payload, not unload modules */
    while ((job = w->pending))
    {
        w->pending = job->next;
        authum_tasks_dispose(job);
    }
    while ((job = w->done))
    {
        w->done = job->next;
        authum_tasks_dispose(job);
    }
    --m_dispatching;
    /* Keep the stopped owner registered, with a live lock, until all payload
     * destructors return. They must not resurrect a worker during unload. */
    link = &m_workers;
    while (*link != w)
    {
        link = &(*link)->next;
    }
    *link = w->next;
    authum_tasks_require(lh_os_condition_close(&w->wake));
    authum_tasks_require(lh_os_mutex_close(&w->mutex));
    free(w);
}

const authum_task_ops_t *
authum_tasks_host_ops(void)
{
    static const authum_task_ops_t ops = {sizeof ops, authum_tasks_host_submit};
    return &ops;
}
