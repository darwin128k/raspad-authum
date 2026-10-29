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
    authum_unused(user);
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
    authum_unused(user);
    CHECK(thread_id() == main_thread);
    ++completed;
}

void
destroy(void *user)
{
    authum_unused(user);
    CHECK(thread_id() == main_thread);
    ++destroyed;
    if (closing_tasks)
    {
        CHECK(!authum_tasks_submit(closing_tasks, work, NULL, NULL, NULL));
    }
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
    authum_unused(user);
    return authum_sql_exec(db, "SELECT * FROM missing_table");
}

authum_bool_t
sql_swallowed(authum_sql_t *db, void *user)
{
    authum_unused(user);
    (void)authum_sql_exec(db, "SELECT * FROM missing_table");
    return authum_sql_exec(db, "SELECT 1");
}

authum_bool_t
sql_handled(authum_sql_t *db, void *user)
{
    authum_unused(user);
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

typedef struct scheduler_case scheduler_case_t;

typedef struct scheduler_owner
{
    authum_tasks_t tasks;
    scheduler_case_t *test;
    authum_usize_t id;
    authum_usize_t destroyed;
    atomic_int ready;
    authum_bool_t closing;
} scheduler_owner_t;

struct scheduler_case
{
    scheduler_owner_t owners[4];
    authum_usize_t order[32];
    authum_usize_t count;
    authum_bool_t reentrant;
    authum_bool_t mutated;
    authum_bool_t added;
    authum_bool_t resubmitted;
};

static void
scheduler_enqueue(scheduler_owner_t *owner);

static void
scheduler_work(authum_ptr user)
{
    authum_unused(user);
}

static void
scheduler_ready(authum_ptr user)
{
    scheduler_owner_t *owner = user;
    /* FIFO execution means the three earlier completions are now queued. */
    atomic_store(&owner->ready, 1);
}

static void
scheduler_destroy(authum_ptr user)
{
    scheduler_owner_t *owner = user;
    CHECK(thread_id() == main_thread);
    CHECK(authum_tasks_host_dispatching());
    ++owner->destroyed;
    if (owner->closing)
    {
        CHECK(!authum_tasks_submit(&owner->tasks, scheduler_work, authum_null, authum_null,
                                   authum_null));
        if (!owner->test->added)
        {
            owner->test->added = authum_bool_true;
            scheduler_enqueue(&owner->test->owners[3]);
        }
    }
}

static void
scheduler_complete(authum_ptr user)
{
    scheduler_owner_t *owner = user;
    scheduler_case_t *test = owner->test;
    CHECK(thread_id() == main_thread);
    CHECK(authum_tasks_host_dispatching());
    CHECK(test->count < sizeof test->order / sizeof test->order[0]);
    if (test->count < sizeof test->order / sizeof test->order[0])
    {
        test->order[test->count++] = owner->id;
    }
    if (test->reentrant && owner->id == 2 && !test->mutated)
    {
        test->mutated = authum_bool_true;
        /* Pump has just moved this owner to the tail. A destructor prepends
         * another owner while the stopped tail is still registered. */
        owner->closing = authum_bool_true;
        authum_tasks_host_drop_owner(owner);
        owner->closing = authum_bool_false;
    }
    if (test->reentrant && owner->id == 1 && !test->resubmitted)
    {
        test->resubmitted = authum_bool_true;
        CHECK(authum_tasks_submit(&owner->tasks, scheduler_work, scheduler_complete,
                                  scheduler_destroy, owner));
    }
}

static void
scheduler_enqueue(scheduler_owner_t *owner)
{
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        CHECK(authum_tasks_submit(&owner->tasks, scheduler_work, scheduler_complete,
                                  scheduler_destroy, owner));
    }
    CHECK(
        authum_tasks_submit(&owner->tasks, scheduler_ready, authum_null, scheduler_destroy, owner));
}

static void
scheduler_check(authum_bool_t reentrant, const authum_usize_t drop_order[3])
{
    scheduler_case_t test = {0};
    test.reentrant = reentrant;
    for (authum_usize_t index = 0; index < 4; ++index)
    {
        scheduler_owner_t *owner = &test.owners[index];
        owner->test = &test;
        owner->id = index;
        owner->tasks.ops = authum_tasks_host_ops();
        owner->tasks.owner = owner;
        atomic_init(&owner->ready, 0);
    }
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        scheduler_enqueue(&test.owners[index]);
    }
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        for (authum_usize_t attempt = 0; attempt < 5000 && !atomic_load(&test.owners[index].ready);
             ++attempt)
        {
            pause_ms();
        }
        CHECK(atomic_load(&test.owners[index].ready));
    }
    const authum_usize_t expected = reentrant ? 11u : 9u;
    for (authum_usize_t attempt = 0; attempt < 5000 && test.count < expected; ++attempt)
    {
        authum_tasks_host_pump();
        pause_ms();
    }
    CHECK(test.count == expected);
    if (!reentrant)
    {
        for (authum_usize_t index = 0; index < expected; ++index)
        {
            CHECK(test.order[index] == 2u - index % 3u);
        }
    }
    else
    {
        CHECK(test.mutated && test.added && test.resubmitted);
        authum_usize_t counts[4] = {0};
        for (authum_usize_t index = 0; index < test.count; ++index)
        {
            ++counts[test.order[index]];
        }
        CHECK(counts[0] == 3 && counts[1] == 4 && counts[2] == 1 && counts[3] == 3);
    }
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        authum_tasks_host_drop_owner(&test.owners[drop_order[index]]);
    }
    authum_tasks_host_drop_owner(&test.owners[3]);
    for (authum_usize_t index = 0; index < 4; ++index)
    {
        CHECK(test.owners[index].destroyed ==
              (index == 3 ? (reentrant ? 4u : 0u) : (reentrant && index == 1 ? 5u : 4u)));
    }
    CHECK(!authum_tasks_host_dispatching());
    authum_tasks_host_pump();
    CHECK(test.count == expected);
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
    host.tasks = &tasks;
    memset(&connection, 0, sizeof connection);
    connection.driver = authum_sql_driver_sqlite;
    connection.path = path;
    authum_char_t optional[] = "copied option";
    connection.host = optional;
    connection.user = "";
    connection.database = optional;
    connection.password = optional;
    CHECK(authum_sql_thread_query(&host, &connection, sql_work, sql_complete, NULL, &sql));
    memset(optional, 'x', sizeof optional - 1);
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
    /* Missing SQLite path fails asynchronously and still releases the payload. */
    memset(&connection, 0, sizeof connection);
    connection.driver = authum_sql_driver_sqlite;
    CHECK(authum_sql_thread_query(&host, &connection, sql_work, sql_complete, destroy, &sql));
    pump_until(69);
    CHECK(!sql.connected);
    CHECK(!sql.success);
    CHECK(destroyed == 75);
    authum_tasks_host_drop_owner(&owner);
    /* Exercise head, middle, tail and final removal, then reuse an empty list. */
    const authum_usize_t drop_orders[][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                             {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (authum_usize_t index = 0; index < sizeof drop_orders / sizeof drop_orders[0]; ++index)
    {
        scheduler_check(authum_bool_false, drop_orders[index]);
    }
    scheduler_check(authum_bool_true, drop_orders[0]);
    scheduler_check(authum_bool_false, drop_orders[0]);
    return authum_check_report("tasks + threaded SQL");
}
