#include <authum/host/threads.h>
#include <authum/null.h>
#include <lh/util/ptr.h>
#include <authum/memory.h>
#include <lh/memory/std.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lh/os/thread.h>
#include <lh/os/system.h>

struct authum_thread
{
    struct authum_thread *next;
    authum_thread_fn wake;
    authum_ptr user;
    authum_i32_t requested;
    lh_os_thread_t native;
};

/**
 * @brief Request cooperative stop at most once, on the owner thread.
 */
static void
request_stop(authum_thread_t *t)
{
    if (!t->requested)
    {
        t->requested = 1;
        t->wake(t->user);
    }
}

/**
 * @brief Wait for an owned worker while retaining its handle if the join fails.
 */
static authum_bool_t
join_thread(authum_thread_t *t)
{
    return lh_os_thread_join(lh_addr_of(t->native)) ? authum_bool_true : authum_bool_false;
}

/**
 * @brief Validate owner limits and register a worker only after LH startup succeeds.
 */
static authum_thread_t *
start_thread(authum_ptr context, authum_thread_fn run, authum_thread_fn wake, authum_ptr user,
             authum_i32_t cpu, authum_u32_t flags, authum_char_t *error, authum_usize_t error_size)
{
    authum_thread_owner_t *owner = context;
    if (!owner || owner->closing || lh_math_ge(owner->count, 64) || !run || !wake ||
        lh_math_lt(cpu, -1) || (lh_math_bit_and(flags, ~AUTHUM_THREAD_BLOCK_SIGPIPE)))
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "invalid thread options or owner stopping/limit reached");
        }
        return authum_null;
    }
    authum_thread_t *t = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(t));
    if (!t)
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "thread allocation failed");
        }
        return authum_null;
    }
    t->wake = wake;
    t->user = user;
    lh_os_thread_init(lh_addr_of(t->native));
    if (!lh_os_thread_start(
            lh_addr_of(t->native), run, user, cpu,
            lh_math_bit_and(flags, AUTHUM_THREAD_BLOCK_SIGPIPE) ? lh_os_thread_block_sigpipe : 0))
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "cannot create thread (cpu=%d): OS error %lu", cpu,
                     lh_cast_static(lh_ulong_t, lh_os_system_get_last_error_code()));
        }
        authum_memory_free(t);
        return authum_null;
    }
    t->next = owner->head;
    owner->head = t;
    owner->count = lh_math_add_one(owner->count);
    return t;
}

/**
 * @brief Stop, join and release a handle belonging to the requesting owner.
 */
static authum_bool_t
stop_thread(authum_ptr context, authum_thread_t *thread)
{
    authum_thread_owner_t *owner = context;
    if (!owner || !thread)
    {
        return authum_bool_false;
    }
    authum_thread_t **link = lh_addr_of(owner->head);
    while (lh_ptr_deref(link) && lh_math_ne(lh_ptr_deref(link), thread))
    {
        link = lh_addr_of((lh_ptr_deref(link))->next);
    }
    if (!lh_ptr_deref(link))
    {
        return authum_bool_false;
    }
    request_stop(thread);
    if (!join_thread(thread))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(link) = thread->next;
    owner->count = lh_math_sub_one(owner->count);
    authum_memory_free(thread);
    return authum_bool_true;
}

static const authum_thread_ops_t ops = {.start = start_thread, .stop = stop_thread};

/**
 * @brief Initialize an owner and bind its public thread facade.
 */
void
authum_threads_host_init(authum_thread_owner_t *owner, authum_threads_t *facade)
{
    lh_memory_std_set(owner, 0, sizeof lh_ptr_deref(owner));
    facade->owner = owner;
    facade->ops = lh_addr_of(ops);
}

/**
 * @brief Wake every owned worker before joining any, preventing unsafe module unload.
 */
void
authum_threads_host_stop_owner(authum_thread_owner_t *owner)
{
    if (!owner)
    {
        return;
    }
    owner->closing = 1;
    /* Wake all first: workers are allowed to wait for each other to exit. */
    for (authum_thread_t *t = owner->head; t; t = t->next)
    {
        request_stop(t);
    }
    for (authum_thread_t *t = owner->head; t; t = t->next)
    {
        if (!join_thread(t))
        {
            /* A live worker must never survive unloading its module code. */
            fputs("Authum: cannot join owned worker before module unload\n", stderr);
            abort();
        }
    }
}

/**
 * @brief Join all owned workers and reclaim their remaining host handles.
 */
void
authum_threads_host_drop_owner(authum_thread_owner_t *owner)
{
    if (!owner)
    {
        return;
    }
    authum_threads_host_stop_owner(owner);
    while (owner->head)
    {
        authum_thread_t *t = owner->head;
        owner->head = t->next;
        authum_memory_free(t);
    }
    owner->count = 0;
}
