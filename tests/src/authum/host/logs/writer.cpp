#include <authum/host/logs/writer.h>
#include <authum/host/clock.h>
#include <atomic>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static std::atomic_bool entered, released, closed;
static std::atomic_uint received, losses;
static authum_u32_t expected;

void
pause_test(void)
{
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, authum_null);
}

void
emit(const authum_logs_record_t *record)
{
    std::atomic_store(&entered, 1);
    while (!std::atomic_load(&released))
    {
        pause_test();
    }
    /* Caller mutates its source immediately after submit; the copy must survive. */
    EXPECT_EQ(record->keep_days, (long)expected++);
    EXPECT_STREQ(record->text, "owned text");
    std::atomic_fetch_add(&received, 1);
}

void
flush(void)
{
}

void
close_sink(void)
{
    std::atomic_store(&closed, 1);
}

void
report_loss(authum_u32_t count)
{
    std::atomic_fetch_add(&losses, count);
}

static const authum_logs_sink_t sink = {emit, flush, close_sink, report_loss};

namespace tests
{

class authum_log_writer : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        entered = false;
        released = false;
        closed = false;
        received = 0;
        losses = 0;
        expected = 0;
        ASSERT_TRUE(authum_logs_writer_start(&sink));
    }

    void
    TearDown() override
    {
        released = true;
        authum_logs_writer_stop();
    }
};

void
wait_for_sink(void)
{
    authum_i64_t start = authum_clock_get_monotonic_ms(authum_clock_host());
    while (!entered && authum_clock_get_monotonic_ms(authum_clock_host()) - start < 5000)
    {
        pause_test();
    }
    ASSERT_TRUE(entered) << "Writer did not reach the test sink within five seconds";
}

TEST_F(authum_log_writer, owns_submitted_record_bytes)
{
    authum_logs_record_t record = {0};
    strcpy(record.text, "owned text");
    ASSERT_TRUE(authum_logs_writer_submit(&record));
    ASSERT_NO_FATAL_FAILURE(wait_for_sink());
    memset(&record, 0, sizeof record);
    released = true;
    ASSERT_TRUE(authum_logs_writer_sync(5000));
    EXPECT_EQ(received.load(), 1u);
}

TEST_F(authum_log_writer, delivers_records_in_fifo_order)
{
    authum_logs_record_t record = {0};
    strcpy(record.text, "owned text");
    for (authum_u32_t index = 0; index < 16; ++index)
    {
        record.keep_days = index;
        ASSERT_TRUE(authum_logs_writer_submit(&record));
    }
    released = true;
    ASSERT_TRUE(authum_logs_writer_sync(5000));
    EXPECT_EQ(received.load(), 16u);
}

TEST_F(authum_log_writer, full_queue_drops_newest_without_waiting_for_sink)
{
    authum_logs_record_t record = {0};
    strcpy(record.text, "owned text");
    ASSERT_TRUE(authum_logs_writer_submit(&record));
    ASSERT_NO_FATAL_FAILURE(wait_for_sink());
    // The sink cannot continue until this thread releases it.
    for (authum_u32_t index = 1; index < AUTHUM_LOGS_QUEUE_CAPACITY; ++index)
    {
        record.keep_days = index;
        ASSERT_TRUE(authum_logs_writer_submit(&record));
    }
    for (authum_u32_t index = 0; index < 57; ++index)
    {
        EXPECT_FALSE(authum_logs_writer_submit(&record));
    }
    EXPECT_EQ(authum_logs_writer_dropped(), 57u);
    released = true;
    ASSERT_TRUE(authum_logs_writer_sync(5000));
    EXPECT_EQ(received.load(), AUTHUM_LOGS_QUEUE_CAPACITY);
    EXPECT_EQ(losses.load(), 57u);
}

TEST_F(authum_log_writer, barrier_times_out_while_sink_is_blocked)
{
    authum_logs_record_t record = {0};
    strcpy(record.text, "owned text");
    ASSERT_TRUE(authum_logs_writer_submit(&record));
    ASSERT_NO_FATAL_FAILURE(wait_for_sink());
    EXPECT_FALSE(authum_logs_writer_sync(20));
    released = true;
    EXPECT_TRUE(authum_logs_writer_sync(5000));
}

TEST_F(authum_log_writer, shutdown_drains_records_and_closes_sink)
{
    authum_logs_record_t record = {0};
    strcpy(record.text, "owned text");
    ASSERT_TRUE(authum_logs_writer_submit(&record));
    released = true;
    authum_logs_writer_stop();
    EXPECT_EQ(received.load(), 1u);
    EXPECT_TRUE(closed);
    EXPECT_FALSE(authum_logs_writer_submit(&record));
}

TEST_F(authum_log_writer, restart_resets_drop_counter)
{
    authum_logs_record_t record = {0};
    strcpy(record.text, "owned text");
    ASSERT_TRUE(authum_logs_writer_submit(&record));
    ASSERT_NO_FATAL_FAILURE(wait_for_sink());
    for (authum_u32_t index = 1; index < AUTHUM_LOGS_QUEUE_CAPACITY; ++index)
    {
        record.keep_days = index;
        ASSERT_TRUE(authum_logs_writer_submit(&record));
    }
    EXPECT_FALSE(authum_logs_writer_submit(&record));
    EXPECT_EQ(authum_logs_writer_dropped(), 1u);
    released = true;
    authum_logs_writer_stop();
    ASSERT_TRUE(authum_logs_writer_start(&sink));
    EXPECT_EQ(authum_logs_writer_dropped(), 0u);
}

} // namespace tests
