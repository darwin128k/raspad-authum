/**
 * @file private.h
 * @brief Private to `thread.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_SQL_THREAD_PRIVATE_H
#define AUTHUM_API_SQL_THREAD_PRIVATE_H

#include <authum/sql/thread.h>
#include <lh/core/attribute/thread_local.h>
#include <lh/core/memory/std.h>
#include <lh/os/sql/mysql/api.h>

typedef struct authum_sql_thread_copy
{
    authum_sql_connection_t connection;
    authum_usize_t strings_size;
    authum_char_t strings[];
} authum_sql_thread_copy_t;

typedef struct authum_sql_thread_job
{
    authum_sql_thread_work_fn work;
    authum_sql_thread_complete_fn complete;
    authum_task_fn destroy;
    authum_ptr user;
    /** No end-of-thread call on this host: the job closes its own connection. */
    authum_bool_t own_connection;
    authum_sql_thread_result_t result;
    authum_sql_connection_t connection;
    authum_usize_t strings_size;
    authum_char_t strings[];
} authum_sql_thread_job_t;

/** The connection a task thread keeps between jobs; that thread's alone. */
typedef struct authum_sql_thread_cache
{
    authum_sql_t db;
    /** The description db was opened with (null: no connection kept). */
    authum_sql_thread_copy_t *key;
    /** The MySQL client was used on this thread: mysql_thread_end at its end. */
    authum_bool_t used_mysql;
} authum_sql_thread_cache_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Bytes the strings of @p c take with their terminators; 0 on overflow past @p limit.
 */
authum_usize_t
authum_sql_thread_strings_size(const authum_sql_connection_t *c, authum_usize_t limit);

/**
 * @brief Copy @p c into @p dst, its strings into @p strings (sized by
 * authum_sql_thread_strings_size).
 */
void
authum_sql_thread_copy_init(authum_sql_connection_t *dst, authum_char_t *strings,
                            const authum_sql_connection_t *c);

/**
 * @brief Whether two optional strings are equal (both absent counts as equal).
 */
authum_bool_t
authum_sql_thread_same_text(const authum_char_t *a, const authum_char_t *b);

/**
 * @brief Whether @p a and @p b describe the same connection.
 */
authum_bool_t
authum_sql_thread_same(const authum_sql_connection_t *a, const authum_sql_connection_t *b);

/**
 * @brief Close the connection this thread keeps, if any, and forget its description.
 */
void
authum_sql_thread_drop(authum_sql_thread_cache_t *cache);

/**
 * @brief The connection this thread keeps for @p c: kept, reopened, or opened now.
 */
authum_sql_t *
authum_sql_thread_connection(authum_sql_thread_cache_t *cache, const authum_sql_connection_t *c,
                             authum_usize_t strings_size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_SQL_THREAD_PRIVATE_H */
