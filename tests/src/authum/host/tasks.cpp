#include <authum/client.h>
#include <authum/host/tasks.h>
#include <gtest/gtest.h>
#include <atomic>
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
    nanosleep(&t, authum_null);
}
#endif

static std::atomic_int release_work, worked;
static int completed, destroyed, worker_is_main;
static thread_id_t main_thread;
static const authum_tasks_t *closing_tasks;

void
work(void *user)
{
    authum_unused(user);
    worker_is_main = thread_id() == main_thread;
    while (!std::atomic_load(&release_work))
    {
        pause_ms();
    }
    std::atomic_fetch_add(&worked, 1);
}

void
complete(void *user)
{
    authum_unused(user);
    EXPECT_EQ(thread_id(), main_thread);
    ++completed;
}

void
destroy(void *user)
{
    authum_unused(user);
    EXPECT_EQ(thread_id(), main_thread);
    ++destroyed;
    if (closing_tasks)
    {
        EXPECT_FALSE(
            authum_tasks_submit(closing_tasks, work, authum_null, authum_null, authum_null));
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
    EXPECT_EQ(completed, count);
}

typedef struct sql_case
{
    int value, wrong_thread, success, connected;
} sql_case_t;

authum_bool_t
sql_work(authum_sql_t *db, void *user)
{
    sql_case_t *test = static_cast<sql_case_t *>(user);
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
    sql_case_t *test = static_cast<sql_case_t *>(user);
    EXPECT_EQ(thread_id(), main_thread);
    test->success = result->success;
    test->connected = result->connected;
    if (!result->success)
    {
        EXPECT_TRUE(result->error[0]);
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
    std::atomic_int ready;
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

void
scheduler_enqueue(scheduler_owner_t *owner);

void
scheduler_work(authum_ptr user)
{
    authum_unused(user);
}

void
scheduler_ready(authum_ptr user)
{
    scheduler_owner_t *owner = static_cast<scheduler_owner_t *>(user);
    /* FIFO execution means the three earlier completions are now queued. */
    std::atomic_store(&owner->ready, 1);
}

void
scheduler_destroy(authum_ptr user)
{
    scheduler_owner_t *owner = static_cast<scheduler_owner_t *>(user);
    EXPECT_EQ(thread_id(), main_thread);
    EXPECT_TRUE(authum_tasks_host_dispatching());
    ++owner->destroyed;
    if (owner->closing)
    {
        EXPECT_TRUE(!authum_tasks_submit(&owner->tasks, scheduler_work, authum_null, authum_null,
                                         authum_null));
        if (!owner->test->added)
        {
            owner->test->added = authum_bool_true;
            scheduler_enqueue(&owner->test->owners[3]);
        }
    }
}

void
scheduler_complete(authum_ptr user)
{
    scheduler_owner_t *owner = static_cast<scheduler_owner_t *>(user);
    scheduler_case_t *test = owner->test;
    EXPECT_EQ(thread_id(), main_thread);
    EXPECT_TRUE(authum_tasks_host_dispatching());
    EXPECT_LT(test->count, sizeof test->order / sizeof test->order[0]);
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
        EXPECT_TRUE(authum_tasks_submit(&owner->tasks, scheduler_work, scheduler_complete,
                                        scheduler_destroy, owner));
    }
}

void
scheduler_enqueue(scheduler_owner_t *owner)
{
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        EXPECT_TRUE(authum_tasks_submit(&owner->tasks, scheduler_work, scheduler_complete,
                                        scheduler_destroy, owner));
    }
    EXPECT_TRUE(
        authum_tasks_submit(&owner->tasks, scheduler_ready, authum_null, scheduler_destroy, owner));
}

void
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
        std::atomic_init(&owner->ready, 0);
    }
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        scheduler_enqueue(&test.owners[index]);
    }
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        for (authum_usize_t attempt = 0;
             attempt < 5000 && !std::atomic_load(&test.owners[index].ready); ++attempt)
        {
            pause_ms();
        }
        EXPECT_TRUE(std::atomic_load(&test.owners[index].ready));
    }
    const authum_usize_t expected = reentrant ? 11u : 9u;
    for (authum_usize_t attempt = 0; attempt < 5000 && test.count < expected; ++attempt)
    {
        authum_tasks_host_pump();
        pause_ms();
    }
    EXPECT_EQ(test.count, expected);
    if (!reentrant)
    {
        for (authum_usize_t index = 0; index < expected; ++index)
        {
            EXPECT_EQ(test.order[index], 2u - index % 3u);
        }
    }
    else
    {
        EXPECT_TRUE(test.mutated && test.added && test.resubmitted);
        authum_usize_t counts[4] = {0};
        for (authum_usize_t index = 0; index < test.count; ++index)
        {
            ++counts[test.order[index]];
        }
        EXPECT_TRUE(counts[0] == 3 && counts[1] == 4 && counts[2] == 1 && counts[3] == 3);
    }
    for (authum_usize_t index = 0; index < 3; ++index)
    {
        authum_tasks_host_drop_owner(&test.owners[drop_order[index]]);
    }
    authum_tasks_host_drop_owner(&test.owners[3]);
    for (authum_usize_t index = 0; index < 4; ++index)
    {
        EXPECT_TRUE(test.owners[index].destroyed ==
                    (index == 3 ? (reentrant ? 4u : 0u) : (reentrant && index == 1 ? 5u : 4u)));
    }
    EXPECT_FALSE(authum_tasks_host_dispatching());
    authum_tasks_host_pump();
    EXPECT_EQ(test.count, expected);
}

