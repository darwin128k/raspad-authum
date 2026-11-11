/**
 * @file tasks.c
 * @brief Modules' background tasks: one LH thread executor and one mailbox per owner.
 *
 * Each owner (a loaded module) gets an `lh_rail_thread_t` that runs its tasks in
 * order and an `lh_rail_mailbox_t` that bounds them (queued, running and finished
 * until delivered) and holds the finished ones for the game frame. The pump
 * delivers `complete` then `destroy` on the game thread, fairly across owners.
 * Dropping an owner cancels what has not started, joins the running task, and
 * runs the remaining payload destructors on the game thread without `complete`.
 *
 * Authum's task has one-argument callbacks; a record per submission carries them
 * next to the LH task it is submitted as, and is freed by its own `destroy`.
 *
 * A module may ask for calls on its task thread when that thread ends
 * (authum_tasks_at_exit): they are the close of the thread's LH context, so
 * they run on that thread after its last task, during the owner's drop.
 */

#include <authum/host/tasks.h>
#include <authum/memory.h>
#include <authum/null.h>

/** One submission: the LH task and the module's callbacks it runs. */
/* Both pointers are null when empty; a nonempty tail always has next == null. */
static authum_task_worker_t *m_workers;
static authum_task_worker_t *m_workers_tail;
static authum_usize_t m_dispatching;

/** The record's work, on the owner's executor thread. */
void
authum_task_record_work(lh_ptr user, lh_ptr local)
{
    authum_task_record_t *record = user;
    (void)local;
    record->work(record->user);
}

/** The record's completion, on the game thread at pump. */
void
authum_task_record_complete(lh_ptr user)
{
    authum_task_record_t *record = user;
    if (!authum_null_eq(record->complete))
    {
        record->complete(record->user);
    }
}

/** The record's cleanup, on the game thread: the payload destructor, then the record. */
void
authum_task_record_destroy(lh_ptr user)
{
    authum_task_record_t *record = user;
    if (!authum_null_eq(record->destroy))
    {
        record->destroy(record->user);
    }
    authum_memory_free(record);
}

/** The close of an owner thread's context: the module's calls for the thread's end. */
void
authum_task_worker_exit(lh_ptr user, lh_ptr local)
{
    authum_task_worker_t *w = user;
    (void)local;
    for (authum_usize_t i = 0; lh_math_lt(i, w->exit_count); i = lh_math_add_one(i))
    {
        w->exits[i].fn(w->exits[i].user);
    }
}

/** The owner's worker, started on first use; null when it cannot be made. */
authum_task_worker_t *
authum_task_worker_get(authum_ptr owner)
{
    authum_task_worker_t *w;
    for (w = m_workers; !authum_null_eq(w) && lh_math_ne(w->owner, owner); w = w->next)
    {
    }
    if (!authum_null_eq(w))
    {
        return w;
    }
    w = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(w));
    if (authum_null_eq(w))
    {
        return authum_null;
    }
    const lh_rail_context_t context =
        lh_rail_context_initializer(lh_null, authum_task_worker_exit, w);
    lh_rail_mailbox_init(lh_addr_of(w->mailbox), authum_task_worker_capacity);
    if (!lh_rail_thread_try_start(lh_addr_of(w->thread), lh_addr_of(context)))
    {
        authum_memory_free(w);
        return authum_null;
    }
    w->owner = owner;
    w->next = m_workers;
    m_workers = w;
    if (authum_null_eq(m_workers_tail))
    {
        m_workers_tail = w;
    }
    return w;
}

authum_bool_t
authum_tasks_host_submit(authum_ptr owner, authum_task_fn work, authum_task_fn complete,
                         authum_task_fn destroy, authum_ptr user)
{
    if (authum_null_eq(owner) || authum_null_eq(work))
    {
        return authum_bool_false;
    }
    authum_task_worker_t *w = authum_task_worker_get(owner);
    if (authum_null_eq(w))
    {
        return authum_bool_false;
    }
    authum_task_record_t *record = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(record));
    if (authum_null_eq(record))
    {
        return authum_bool_false;
    }
    record->work = work;
    record->complete = complete;
    record->destroy = destroy;
    record->user = user;
    lh_rail_task_init(lh_addr_of(record->task), authum_task_record_work,
                      authum_task_record_complete, authum_task_record_destroy, record);
    if (!lh_rail_executor_try_submit(lh_rail_thread_get_executor(lh_addr_of(w->thread)),
                                     lh_addr_of(record->task), lh_addr_of(w->mailbox)))
    {
        /* Refused: nothing ran, the payload stays the caller's; only the record goes. */
        authum_memory_free(record);
        return authum_bool_false;
    }
    return authum_bool_true;
}

