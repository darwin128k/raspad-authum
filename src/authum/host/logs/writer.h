#ifndef AUTHUM_HOST_LOGS_WRITER_H
#define AUTHUM_HOST_LOGS_WRITER_H
#include <authum/path.h>
#include <authum/logger/level.h>
#include <authum/byte.h>
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
} authum_logs_record_t;

/** Worker-only callbacks; injectable so tests can simulate a stalled disk. */
typedef struct authum_logs_sink
{
    void (*emit)(const authum_logs_record_t *record);
    void (*flush)(void);
    void (*close)(void);
    void (*dropped)(authum_u32_t count);
} authum_logs_sink_t;

/** Lifecycle and submit are owner/game-thread only. Queue owns every accepted record.
 * No allocation, locks, file I/O or waiting in submit. Drop newest on overflow.
 * Worker flushes every 100ms; shutdown drains the queue and joins the worker. */
authum_bool_t
authum_logs_writer_start(const authum_logs_sink_t *sink);
authum_bool_t
authum_logs_writer_submit(const authum_logs_record_t *record);
void
authum_logs_writer_stop(void);
/** Blocking barrier for tests/admin ONLY; never call from frame hooks. */
authum_bool_t
authum_logs_writer_sync(authum_u32_t timeout_ms);
authum_u32_t
authum_logs_writer_dropped(void);
#endif
