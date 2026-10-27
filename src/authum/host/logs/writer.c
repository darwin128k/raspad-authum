#include "writer.h"
#include <authum/host/threads.h>
#include <authum/host/clock.h>
#include <stdatomic.h>
#include <lh/spsc.h>
#include <string.h>
#include <lh/os/thread.h>

/* Single producer (the host thread), single consumer (the disk writer).
 * Separate published indices keep partially copied records invisible. */
static authum_logs_record_t queue[AUTHUM_LOGS_QUEUE_CAPACITY];
static lh_spsc_t *records;
static atomic_uint dropped, requested, completed;
static atomic_bool stopping;
static authum_thread_owner_t owner;
static authum_threads_t threads;
static authum_thread_t *worker;
static authum_logs_sink_t sink;

/**
 * @brief Yield the disk worker between empty-queue checks using LH sleep.
 */
static void
pause_writer(void)
{
    (void)lh_os_thread_sleep(10);
}

/**
 * @brief Publish the cooperative stop request to the disk worker.
 */
static void
stop_writer(authum_ptr user)
{
    (void)user;
    atomic_store_explicit(&stopping, 1, memory_order_release);
}

/**
 * @brief Drain accepted records and maintain flush barriers until shutdown completes.
 */
static void
run_writer(authum_ptr user)
{
    authum_i64_t last_flush = authum_clock_monotonic_ms(authum_clock_host());
    unsigned reported = 0;
    (void)user;
    for (;;)
    {
        unsigned request = atomic_load_explicit(&requested, memory_order_acquire);
        /* Bound a batch so sustained logging cannot starve flush/shutdown. */
        for (unsigned count = 0; count < AUTHUM_LOGS_QUEUE_CAPACITY; ++count)
        {
            const authum_logs_record_t *record = lh_spsc_peek(records);
            if (!record)
            {
                break;
            }
            sink.emit(record);
            lh_spsc_release(records);
        }
        unsigned losses = atomic_load_explicit(&dropped, memory_order_relaxed);
        if (sink.dropped && losses != reported)
        {
            sink.dropped(losses - reported);
            reported = losses;
        }
        authum_i64_t now = authum_clock_monotonic_ms(authum_clock_host());
        if (request != atomic_load_explicit(&completed, memory_order_relaxed) ||
            now - last_flush >= 100)
        {
            sink.flush();
            last_flush = now;
            atomic_store_explicit(&completed, request, memory_order_release);
        }
        if (atomic_load_explicit(&stopping, memory_order_acquire) && !lh_spsc_peek(records))
        {
            break;
        }
        if (!lh_spsc_peek(records))
        {
            pause_writer();
        }
    }
    sink.flush();
    sink.close();
}

/**
 * @brief Initialize the bounded log queue and start its owned disk worker.
 */
authum_bool_t
authum_logs_writer_start(const authum_logs_sink_t *custom)
{
    authum_logs_writer_stop();
    atomic_init(&dropped, 0);
    atomic_init(&requested, 0);
    atomic_init(&completed, 0);
    atomic_init(&stopping, 0);
    if (!custom)
    {
        return authum_bool_false;
    }
    sink = *custom;
    if (!sink.emit || !sink.flush || !sink.close)
    {
        return authum_bool_false;
    }
    records = lh_spsc_create(queue, AUTHUM_LOGS_QUEUE_CAPACITY, sizeof queue[0]);
    if (!records)
    {
        return authum_bool_false;
    }
    authum_threads_host_init(&owner, &threads);
    worker = authum_thread_start(&threads, run_writer, stop_writer, NULL, -1, 0, NULL, 0);
    if (!worker)
    {
        lh_spsc_destroy(records);
        records = NULL;
    }
    return worker ? authum_bool_true : authum_bool_false;
}

/**
 * @brief Copy one record into the bounded queue without allocating or waiting.
 */
authum_bool_t
authum_logs_writer_submit(const authum_logs_record_t *record)
{
    if (!worker || !record)
    {
        return authum_bool_false;
    }
    if (!lh_spsc_push(records, record))
    {
        atomic_fetch_add_explicit(&dropped, 1, memory_order_relaxed);
        return authum_bool_false;
    }
    return authum_bool_true;
}

/**
 * @brief Wait for a flush barrier for tests or administration, outside game-frame hooks.
 */
authum_bool_t
authum_logs_writer_sync(authum_u32_t timeout_ms)
{
    if (!worker)
    {
        return authum_bool_false;
    }
    unsigned request = atomic_fetch_add_explicit(&requested, 1, memory_order_release) + 1;
    authum_i64_t start = authum_clock_monotonic_ms(authum_clock_host());
    while (atomic_load_explicit(&completed, memory_order_acquire) != request)
    {
        if (authum_clock_monotonic_ms(authum_clock_host()) - start >= timeout_ms)
        {
            return authum_bool_false;
        }
        pause_writer();
    }
    return authum_bool_true;
}

/**
 * @brief Drain queued records, join the writer and release its owner state.
 */
void
authum_logs_writer_stop(void)
{
    if (worker)
    {
        (void)authum_thread_stop(&threads, worker);
        worker = NULL;
        authum_threads_host_drop_owner(&owner);
        lh_spsc_destroy(records);
        records = NULL;
    }
}

/**
 * @brief Read the cumulative count of records rejected by queue overflow.
 */
authum_u32_t
authum_logs_writer_dropped(void)
{
    return atomic_load_explicit(&dropped, memory_order_relaxed);
}
