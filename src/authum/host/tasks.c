#include <authum/host/tasks.h>
#include <authum/memory.h>
#include <authum/null.h>
#include <lh/os/worker.h>
#include <stdlib.h>

typedef lh_os_worker_job_t authum_task_job_t;

typedef struct authum_task_worker
{
    struct authum_task_worker *next;
    authum_ptr owner;
    lh_os_worker_t *worker;
} authum_task_worker_t;

enum
{
    authum_task_worker_capacity = 64,
    authum_task_completion_budget = 16
};

/* Both pointers are null when empty; a nonempty tail always has next == null. */
static authum_task_worker_t *m_workers;
static authum_task_worker_t *m_workers_tail;
static authum_usize_t m_dispatching;

authum_bool_t
authum_tasks_host_submit(authum_ptr owner, authum_task_fn work, authum_task_fn complete,
                         authum_task_fn destroy, authum_ptr user)
{
    authum_task_worker_t *w;
    if (!owner || !work)
    {
        return authum_bool_false;
    }
    for (w = m_workers; w && lh_math_ne(w->owner, owner); w = w->next)
    {
    }
    if (!w)
    {
        w = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(w));
        if (!w)
        {
            return authum_bool_false;
        }
        w->worker = lh_os_worker_create(authum_task_worker_capacity);
        if (!w->worker)
        {
            authum_memory_free(w);
            return authum_bool_false;
        }
        w->owner = owner;
        w->next = m_workers;
        m_workers = w;
        if (authum_null_eq(m_workers_tail))
        {
            m_workers_tail = w;
        }
    }
    return lh_os_worker_submit(w->worker, work, complete, destroy, user);
}

/**
 * @brief Deliver a bounded batch of completed jobs on the game thread with owner fairness.
 */
void
authum_tasks_host_pump(void)
{
    authum_usize_t budget = authum_task_completion_budget;
    /* Rescan after each callback; it can enqueue work for another owner. */
    while (lh_math_is_positive(budget))
    {
        budget = lh_math_sub_one(budget);
        authum_task_job_t *job = authum_null;
        authum_task_worker_t **link = lh_addr_of(m_workers);
        for (; lh_ptr_deref(link); link = lh_addr_of((lh_ptr_deref(link))->next))
        {
            job = lh_os_worker_take((lh_ptr_deref(link))->worker);
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
        authum_task_worker_t *w = lh_ptr_deref(link);
        if (lh_math_ne(w, m_workers_tail))
        {
            lh_ptr_deref(link) = w->next;
            w->next = authum_null;
            m_workers_tail->next = w;
            m_workers_tail = w;
        }
        m_dispatching = lh_math_add_one(m_dispatching);
        lh_os_worker_job_complete(job);
        lh_os_worker_dispose(job);
        m_dispatching = lh_math_sub_one(m_dispatching);
    }
}

/**
 * @brief Report whether a completion or payload destructor is being dispatched.
 */
authum_bool_t
authum_tasks_host_dispatching(void)
{
    return m_dispatching ? authum_bool_true : authum_bool_false;
}

/**
 * @brief Stop an owner worker and dispose queued payloads before releasing its state.
 */
void
authum_tasks_host_drop_owner(authum_ptr owner)
{
    authum_task_worker_t **link = lh_addr_of(m_workers), *w;
    while (lh_ptr_deref(link) && lh_math_ne((lh_ptr_deref(link))->owner, owner))
    {
        link = lh_addr_of((lh_ptr_deref(link))->next);
    }
    if (!(w = lh_ptr_deref(link)))
    {
        return;
    }
    lh_os_worker_stop(w->worker);
    m_dispatching = lh_math_add_one(m_dispatching);
    lh_os_worker_destroy(w->worker);
    m_dispatching = lh_math_sub_one(m_dispatching);
    /* Keep the stopped owner registered, with its worker, until all payload
     * destructors return. They must not resurrect a worker during unload. */
    authum_task_worker_t *previous = authum_null;
    link = lh_addr_of(m_workers);
    while (lh_math_ne(lh_ptr_deref(link), w))
    {
        previous = lh_ptr_deref(link);
        link = lh_addr_of((lh_ptr_deref(link))->next);
    }
    lh_ptr_deref(link) = w->next;
    if (lh_math_eq(m_workers_tail, w))
    {
        m_workers_tail = previous;
    }
    authum_memory_free(w);
}

/**
 * @brief Return the immutable background-task dispatch table.
 */
const authum_task_ops_t *
authum_tasks_host_ops(void)
{
    static const authum_task_ops_t ops = {sizeof ops, authum_tasks_host_submit};
    return lh_addr_of(ops);
}
