/**
 * @file writer.h
 * @brief Bounded single-producer log queue and its owned disk worker.
 *
 * Lifecycle and submission run on the owner/game thread. Accepted records are
 * copied into queue-owned storage without allocation, locks, file I/O or waiting.
 * Overflow drops the newest record. Sink callbacks run only on the worker.
 * The worker requests periodic flushes every 100 ms; shutdown drains and joins it.
 */

#ifndef AUTHUM_HOST_LOGS_WRITER_H
#define AUTHUM_HOST_LOGS_WRITER_H

#include <authum/path.h>
#include <authum/logger/level.h>
#include <authum/logger/options.h>
#include <time.h>

enum
{
    AUTHUM_LOGS_QUEUE_CAPACITY = 1024
};

typedef struct authum_logs_record
{
    authum_path_t dir;
    authum_char_t text[1024];
    authum_char_t tag[128];
    time_t time;
    long keep_days;
    authum_logger_level_t level;
    authum_logger_format_t format;
    /** Control record: register/update retention without creating a file. */
    authum_bool_t maintenance;
} authum_logs_record_t;

/** Worker-only callbacks; injectable so tests can simulate a stalled disk. */
typedef struct authum_logs_sink
{
    void (*emit)(const authum_logs_record_t *record);
    void (*flush)(void);
    void (*close)(void);
    void (*dropped)(authum_u32_t count);
} authum_logs_sink_t;

/**
 * @brief Reset the queue and start its owned disk worker with the supplied sink.
 */
authum_bool_t
authum_logs_writer_start(const authum_logs_sink_t *sink);

/**
 * @brief Copy a record into the bounded single-producer queue without waiting.
 */
authum_bool_t
authum_logs_writer_submit(const authum_logs_record_t *record);

/**
 * @brief Drain accepted records, join the disk worker and release its owner.
 */
void
authum_logs_writer_stop(void);

/**
 * @brief Wait for a disk flush barrier for tests or administration, outside frame hooks.
 */
authum_bool_t
authum_logs_writer_sync(authum_u32_t timeout_ms);

/**
 * @brief Read the cumulative count of overflow rejections since writer startup.
 */
authum_u32_t
authum_logs_writer_dropped(void);

#endif