static int owner;
static authum_tasks_t tasks;
static authum_host_t host;
static authum_sql_connection_t connection;
static sql_case_t sql;

namespace tests
{

class authum_tasks : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        release_work = 0;
        worked = 0;
        completed = destroyed = worker_is_main = 0;
        closing_tasks = authum_null;
        main_thread = thread_id();
        tasks = {authum_tasks_host_ops(), &owner};
        host = {};
        host.tasks = &tasks;
        connection = {};
        connection.driver = authum_sql_driver_sqlite;
        connection.path = ":memory:";
        sql = {};
    }

    void
    TearDown() override
    {
        release_work = 1;
        authum_tasks_host_drop_owner(&owner);
        closing_tasks = authum_null;
    }
};

TEST_F(authum_tasks, rejects_missing_service)
{
    EXPECT_FALSE(authum_tasks_submit(authum_null, work, complete, destroy, authum_null));
}

TEST_F(authum_tasks, enforces_outstanding_job_limit)
{
    for (unsigned index = 0; index < 64; ++index)
    {
        ASSERT_TRUE(authum_tasks_submit(&tasks, work, complete, destroy, authum_null));
    }
    EXPECT_FALSE(authum_tasks_submit(&tasks, work, complete, destroy, authum_null));
    EXPECT_EQ(completed, 0);
    release_work = 1;
    pump_until(64);
    EXPECT_EQ(worked.load(), 64);
    EXPECT_EQ(destroyed, 64);
    EXPECT_FALSE(worker_is_main);
}

TEST_F(authum_tasks, completion_and_destructor_run_on_game_thread)
{
    ASSERT_TRUE(authum_tasks_submit(&tasks, work, complete, destroy, authum_null));
    release_work = 1;
    pump_until(1);
    EXPECT_EQ(worked.load(), 1);
    EXPECT_EQ(destroyed, 1);
    EXPECT_FALSE(worker_is_main);
}

TEST_F(authum_tasks, unload_cancels_completions_and_disposes_each_payload_once)
{
    for (unsigned index = 0; index < 10; ++index)
    {
        ASSERT_TRUE(authum_tasks_submit(&tasks, work, complete, destroy, authum_null));
    }
    release_work = 1;
    closing_tasks = &tasks;
    authum_tasks_host_drop_owner(&owner);
    closing_tasks = authum_null;
    EXPECT_EQ(completed, 0);
    EXPECT_EQ(destroyed, 10);
    authum_tasks_host_pump();
    EXPECT_EQ(completed, 0);
}

TEST_F(authum_tasks, sql_submission_copies_connection_strings)
{
    char path[] = ":memory:";
    authum_char_t optional[] = "copied option";
    connection.path = path;
    connection.host = optional;
    connection.user = "";
    connection.database = optional;
    connection.password = optional;
    ASSERT_TRUE(
        authum_sql_thread_query(&host, &connection, sql_work, sql_complete, authum_null, &sql));
    memset(optional, 'x', sizeof optional - 1);
    memset(path, 'x', sizeof path - 1);
    pump_until(1);
    EXPECT_EQ(sql.value, 42);
    EXPECT_TRUE(sql.success);
    EXPECT_TRUE(sql.connected);
    EXPECT_FALSE(sql.wrong_thread);
}

TEST_F(authum_tasks, sql_query_failure_reports_connected_but_unsuccessful)
{
    ASSERT_TRUE(
        authum_sql_thread_query(&host, &connection, sql_bad, sql_complete, authum_null, &sql));
    pump_until(1);
    EXPECT_FALSE(sql.success);
    EXPECT_TRUE(sql.connected);
}

TEST_F(authum_tasks, sql_success_does_not_hide_unacknowledged_error)
{
    ASSERT_TRUE(authum_sql_thread_query(&host, &connection, sql_swallowed, sql_complete,
                                        authum_null, &sql));
    pump_until(1);
    EXPECT_FALSE(sql.success);
    EXPECT_TRUE(sql.connected);
}

TEST_F(authum_tasks, sql_acknowledged_error_allows_intentional_fallback)
{
    ASSERT_TRUE(
        authum_sql_thread_query(&host, &connection, sql_handled, sql_complete, authum_null, &sql));
    pump_until(1);
    EXPECT_TRUE(sql.success);
}

TEST_F(authum_tasks, sql_connection_failure_still_disposes_payload)
{
    connection.path = authum_null;
    ASSERT_TRUE(authum_sql_thread_query(&host, &connection, sql_work, sql_complete, destroy, &sql));
    pump_until(1);
    EXPECT_FALSE(sql.connected);
    EXPECT_FALSE(sql.success);
    EXPECT_EQ(destroyed, 1);
}

class authum_tasks_drop_order : public authum_tasks, public ::testing::WithParamInterface<unsigned>
{
};

static const authum_usize_t drop_orders[][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                                {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};

TEST_P(authum_tasks_drop_order, removes_owners_without_corrupting_round_robin_order)
{
    scheduler_check(authum_bool_false, drop_orders[GetParam()]);
}

INSTANTIATE_TEST_SUITE_P(all_owner_orders, authum_tasks_drop_order,
                         ::testing::Values(0u, 1u, 2u, 3u, 4u, 5u));

TEST_F(authum_tasks, callbacks_can_mutate_owner_list_and_resubmit)
{
    scheduler_check(authum_bool_true, drop_orders[0]);
    scheduler_check(authum_bool_false, drop_orders[0]);
}

} // namespace tests
