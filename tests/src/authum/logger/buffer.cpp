#include <authum/logger/buffer.h>
#include <authum/unused.h>
#include <authum/host/tasks.h>
#include <gtest/gtest.h>
#include <atomic>
#include <string.h>
#if defined(_WIN32)
#    include <windows.h>
typedef DWORD thread_id_t;

thread_id_t
thread_id(void)
{
    return GetCurrentThreadId();
}

void
pause_ms(void)
{
    Sleep(1);
}
#else
#    include <pthread.h>
#    include <time.h>
typedef pthread_t thread_id_t;

thread_id_t
thread_id(void)
{
    return pthread_self();
}

void
pause_ms(void)
{
    struct timespec t = {0, 1000000};
    nanosleep(&t, authum_null);
}
#endif
static thread_id_t main_thread;
static std::atomic_int written, enabled, wrong_thread, work_finished;
static int delivered;
static char last_line[authum_logger_line_max];
static int saw_auto_message;

void
write_log(authum_ptr owner, authum_logger_level_t level, const char *text)
{
    authum_unused(owner);
    authum_unused(level);
    if (thread_id() != main_thread)
    {
        std::atomic_store(&wrong_thread, 1);
    }
    std::atomic_fetch_add(&written, 1);
    authum_cstr_copy(last_line, sizeof last_line, text);
    if (!strcmp(text, "worker 42"))
    {
        saw_auto_message = 1;
    }
}

authum_bool_t
log_enabled(authum_ptr owner, authum_logger_level_t level)
{
    authum_unused(owner);
    authum_unused(level);
    if (thread_id() != main_thread)
    {
        std::atomic_store(&wrong_thread, 1);
    }
    std::atomic_fetch_add(&enabled, 1);
    return authum_bool_true;
}

typedef struct log_job
{
    authum_logger_buffer_t buffer;
    authum_logger_t logger;
    authum_host_t host;
} log_job_t;

void
log_work(void *user)
{
    log_job_t *job = static_cast<log_job_t *>(user);
    authum_logger_buffer_t *previous = authum_logger_buffer_begin(&job->buffer);
    authum_host_log_info(&job->host, "worker %d", 42);
    authum_logger_error(&job->logger, "worker error");
    authum_logger_warning(&job->logger, "overflow");
    authum_logger_buffer_end(previous);
    std::atomic_store(&work_finished, 1);
}

void
log_complete(void *user)
{
    log_job_t *job = static_cast<log_job_t *>(user);
    EXPECT_EQ(thread_id(), main_thread);
    EXPECT_EQ(authum_logger_buffer_get_dropped(&job->buffer), 1);
    authum_logger_buffer_flush(&job->buffer);
    EXPECT_TRUE(saw_auto_message);
    ++delivered;
}

TEST(authum_logger_buffer, defers_worker_logging_until_game_thread_completion)
{
    written = 0;
    enabled = 0;
    wrong_thread = 0;
    work_finished = 0;
    delivered = 0;
    saw_auto_message = 0;
    last_line[0] = 0;
    main_thread = thread_id();
    int owner;
    unsigned i;
    log_job_t job = {0};
    authum_logger_ops_t ops = {write_log, log_enabled, 0, 0};
    authum_logger_channel_t channel = {&ops, authum_null};
    authum_tasks_t tasks = {authum_tasks_host_ops(), &owner};
    job.logger.channel = &channel;
    job.host.logger = &channel;
    EXPECT_FALSE(authum_logger_buffer_init(&job.buffer, &job.logger, 0));
    EXPECT_TRUE(authum_logger_buffer_init(&job.buffer, &job.logger, 2));
    EXPECT_TRUE(authum_tasks_submit(&tasks, log_work, log_complete, authum_null, &job));
    for (i = 0; i < 5000 && !std::atomic_load(&work_finished); ++i)
    {
        pause_ms();
    }
    EXPECT_TRUE(std::atomic_load(&work_finished));
    EXPECT_EQ(std::atomic_load(&written), 0);
    EXPECT_EQ(std::atomic_load(&enabled), 0);
    for (i = 0; i < 5000 && !delivered; ++i)
    {
        authum_tasks_host_pump();
        pause_ms();
    }
    EXPECT_EQ(delivered, 1);
    EXPECT_EQ(std::atomic_load(&written), 3);
    EXPECT_FALSE(std::atomic_load(&wrong_thread));
    EXPECT_TRUE(strstr(last_line, "1 background log messages dropped"));
    EXPECT_EQ(authum_logger_buffer_get_dropped(&job.buffer), 0);
    authum_tasks_host_drop_owner(&owner);
    authum_logger_buffer_destroy(&job.buffer);
    authum_logger_buffer_destroy(&job.buffer);
}

TEST(authum_logger_buffer, rejects_zero_capacity)
{
    authum_logger_buffer_t buffer = {};
    authum_logger_t logger = {};
    EXPECT_FALSE(authum_logger_buffer_init(&buffer, &logger, 0));
    authum_logger_buffer_destroy(&buffer);
}

TEST(authum_logger_buffer, nested_capture_restores_previous_buffer)
{
    main_thread = thread_id();
    log_job_t job = {};
    authum_logger_ops_t ops = {write_log, log_enabled, 0, 0};
    authum_logger_channel_t channel = {&ops, authum_null};
    job.logger.channel = &channel;
    authum_logger_buffer_t nested = {}, *previous;
    ASSERT_TRUE(authum_logger_buffer_init(&job.buffer, &job.logger, 2));
    EXPECT_TRUE(authum_logger_buffer_init(&nested, &job.logger, 1));
    previous = authum_logger_buffer_begin(&job.buffer);
    authum_logger_info(&job.logger, "outer");
    {
        authum_logger_buffer_t *outer = authum_logger_buffer_begin(&nested);
        authum_logger_info(&job.logger, "inner");
        authum_logger_buffer_end(outer);
    }
    authum_logger_buffer_end(previous);
    authum_logger_buffer_flush(&nested);
    EXPECT_STREQ(last_line, "inner");
    authum_logger_buffer_flush(&job.buffer);
    EXPECT_STREQ(last_line, "outer");
    authum_logger_buffer_destroy(&nested);

    authum_logger_buffer_destroy(&job.buffer);
}

TEST(authum_logger_buffer, counts_messages_beyond_capacity)
{
    authum_logger_ops_t ops = {write_log, log_enabled, 0, 0};
    authum_logger_channel_t channel = {&ops, authum_null};
    authum_logger_t logger = {};
    logger.channel = &channel;
    authum_logger_buffer_t buffer = {};
    ASSERT_TRUE(authum_logger_buffer_init(&buffer, &logger, 1));
    auto *previous = authum_logger_buffer_begin(&buffer);
    authum_logger_info(&logger, "first");
    authum_logger_info(&logger, "second");
    authum_logger_buffer_end(previous);
    EXPECT_EQ(authum_logger_buffer_get_dropped(&buffer), 1u);
    authum_logger_buffer_destroy(&buffer);
}
