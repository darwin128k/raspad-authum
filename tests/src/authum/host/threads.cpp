#ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#endif
#include <authum/null.h>
#include <authum/host/threads.h>
#include <atomic>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

typedef struct job
{
    int pipe[2];
    std::atomic_int *wakes;
    std::atomic_int exited, masked, all_woken;
} job_t;

void
run_job(void *arg)
{
    job_t *job = static_cast<job_t *>(arg);
    sigset_t mask;
    pthread_sigmask(SIG_SETMASK, authum_null, &mask);
    std::atomic_store(&job->masked, sigismember(&mask, SIGPIPE));
    char b;
    ssize_t n = read(job->pipe[0], &b, 1);
    (void)n;
    for (int i = 0; i < 1000 && std::atomic_load(job->wakes) != 2; i++)
    {
        usleep(1000);
    }
    std::atomic_store(&job->all_woken, std::atomic_load(job->wakes) == 2);
    std::atomic_store(&job->exited, 1);
}

void
wake_job(void *arg)
{
    job_t *job = static_cast<job_t *>(arg);
    std::atomic_fetch_add(job->wakes, 1);
    ssize_t n = write(job->pipe[1], "x", 1);
    (void)n;
}

static authum_thread_owner_t owner, other;
static authum_threads_t threads, others;
static std::atomic_int wakes;
static job_t jobs[2];
static authum_thread_t *handles[2];
static sigset_t original;
static char error[256];

namespace tests
{

class authum_threads : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        authum_threads_host_init(&owner, &threads);
        authum_threads_host_init(&other, &others);
        wakes = 0;
        for (int index = 0; index < 2; ++index)
        {
            jobs[index].pipe[0] = jobs[index].pipe[1] = -1;
            jobs[index].wakes = &wakes;
            jobs[index].exited = jobs[index].masked = jobs[index].all_woken = 0;
            handles[index] = authum_null;
        }
        pthread_sigmask(SIG_SETMASK, authum_null, &original);
    }

    void
    TearDown() override
    {
        authum_threads_host_stop_owner(&owner);
        authum_threads_host_drop_owner(&owner);
        authum_threads_host_drop_owner(&other);
        for (int index = 0; index < 2; ++index)
        {
            if (jobs[index].pipe[0] >= 0)
            {
                close(jobs[index].pipe[0]);
            }
            if (jobs[index].pipe[1] >= 0)
            {
                close(jobs[index].pipe[1]);
            }
        }
        pthread_sigmask(SIG_SETMASK, &original, authum_null);
    }
};

void
start_two_jobs(void)
{
    for (int index = 0; index < 2; ++index)
    {
        ASSERT_EQ(pipe(jobs[index].pipe), 0);
        handles[index] = authum_thread_start(&threads, run_job, wake_job, &jobs[index], -1,
                                             AUTHUM_THREAD_BLOCK_SIGPIPE, error, sizeof error);
        ASSERT_NE(handles[index], authum_null) << error;
    }
}

TEST_F(authum_threads, missing_service_reports_unsupported)
{
    EXPECT_EQ(authum_thread_start(authum_null, run_job, wake_job, authum_null, -1, 0, error,
                                  sizeof error),
              authum_null);
    EXPECT_NE(strstr(error, "on this platform"), authum_null);
}

TEST_F(authum_threads, invalid_affinity_does_not_register_handle)
{
    EXPECT_EQ(
        authum_thread_start(&threads, run_job, wake_job, authum_null, 1024, 0, error, sizeof error),
        authum_null);
    EXPECT_EQ(lh_os_thread_group_get_count(&owner.group), 0u);
}

TEST_F(authum_threads, refuses_foreign_owner_handle)
{
    ASSERT_NO_FATAL_FAILURE(start_two_jobs());
    EXPECT_FALSE(authum_thread_stop(&others, handles[0]));
    EXPECT_EQ(lh_os_thread_group_get_count(&owner.group), 2u);
}

TEST_F(authum_threads, refuses_submission_at_owner_limit)
{
    lh_os_thread_group_set_count(&owner.group, 64U);
    EXPECT_EQ(
        authum_thread_start(&threads, run_job, wake_job, authum_null, -1, 0, error, sizeof error),
        authum_null);
    lh_os_thread_group_set_count(&owner.group, 0U);
}

TEST_F(authum_threads, wakes_every_worker_before_joining)
{
    ASSERT_NO_FATAL_FAILURE(start_two_jobs());
    authum_threads_host_stop_owner(&owner);
    EXPECT_EQ(wakes.load(), 2);
    for (int index = 0; index < 2; ++index)
    {
        EXPECT_TRUE(jobs[index].exited.load());
        EXPECT_TRUE(jobs[index].all_woken.load());
    }
}

TEST_F(authum_threads, stopped_owner_rejects_new_threads)
{
    authum_threads_host_stop_owner(&owner);
    EXPECT_EQ(
        authum_thread_start(&threads, run_job, wake_job, authum_null, -1, 0, error, sizeof error),
        authum_null);
}

TEST_F(authum_threads, joined_handles_can_be_released_explicitly)
{
    ASSERT_NO_FATAL_FAILURE(start_two_jobs());
    authum_threads_host_stop_owner(&owner);
    for (int index = 0; index < 2; ++index)
    {
        EXPECT_TRUE(authum_thread_stop(&threads, handles[index]));
    }
    EXPECT_EQ(lh_os_thread_group_get_count(&owner.group), 0u);
    EXPECT_EQ(wakes.load(), 2);
}

TEST_F(authum_threads, sigpipe_mask_is_local_to_workers)
{
    ASSERT_NO_FATAL_FAILURE(start_two_jobs());
    authum_threads_host_stop_owner(&owner);
    for (int index = 0; index < 2; ++index)
    {
        EXPECT_TRUE(jobs[index].masked.load());
    }
    sigset_t after;
    pthread_sigmask(SIG_SETMASK, authum_null, &after);
    EXPECT_EQ(sigismember(&original, SIGPIPE), sigismember(&after, SIGPIPE));
}

} // namespace tests
