#include <authum/host/tasks.h>
#include <stdlib.h>
#if defined(_WIN32)
#    include <windows.h>
#else
#    include <pthread.h>
#endif

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
#if defined(_WIN32)
    HANDLE thread;
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE wake;
#else
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t wake;
#endif
} authum_task_worker_t;
static authum_task_worker_t *m_workers;
static unsigned m_dispatching;

void
authum_tasks_lock(authum_task_worker_t *w)
{
#if defined(_WIN32)
    EnterCriticalSection(&w->mutex);
#else
    pthread_mutex_lock(&w->mutex);
#endif
}
void
authum_tasks_unlock(authum_task_worker_t *w)
{
#if defined(_WIN32)
    LeaveCriticalSection(&w->mutex);
#else
    pthread_mutex_unlock(&w->mutex);
#endif
}
void
authum_tasks_wake(authum_task_worker_t *w)
{
#if defined(_WIN32)
    WakeConditionVariable(&w->wake);
#else
    pthread_cond_signal(&w->wake);
#endif
}
#if defined(_WIN32)
DWORD WINAPI
authum_tasks_run(LPVOID arg)
#else
void *
authum_tasks_run(void *arg)
#endif
{
    authum_task_worker_t *w = arg;
    for (;;)
    {
        authum_task_job_t *job;
        authum_tasks_lock(w);
        while (!w->stop && !w->pending)
        {
#if defined(_WIN32)
            SleepConditionVariableCS(&w->wake, &w->mutex, INFINITE);
#else
            pthread_cond_wait(&w->wake, &w->mutex);
#endif
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
    return 0;
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
#if defined(_WIN32)
    InitializeCriticalSection(&w->mutex);
    InitializeConditionVariable(&w->wake);
    w->thread = CreateThread(NULL, 0, authum_tasks_run, w, 0, NULL);
    if (!w->thread)
    {
        DeleteCriticalSection(&w->mutex);
        free(w);
        return NULL;
    }
#else
    if (pthread_mutex_init(&w->mutex, NULL))
    {
        free(w);
        return NULL;
    }
    if (pthread_cond_init(&w->wake, NULL))
    {
        pthread_mutex_destroy(&w->mutex);
        free(w);
        return NULL;
    }
    if (pthread_create(&w->thread, NULL, authum_tasks_run, w))
    {
        pthread_cond_destroy(&w->wake);
        pthread_mutex_destroy(&w->mutex);
        free(w);
        return NULL;
    }
#endif
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
#if defined(_WIN32)
    WaitForSingleObject(w->thread, INFINITE);
    CloseHandle(w->thread);
#else
    pthread_join(w->thread, NULL);
#endif
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
    while (*link != w) link = &(*link)->next;
    *link = w->next;
#if defined(_WIN32)
    DeleteCriticalSection(&w->mutex);
#else
    pthread_cond_destroy(&w->wake);
    pthread_mutex_destroy(&w->mutex);
#endif
    free(w);
}
const authum_task_ops_t *
authum_tasks_host_ops(void)
{
    static const authum_task_ops_t ops = {sizeof ops, authum_tasks_host_submit};
    return &ops;
}
