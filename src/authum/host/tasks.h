#ifndef AUTHUM_HOST_TASKS_H
#define AUTHUM_HOST_TASKS_H

#include <authum/task.h>
#include <lh/rail/context/initializer.h>
#include <lh/rail/thread.h>

typedef struct authum_task_record
{
    lh_rail_task_t task;
    authum_task_fn work;
    authum_task_fn complete;
    authum_task_fn destroy;
    authum_ptr user;
} authum_task_record_t;

enum
{
    /** Calls one owner may ask to run when its task thread ends. */
    authum_task_exit_max = 8,

    authum_task_worker_capacity = 64,
    authum_task_completion_budget = 16,
    /** One wait of the unload's stop; it waits again until the running task returns. */
    authum_task_stop_wait_ms = 1000
};

/** A call for the end of an owner's task thread. */
typedef struct authum_task_exit
{
    authum_task_fn fn;
    authum_ptr user;
} authum_task_exit_t;

/** One owner: its executor thread, its mailbox and the calls for the thread's end.
 *  Allocated, so it never moves. The calls are written on the game thread and read
 *  by the task thread only after its stop (ordered by the executor's mutex). */
typedef struct authum_task_worker
{
    struct authum_task_worker *next;
    authum_ptr owner;
    lh_rail_thread_t thread;
    lh_rail_mailbox_t mailbox;
    authum_task_exit_t exits[authum_task_exit_max];
    authum_usize_t exit_count;
} authum_task_worker_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Return the immutable host background-task operations table.
 */
const authum_task_ops_t lh_ptr_deref(authum_tasks_host_ops(void));

/**
 * @brief Register a call for the end of the owner's task thread (authum_tasks_at_exit).
 */
authum_bool_t
authum_tasks_host_at_exit(authum_ptr owner, authum_task_fn fn, authum_ptr user);

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

/**
 * @brief The record's work, on the owner's executor thread.
 */
void
authum_task_record_work(lh_ptr user, lh_ptr local);

/**
 * @brief The record's completion, on the game thread at pump.
 */
void
authum_task_record_complete(lh_ptr user);

/**
 * @brief The record's cleanup, on the game thread: the payload destructor, then the record.
 */
void
authum_task_record_destroy(lh_ptr user);

/**
 * @brief The close of an owner thread's context: the module's calls for the thread's end.
 */
void
authum_task_worker_exit(lh_ptr user, lh_ptr local);

/**
 * @brief The owner's worker, started on first use; null when it cannot be made.
 */
authum_task_worker_t *
authum_task_worker_get(authum_ptr owner);

AUTHUM_EXTERN_C_END

#endif