/**
 * @brief Register fn(user) for the end of the owner's task thread (deduplicated).
 */
authum_bool_t
authum_tasks_host_at_exit(authum_ptr owner, authum_task_fn fn, authum_ptr user)
{
    if (authum_null_eq(owner) || authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    authum_task_worker_t *w = authum_task_worker_get(owner);
    if (authum_null_eq(w))
    {
        return authum_bool_false;
    }
    for (authum_usize_t i = 0; lh_math_lt(i, w->exit_count); i = lh_math_add_one(i))
    {
        if (lh_math_eq(w->exits[i].fn, fn) && lh_math_eq(w->exits[i].user, user))
        {
            return authum_bool_true;
        }
    }
    if (lh_math_ge(w->exit_count, authum_task_exit_max))
    {
        return authum_bool_false;
    }
    w->exits[w->exit_count].fn = fn;
    w->exits[w->exit_count].user = user;
    w->exit_count = lh_math_add_one(w->exit_count);
    return authum_bool_true;
}

/**
 * @brief Deliver a bounded batch of completed tasks on the game thread with owner fairness.
 */
void
authum_tasks_host_pump(void)
{
    authum_usize_t budget = authum_task_completion_budget;
    /* Rescan after each callback; it can enqueue work for another owner. */
    while (lh_math_is_positive(budget))
    {
        budget = lh_math_sub_one(budget);
        lh_rail_task_t *task = authum_null;
        authum_task_worker_t **link = lh_addr_of(m_workers);
        for (; !authum_null_eq(lh_ptr_deref(link)); link = lh_addr_of((lh_ptr_deref(link))->next))
        {
            task = lh_rail_mailbox_try_take(lh_addr_of((lh_ptr_deref(link))->mailbox));
            if (!authum_null_eq(task))
            {
                break;
            }
        }
        if (authum_null_eq(task))
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
        lh_rail_mailbox_deliver(lh_addr_of(w->mailbox), task);
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
 * @brief Cancel an owner's tasks, join its thread and dispose the payloads left.
 */
void
authum_tasks_host_drop_owner(authum_ptr owner)
{
    authum_task_worker_t **link = lh_addr_of(m_workers), *w;
    while (!authum_null_eq(lh_ptr_deref(link)) && lh_math_ne((lh_ptr_deref(link))->owner, owner))
    {
        link = lh_addr_of((lh_ptr_deref(link))->next);
    }
    if (authum_null_eq(w = lh_ptr_deref(link)))
    {
        return;
    }
    /* Refuse new tasks, cancel the ones not started and wait for the running one.
     * The executor stays alive (stopped) until the destructors below returned, so
     * one that submits again is refused instead of reaching a released thread. */
    while (!lh_rail_thread_try_stop(lh_addr_of(w->thread), lh_rail_stop_kind_cancel,
                                    authum_task_stop_wait_ms))
    {
    }
    /* Every accepted task is in the mailbox now. None gets complete during unload:
     * finished ones are cancelled too, so only their destructors run, here. */
    m_dispatching = lh_math_add_one(m_dispatching);
    for (lh_rail_task_t *task = lh_rail_mailbox_try_take(lh_addr_of(w->mailbox));
         !authum_null_eq(task); task = lh_rail_mailbox_try_take(lh_addr_of(w->mailbox)))
    {
        lh_rail_task_set_cancelled(task, lh_bool_true);
        lh_rail_mailbox_deliver(lh_addr_of(w->mailbox), task);
    }
    m_dispatching = lh_math_sub_one(m_dispatching);
    lh_rail_thread_destroy(lh_addr_of(w->thread));
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
    static const authum_task_ops_t ops = {.submit = authum_tasks_host_submit,
                                          .at_exit = authum_tasks_host_at_exit};
    return lh_addr_of(ops);
}
