#include <authum/host/logs/writer.h>
#include <authum/host/clock.h>
#include <stdatomic.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static atomic_bool entered, released, closed;
static atomic_uint received, losses;
static authum_u32_t expected;

static void
pause_test(void)
{
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
}

static void
emit(const authum_logs_record_t *record)
{
    atomic_store(&entered, 1);
    while (!atomic_load(&released))
    {
        pause_test();
    }
    /* Caller mutates its source immediately after submit; the copy must survive. */
    assert(record->keep_days == (long)expected++);
    assert(strcmp(record->text, "owned text") == 0);
    atomic_fetch_add(&received, 1);
}

static void
flush(void)
{
}

static void
close_sink(void)
{
    atomic_store(&closed, 1);
}

static void
report_loss(authum_u32_t count)
{
    atomic_fetch_add(&losses, count);
}

int
main(void)
{
    const authum_logs_sink_t sink = {emit, flush, close_sink, report_loss};
    authum_logs_record_t record = {0};
    assert(authum_logs_writer_start(&sink));
    strcpy(record.text, "owned text");
    assert(authum_logs_writer_submit(&record));
    authum_i64_t start = authum_clock_get_monotonic_ms(authum_clock_host());
    while (!atomic_load(&entered))
    {
        assert(authum_clock_get_monotonic_ms(authum_clock_host()) - start < 5000);
        pause_test();
    }
    /* Sink is blocked until THIS thread releases it. A waiting submit deadlocks. */
    for (authum_u32_t index = 1; index < AUTHUM_LOGS_QUEUE_CAPACITY; ++index)
    {
        record.keep_days = index;
        assert(authum_logs_writer_submit(&record));
    }
    for (authum_u32_t index = 0; index < 57; ++index)
    {
        assert(!authum_logs_writer_submit(&record));
    }
    assert(authum_logs_writer_dropped() == 57);
    assert(!authum_logs_writer_sync(20));
    memset(&record, 0, sizeof record);
    atomic_store(&released, 1);
    assert(authum_logs_writer_sync(5000));
    assert(atomic_load(&received) == AUTHUM_LOGS_QUEUE_CAPACITY);
    assert(atomic_load(&losses) == 57);
    strcpy(record.text, "owned text");
    record.keep_days = AUTHUM_LOGS_QUEUE_CAPACITY;
    assert(authum_logs_writer_submit(&record));
    authum_logs_writer_stop();
    assert(atomic_load(&received) == AUTHUM_LOGS_QUEUE_CAPACITY + 1);
    assert(atomic_load(&closed));
    assert(!authum_logs_writer_submit(&record));
    assert(authum_logs_writer_start(&sink));
    assert(authum_logs_writer_dropped() == 0);
    authum_logs_writer_stop();
    puts("async logger: bounded nonblocking queue, FIFO, owned copies, barrier and drain OK");
    return 0;
}
