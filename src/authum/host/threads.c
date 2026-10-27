#include <authum/host/threads.h>
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
    int requested;
    lh_os_thread_t native;
};

static void
request_stop(authum_thread_t *t)
{
    if (!t->requested)
    {
        t->requested = 1;
        t->wake(t->user);
    }
}

static authum_bool_t
join_thread(authum_thread_t *t)
{
    return lh_os_thread_join(&t->native) ? authum_bool_true : authum_bool_false;
}

static authum_thread_t *
start_thread(authum_ptr context, authum_thread_fn run, authum_thread_fn wake, authum_ptr user,
             int cpu, unsigned flags, authum_char_t *error, authum_usize_t error_size)
{
    authum_thread_owner_t *owner = context;
    if (!owner || owner->closing || owner->count >= 64 || !run || !wake || cpu < -1 ||
        (flags & ~AUTHUM_THREAD_BLOCK_SIGPIPE))
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "invalid thread options or owner stopping/limit reached");
        }
        return NULL;
    }
    authum_thread_t *t = calloc(1, sizeof *t);
    if (!t)
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "thread allocation failed");
        }
        return NULL;
    }
    t->wake = wake;
    t->user = user;
    lh_os_thread_init(&t->native);
    if (!lh_os_thread_start(&t->native, run, user, cpu,
                            flags & AUTHUM_THREAD_BLOCK_SIGPIPE ? lh_os_thread_block_sigpipe : 0))
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "cannot create thread (cpu=%d): OS error %lu", cpu,
                     (unsigned long)lh_os_system_get_last_error_code());
        }
        free(t);
        return NULL;
    }
    t->next = owner->head;
    owner->head = t;
    ++owner->count;
    return t;
}

static authum_bool_t
stop_thread(authum_ptr context, authum_thread_t *thread)
{
    authum_thread_owner_t *owner = context;
    if (!owner || !thread)
    {
        return authum_bool_false;
    }
    authum_thread_t **link = &owner->head;
    while (*link && *link != thread)
    {
        link = &(*link)->next;
    }
    if (!*link)
    {
        return authum_bool_false;
    }
    request_stop(thread);
    if (!join_thread(thread))
    {
        return authum_bool_false;
    }
    *link = thread->next;
    --owner->count;
    free(thread);
    return authum_bool_true;
}

static const authum_thread_ops_t ops = {
    .size = sizeof ops, .start = start_thread, .stop = stop_thread};

void
authum_threads_host_init(authum_thread_owner_t *owner, authum_threads_t *facade)
{
    memset(owner, 0, sizeof *owner);
    facade->owner = owner;
    facade->ops = &ops;
}

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
        free(t);
    }
    owner->count = 0;
}
