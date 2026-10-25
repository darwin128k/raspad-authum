#define _GNU_SOURCE
#include <authum/host/threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#    include <windows.h>
#else
#    include <pthread.h>
#    include <signal.h>
#    include <sched.h>
#endif
struct authum_thread
{
    struct authum_thread *next;
    authum_thread_fn run, wake;
    authum_ptr user;
    unsigned flags;
    int requested, joined;
#if defined(_WIN32)
    HANDLE native;
#else
    pthread_t native;
#endif
};
#if defined(_WIN32)
static DWORD WINAPI
thread_run(LPVOID arg)
{
#else
static void *
thread_run(void *arg)
{
#endif
    authum_thread_t *t = arg;
#if !defined(_WIN32)
    if (t->flags & AUTHUM_THREAD_BLOCK_SIGPIPE)
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
    }
#endif
    t->run(t->user);
    return 0;
}

static void
request_stop(authum_thread_t *t)
{
    if (!t->requested)
    {
        t->requested = 1;
        t->wake(t->user);
    }
}

static void
join_thread(authum_thread_t *t)
{
    if (t->joined)
    {
        return;
    }
#if defined(_WIN32)
    WaitForSingleObject(t->native, INFINITE);
    CloseHandle(t->native);
#else
    pthread_join(t->native, NULL);
#endif
    t->joined = 1;
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
    t->run = run;
    t->wake = wake;
    t->user = user;
    t->flags = flags;
#if defined(_WIN32)
    if (cpu >= (int)(8 * sizeof(DWORD_PTR)))
    {
        goto failed;
    }
    t->native = CreateThread(NULL, 0, thread_run, t, CREATE_SUSPENDED, NULL);
    if (!t->native)
    {
        goto failed;
    }
    if (cpu >= 0 && !SetThreadAffinityMask(t->native, (DWORD_PTR)1 << cpu))
    {
        /* Thread has not executed any module code yet. */
        TerminateThread(t->native, 1);
        CloseHandle(t->native);
        goto failed;
    }
    if (ResumeThread(t->native) == (DWORD)-1)
    {
        TerminateThread(t->native, 1);
        CloseHandle(t->native);
        goto failed;
    }
#else
    int rc;
    if (cpu >= CPU_SETSIZE)
    {
        rc = 22;
        goto failed;
    }
    pthread_attr_t attr;
    rc = pthread_attr_init(&attr);
    if (rc)
    {
        goto failed;
    }
    if (cpu >= 0)
    {
        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        CPU_SET(cpu, &cpus);
        rc = pthread_attr_setaffinity_np(&attr, sizeof cpus, &cpus);
    }
    if (!rc)
    {
        rc = pthread_create(&t->native, &attr, thread_run, t);
    }
    pthread_attr_destroy(&attr);
    if (rc)
    {
        goto failed;
    }
#endif
    t->next = owner->head;
    owner->head = t;
    ++owner->count;
    return t;
failed:
#if defined(_WIN32)
    if (error && error_size)
    {
        snprintf(error, error_size, "cannot create thread (cpu=%d): Windows error %lu", cpu,
                 (unsigned long)GetLastError());
    }
#else
    if (error && error_size)
    {
        snprintf(error, error_size, "cannot create thread (cpu=%d): %s", cpu, strerror(rc));
    }
#endif
    free(t);
    return NULL;
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
    join_thread(thread);
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
        join_thread(t);
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
