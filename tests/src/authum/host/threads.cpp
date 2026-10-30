/**
 * @file threads.cpp
 * @brief The module threads: they start, they are woken before the join, they release.
 *
 * The wake path uses the lh condition and mutex bricks, so the suite runs on
 * every OS. The signal mask is POSIX-only (Windows has no SIGPIPE), so that
 * part sits behind `#if !defined(_WIN32)`; the flag itself is refused on
 * Windows, and that refusal is what the platform test checks.
 */
#ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#endif
#include <authum/null.h>
#include <authum/host/threads.h>
#include <lh/os/condition.h>
#include <lh/os/mutex.h>
#include <lh/util/str/ptr.h>
#include <atomic>
#include <gtest/gtest.h>
#if !defined(_WIN32)
#    include <pthread.h>
#    include <signal.h>
#endif

typedef struct job
{
    lh_os_mutex_t mutex;
    lh_os_condition_t condition;
    std::atomic_int *wakes;
    std::atomic_int exited, all_woken;
#if !defined(_WIN32)
    std::atomic_int masked;
#endif
} job_t;

void
run_job(void *arg)
{
    job_t *job = static_cast<job_t *>(arg);
#if !defined(_WIN32)
    sigset_t mask;
    pthread_sigmask(SIG_SETMASK, authum_null, &mask);
    std::atomic_store(&job->masked, sigismember(&mask, SIGPIPE));
#endif
    lh_os_mutex_lock(lh_addr_of(job->mutex));
    for (int i = 0; i < 1000 && std::atomic_load(job->wakes) < 2; i++)
    {
        lh_os_condition_wait_for(lh_addr_of(job->condition), lh_addr_of(job->mutex), 1u);
    }
    lh_os_mutex_unlock(lh_addr_of(job->mutex));
    std::atomic_store(&job->all_woken, std::atomic_load(job->wakes) == 2);
    std::atomic_store(&job->exited, 1);
}

void
wake_job(void *arg)
{
    job_t *job = static_cast<job_t *>(arg);
    lh_os_mutex_lock(lh_addr_of(job->mutex));
    std::atomic_fetch_add(job->wakes, 1);
    lh_os_condition_broadcast(lh_addr_of(job->condition));
    lh_os_mutex_unlock(lh_addr_of(job->mutex));
}

static authum_thread_owner_t owner, other;
static authum_threads_t threads, others;
static std::atomic_int wakes;
static job_t jobs[2];
static authum_thread_t *handles[2];
#if !defined(_WIN32)
static sigset_t original;
#endif
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
            lh_os_mutex_init(lh_addr_of(jobs[index].mutex));
            lh_os_condition_init(lh_addr_of(jobs[index].condition));
            jobs[index].wakes = &wakes;
            jobs[index].exited = jobs[index].all_woken = 0;
#if !defined(_WIN32)
            jobs[index].masked = 0;
#endif
            handles[index] = authum_null;
        }
#if !defined(_WIN32)
        pthread_sigmask(SIG_SETMASK, authum_null, &original);
#endif
    }

    void
    TearDown() override
    {
        authum_threads_host_stop_owner(&owner);
        authum_threads_host_drop_owner(&owner);
        authum_threads_host_drop_owner(&other);
        for (int index = 0; index < 2; ++index)
        {
            lh_os_condition_close(lh_addr_of(jobs[index].condition));
            lh_os_mutex_close(lh_addr_of(jobs[index].mutex));
        }
#if !defined(_WIN32)
        pthread_sigmask(SIG_SETMASK, &original, authum_null);
#endif
    }
};

void
start_two_jobs(lh_u32_t flags)
{
    for (int index = 0; index < 2; ++index)
    {
        handles[index] = authum_thread_start(&threads, run_job, wake_job, &jobs[index], -1, flags,
                                             error, sizeof error);
        ASSERT_NE(handles[index], authum_null) << error;
    }
}

TEST_F(authum_threads, missing_service_reports_unsupported)
{
    EXPECT_EQ(authum_thread_start(authum_null, run_job, wake_job, authum_null, -1, 0, error,
                                  sizeof error),
              authum_null);
    EXPECT_TRUE(lh_str_ptr_contains(error, "on this platform", lh_bool_false));
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
    ASSERT_NO_FATAL_FAILURE(start_two_jobs(0u));
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
    ASSERT_NO_FATAL_FAILURE(start_two_jobs(0u));
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
    ASSERT_NO_FATAL_FAILURE(start_two_jobs(0u));
    authum_threads_host_stop_owner(&owner);
    for (int index = 0; index < 2; ++index)
    {
        EXPECT_TRUE(authum_thread_stop(&threads, handles[index]));
    }
    EXPECT_EQ(lh_os_thread_group_get_count(&owner.group), 0u);
    EXPECT_EQ(wakes.load(), 2);
}

#if defined(_WIN32)

TEST_F(authum_threads, sigpipe_flag_is_refused_without_a_signal)
{
    EXPECT_EQ(authum_thread_start(&threads, run_job, wake_job, authum_null, -1,
                                  AUTHUM_THREAD_BLOCK_SIGPIPE, error, sizeof error),
              authum_null);
    EXPECT_EQ(lh_os_thread_group_get_count(&owner.group), 0u);
}

#else

TEST_F(authum_threads, sigpipe_mask_is_local_to_workers)
{
    ASSERT_NO_FATAL_FAILURE(start_two_jobs(AUTHUM_THREAD_BLOCK_SIGPIPE));
    authum_threads_host_stop_owner(&owner);
    for (int index = 0; index < 2; ++index)
    {
        EXPECT_TRUE(jobs[index].masked.load());
    }
    sigset_t after;
    pthread_sigmask(SIG_SETMASK, authum_null, &after);
    EXPECT_EQ(sigismember(&original, SIGPIPE), sigismember(&after, SIGPIPE));
}

#endif

} // namespace tests
