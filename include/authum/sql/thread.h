#ifndef AUTHUM_SQL_THREAD_H
#define AUTHUM_SQL_THREAD_H

#include <authum/host.h>
#include <authum/sql.h>

/** A connection description, not a live handle. Strings are copied on submit.
 * Use either the SQLite path or the MySQL fields, according to driver.
 */
typedef struct authum_sql_connection
{
    authum_sql_driver_t driver;
    const authum_char_t *path, *host, *user, *password, *database;
    authum_u32_t port;
} authum_sql_connection_t;
typedef struct authum_sql_thread_result
{
    authum_bool_t connected, success;
    authum_char_t error[authum_sql_errmsg_max];
} authum_sql_thread_result_t;

AUTHUM_EXTERN_C_BEGIN

/** Worker-only SQL batch. Copy wanted rows into user-owned storage before
 * returning. No engine/host-log/event APIs here. Immutable environment/path
 * getters and a private buffered logger are allowed. Finalize every statement.
 */
typedef authum_bool_t (*authum_sql_thread_work_fn)(authum_sql_t *db, authum_ptr user);
typedef void (*authum_sql_thread_complete_fn)(const authum_sql_thread_result_t *result,
                                              authum_ptr user);

/** Synchronous connect using the same description as a threaded query. */
authum_bool_t
authum_sql_connect(authum_sql_t *db, const authum_sql_connection_t *connection);

/** New private connection per task, closed in the worker after work returns.
 * complete and destroy run on the game thread; no completion after unload.
 * As with tasks_submit, false leaves user ownership with the caller.
 * SQLite :memory: databases are private per task; use a file to share data.
 */
authum_bool_t
authum_sql_thread_query(const authum_host_t *host, const authum_sql_connection_t *connection,
                        authum_sql_thread_work_fn work, authum_sql_thread_complete_fn complete,
                        authum_task_fn destroy, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif
