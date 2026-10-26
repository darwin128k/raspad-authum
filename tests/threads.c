#define _GNU_SOURCE
#include <authum/host/threads.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "threads:%d: %s\n", __LINE__, #x);                                     \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

typedef struct job
{
    int pipe[2];
    atomic_int *wakes;
    atomic_int exited, masked, all_woken;
} job_t;

static void
run_job(void *arg)
{
    job_t *job = arg;
    sigset_t mask;
    pthread_sigmask(SIG_SETMASK, NULL, &mask);
    atomic_store(&job->masked, sigismember(&mask, SIGPIPE));
    char b;
    ssize_t n = read(job->pipe[0], &b, 1);
    (void)n;
    for (int i = 0; i < 1000 && atomic_load(job->wakes) != 2; i++)
    {
        usleep(1000);
    }
    atomic_store(&job->all_woken, atomic_load(job->wakes) == 2);
    atomic_store(&job->exited, 1);
}

static void
wake_job(void *arg)
{
    job_t *job = arg;
    atomic_fetch_add(job->wakes, 1);
    ssize_t n = write(job->pipe[1], "x", 1);
    (void)n;
}

int
main(void)
{
    authum_thread_owner_t owner, other;
    authum_threads_t threads, others;
    authum_threads_host_init(&owner, &threads);
    authum_threads_host_init(&other, &others);
    char error[256];
    CHECK(!authum_thread_start(NULL, run_job, wake_job, NULL, -1, 0, error, sizeof error));
    CHECK(strstr(error, "update Authum"));
    CHECK(!authum_thread_start(&threads, run_job, wake_job, NULL, 1024, 0, error, sizeof error));
    CHECK(owner.count == 0);
    atomic_int wakes = ATOMIC_VAR_INIT(0);
    job_t jobs[2];
    memset(jobs, 0, sizeof jobs);
    authum_thread_t *handles[2];
    sigset_t original, after;
    pthread_sigmask(SIG_SETMASK, NULL, &original);
    for (int i = 0; i < 2; i++)
    {
        CHECK(!pipe(jobs[i].pipe));
        jobs[i].wakes = &wakes;
        atomic_init(&jobs[i].exited, 0);
        atomic_init(&jobs[i].masked, 0);
        atomic_init(&jobs[i].all_woken, 0);
        handles[i] = authum_thread_start(&threads, run_job, wake_job, &jobs[i], -1,
                                         AUTHUM_THREAD_BLOCK_SIGPIPE, error, sizeof error);
        CHECK(handles[i]);
    }
    CHECK(!authum_thread_stop(&others, handles[0]));
    CHECK(owner.count == 2);
    owner.count = 64;
    CHECK(!authum_thread_start(&threads, run_job, wake_job, NULL, -1, 0, error, sizeof error));
    owner.count = 2;
    authum_threads_host_stop_owner(&owner);
    CHECK(atomic_load(&wakes) == 2);
    CHECK(!authum_thread_start(&threads, run_job, wake_job, NULL, -1, 0, error, sizeof error));
    for (int i = 0; i < 2; i++)
    {
        CHECK(atomic_load(&jobs[i].exited));
        CHECK(atomic_load(&jobs[i].masked));
        CHECK(atomic_load(&jobs[i].all_woken));
        CHECK(authum_thread_stop(&threads, handles[i]));
        close(jobs[i].pipe[0]);
        close(jobs[i].pipe[1]);
    }
    CHECK(owner.count == 0);
    CHECK(atomic_load(&wakes) == 2);
    authum_threads_host_drop_owner(&owner);
    authum_threads_host_drop_owner(&other);
    pthread_sigmask(SIG_SETMASK, NULL, &after);
    CHECK(sigismember(&original, SIGPIPE) == sigismember(&after, SIGPIPE));
    puts("PASS: owner isolation, missing API, limits, affinity errors, wake-all-before-join, "
         "stop-before-module-stop, cleanup and signal mask");
    return 0;
}
