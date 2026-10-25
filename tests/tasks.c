#include <authum/client.h>
#include <authum/host/tasks.h>
#include "check.h"
#include <stdatomic.h>
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
    nanosleep(&t, NULL);
}
#endif

static atomic_int release_work, worked;
static int completed, destroyed, worker_is_main;
static thread_id_t main_thread;
static const authum_tasks_t *closing_tasks;
void
work(void *user)
{
    (void)user;
    worker_is_main = thread_id() == main_thread;
    while (!atomic_load(&release_work))
    {
        pause_ms();
    }
    atomic_fetch_add(&worked, 1);
}
void
complete(void *user)
{
    (void)user;
    CHECK(thread_id() == main_thread);
    ++completed;
}
void
destroy(void *user)
{
    (void)user;
    CHECK(thread_id() == main_thread);
    ++destroyed;
    if (closing_tasks)
        CHECK(!authum_tasks_submit(closing_tasks, work, NULL, NULL, NULL));
}
void
pump_until(int count)
{
    unsigned i;
    for (i = 0; i < 5000 && completed < count; ++i)
    {
        authum_tasks_host_pump();
        pause_ms();
    }
    CHECK(completed == count);
}
typedef struct sql_case
{
    int value, wrong_thread, success, connected;
} sql_case_t;
authum_bool_t
sql_work(authum_sql_t *db, void *user)
{
    sql_case_t *test = user;
    authum_sql_stmt_t stmt;
    test->wrong_thread = thread_id() == main_thread;
    if (!authum_sql_exec(db, "CREATE TABLE t (value INTEGER)") ||
        !authum_sql_exec(db, "INSERT INTO t VALUES (42)") ||
        !authum_sql_prepare(db, &stmt, "SELECT value FROM t"))
    {
        return authum_bool_false;
    }
    if (!authum_sql_stmt_step_row(&stmt))
    {
        authum_sql_stmt_finalize(&stmt);
        return authum_bool_false;
    }
    test->value = authum_sql_stmt_column_i32(&stmt, 0);
    authum_sql_stmt_finalize(&stmt);
    return authum_bool_true;
}
authum_bool_t
sql_bad(authum_sql_t *db, void *user)
{
    (void)user;
    return authum_sql_exec(db, "SELECT * FROM missing_table");
}
authum_bool_t sql_swallowed(authum_sql_t *db, void *user)
{
    (void)user;
    (void)authum_sql_exec(db, "SELECT * FROM missing_table");
    return authum_sql_exec(db, "SELECT 1");
}
authum_bool_t sql_handled(authum_sql_t *db, void *user)
{
    (void)user;
    (void)authum_sql_exec(db, "SELECT * FROM missing_table");
    authum_sql_clear_errors(db);
    return authum_sql_exec(db, "SELECT 1");
}
void
sql_complete(const authum_sql_thread_result_t *result, void *user)
{
    sql_case_t *test = user;
    CHECK(thread_id() == main_thread);
    test->success = result->success;
    test->connected = result->connected;
    if (!result->success)
    {
        CHECK(result->error[0]);
    }
    ++completed;
}
int
main(void)
{
    int owner = 1;
    authum_tasks_t tasks = {authum_tasks_host_ops(), &owner};
    authum_host_t host;
    authum_sql_connection_t connection;
    sql_case_t sql = {0};
    unsigned i;
    char path[] = ":memory:";
    main_thread = thread_id();
    CHECK(!authum_tasks_submit(NULL, work, complete, destroy, NULL));
    for (i = 0; i < 64; ++i)
    {
        CHECK(authum_tasks_submit(&tasks, work, complete, destroy, NULL));
    }
    CHECK(!authum_tasks_submit(&tasks, work, complete, destroy, NULL));
    CHECK(completed == 0);
    atomic_store(&release_work, 1);
    pump_until(64);
    CHECK(atomic_load(&worked) == 64);
    CHECK(destroyed == 64);
    CHECK(!worker_is_main);
    authum_tasks_host_drop_owner(&owner);

    /* Cancellation drains storage without calling module completions. */
    atomic_store(&release_work, 0);
    for (i = 0; i < 10; ++i)
    {
        CHECK(authum_tasks_submit(&tasks, work, complete, destroy, NULL));
    }
    atomic_store(&release_work, 1);
    closing_tasks = &tasks;
    authum_tasks_host_drop_owner(&owner);
    closing_tasks = NULL;
    CHECK(completed == 64);
    CHECK(destroyed == 74);
    authum_tasks_host_pump();
    CHECK(completed == 64);

    memset(&host, 0, sizeof host);
    host.size = sizeof host;
    host.tasks = &tasks;
    memset(&connection, 0, sizeof connection);
    connection.driver = authum_sql_driver_sqlite;
    connection.path = path;
    CHECK(authum_sql_thread_query(&host, &connection, sql_work, sql_complete, NULL, &sql));
    memset(path, 'x', sizeof path - 1); /* request owns its copy */
    pump_until(65);
    CHECK(sql.value == 42);
    CHECK(sql.success);
    CHECK(sql.connected);
    CHECK(!sql.wrong_thread);
    connection.path = ":memory:";
    CHECK(authum_sql_thread_query(&host, &connection, sql_bad, sql_complete, NULL, &sql));
    pump_until(66);
    CHECK(!sql.success);
    CHECK(sql.connected);
    CHECK(authum_sql_thread_query(&host, &connection, sql_swallowed, sql_complete, NULL, &sql));
    pump_until(67);
    CHECK(!sql.success); /* success of a later query must not erase the failure */
    CHECK(authum_sql_thread_query(&host, &connection, sql_handled, sql_complete, NULL, &sql));
    pump_until(68);
    CHECK(sql.success); /* explicit acknowledgement permits an intentional fallback */
    authum_tasks_host_drop_owner(&owner);
    return authum_check_report("tasks + threaded SQL");
}
