#include <authum/host/logs/writer.h>
#include <authum/unused.h>
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
    authum_unused(lh_os_thread_sleep(10));
}

/**
 * @brief Publish the cooperative stop request to the disk worker.
 */
static void
stop_writer(authum_ptr user)
{
    authum_unused(user);
    atomic_store_explicit(lh_addr_of(stopping), 1, memory_order_release);
}

/**
 * @brief Drain accepted records and maintain flush barriers until shutdown completes.
 */
static void
run_writer(authum_ptr user)
{
    authum_i64_t last_flush = authum_clock_get_monotonic_ms(authum_clock_host());
    authum_u32_t reported = 0;
    authum_unused(user);
    for (;;)
    {
        authum_u32_t request = atomic_load_explicit(lh_addr_of(requested), memory_order_acquire);
        /* Bound a batch so sustained logging cannot starve flush/shutdown. */
        for (authum_usize_t count = 0; lh_math_lt(count, AUTHUM_LOGS_QUEUE_CAPACITY);
             count = lh_math_add_one(count))
        {
            const authum_logs_record_t *record = lh_spsc_peek(records);
            if (!record)
            {
                break;
            }
            sink.emit(record);
            lh_spsc_release(records);
        }
        authum_u32_t losses = atomic_load_explicit(lh_addr_of(dropped), memory_order_relaxed);
        if (sink.dropped && lh_math_ne(losses, reported))
        {
            sink.dropped(lh_math_sub(losses, reported));
            reported = losses;
        }
        authum_i64_t now = authum_clock_get_monotonic_ms(authum_clock_host());
        if (lh_math_ne(request,
                       atomic_load_explicit(lh_addr_of(completed), memory_order_relaxed)) ||
            lh_math_ge(lh_math_sub(now, last_flush), 100))
        {
            sink.flush();
            last_flush = now;
            atomic_store_explicit(lh_addr_of(completed), request, memory_order_release);
        }
        if (atomic_load_explicit(lh_addr_of(stopping), memory_order_acquire) &&
            !lh_spsc_peek(records))
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
    atomic_init(lh_addr_of(dropped), 0);
    atomic_init(lh_addr_of(requested), 0);
    atomic_init(lh_addr_of(completed), 0);
    atomic_init(lh_addr_of(stopping), 0);
    if (!custom)
    {
        return authum_bool_false;
    }
    sink = lh_ptr_deref(custom);
    if (!sink.emit || !sink.flush || !sink.close)
    {
        return authum_bool_false;
    }
    records = lh_spsc_create(queue, AUTHUM_LOGS_QUEUE_CAPACITY, sizeof queue[0]);
    if (!records)
    {
        return authum_bool_false;
    }
    authum_threads_host_init(lh_addr_of(owner), lh_addr_of(threads));
    worker = authum_thread_start(lh_addr_of(threads), run_writer, stop_writer, authum_null, -1, 0,
                                 authum_null, 0);
    if (!worker)
    {
        lh_spsc_destroy(records);
        records = authum_null;
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
        atomic_fetch_add_explicit(lh_addr_of(dropped), 1, memory_order_relaxed);
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
    authum_u32_t request =
        lh_math_add(atomic_fetch_add_explicit(lh_addr_of(requested), 1, memory_order_release), 1);
    authum_i64_t start = authum_clock_get_monotonic_ms(authum_clock_host());
    while (lh_math_ne(atomic_load_explicit(lh_addr_of(completed), memory_order_acquire), request))
    {
        if (lh_math_ge(lh_math_sub(authum_clock_get_monotonic_ms(authum_clock_host()), start),
                       timeout_ms))
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
        authum_unused(authum_thread_stop(lh_addr_of(threads), worker));
        worker = authum_null;
        authum_threads_host_drop_owner(lh_addr_of(owner));
        lh_spsc_destroy(records);
        records = authum_null;
    }
}

/**
 * @brief Read the cumulative count of records rejected by queue overflow.
 */
authum_u32_t
authum_logs_writer_dropped(void)
{
    return atomic_load_explicit(lh_addr_of(dropped), memory_order_relaxed);
}
