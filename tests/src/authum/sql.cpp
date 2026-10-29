#include <authum/sql.h>
#include <authum/cstr.h>
#include <gtest/gtest.h>

static authum_sql_t db;

namespace tests
{
class authum_sql : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        authum_sql_init(&db);
        ASSERT_TRUE(authum_sql_open_sqlite(&db, ":memory:"));
    }

    void
    TearDown() override
    {
        authum_sql_close(&db);
    }
};

TEST_F(authum_sql, success_clears_last_operation_but_keeps_first_error)
{
    ASSERT_FALSE(authum_sql_exec(&db, "SELECT * FROM missing_first"));
    char first[authum_sql_errmsg_max];
    authum_cstr_copy(first, sizeof first, authum_sql_first_error(&db));
    ASSERT_NE(first[0], 0);
    ASSERT_TRUE(authum_sql_exec(&db, "SELECT 1"));
    EXPECT_STREQ(authum_sql_errmsg(&db), "");
    EXPECT_STREQ(authum_sql_first_error(&db), first);
}

TEST_F(authum_sql, subsequent_failure_does_not_replace_first_error)
{
    ASSERT_FALSE(authum_sql_exec(&db, "SELECT * FROM missing_first"));
    char first[authum_sql_errmsg_max];
    authum_cstr_copy(first, sizeof first, authum_sql_first_error(&db));
    ASSERT_FALSE(authum_sql_exec(&db, "SELECT * FROM missing_second"));
    EXPECT_STREQ(authum_sql_first_error(&db), first);
}

TEST_F(authum_sql, acknowledgement_clears_error_history)
{
    ASSERT_FALSE(authum_sql_exec(&db, "SELECT * FROM missing_first"));
    authum_sql_clear_errors(&db);
    EXPECT_STREQ(authum_sql_first_error(&db), "");
    ASSERT_TRUE(authum_sql_ping(&db));
    EXPECT_STREQ(authum_sql_first_error(&db), "");
}

TEST_F(authum_sql, invalid_binding_records_first_error)
{
    authum_sql_stmt_t stmt;
    ASSERT_TRUE(authum_sql_prepare(&db, &stmt, "SELECT ?"));
    EXPECT_FALSE(authum_sql_stmt_bind_i32(&stmt, 0, 1));
    EXPECT_NE(authum_sql_first_error(&db)[0], 0);
    authum_sql_stmt_finalize(&stmt);
}

TEST_F(authum_sql, parameterized_sqlite_roundtrip_preserves_column_values)
{
    authum_sql_stmt_t stmt = {};
    ASSERT_TRUE(
        authum_sql_exec(&db, "CREATE TABLE bans (id INTEGER PRIMARY KEY, steam INTEGER, "
                             "name TEXT, reason TEXT); CREATE INDEX bans_steam ON bans(steam);"));
    ASSERT_TRUE(
        authum_sql_prepare(&db, &stmt, "INSERT INTO bans (steam, name, reason) VALUES (?, ?, ?)"));
    EXPECT_TRUE(authum_sql_stmt_bind_i64(&stmt, 1, 76561198000000001LL));
    EXPECT_TRUE(authum_sql_stmt_bind_cstr(&stmt, 2, "O'Brien"));
    EXPECT_TRUE(authum_sql_stmt_bind_cstr(&stmt, 3, authum_null));
    EXPECT_FALSE(authum_sql_stmt_bind_i32(&stmt, 0, 1));
    EXPECT_TRUE(authum_sql_stmt_step_done(&stmt));
    authum_sql_stmt_finalize(&stmt);
    EXPECT_EQ(authum_sql_last_insert_id(&db), 1);
    ASSERT_TRUE(
        authum_sql_prepare(&db, &stmt, "SELECT steam, name, reason FROM bans WHERE steam = ?"));
    EXPECT_TRUE(authum_sql_stmt_bind_i64(&stmt, 1, 76561198000000001LL));
    if (authum_sql_stmt_step_row(&stmt))
    {
        const auto *name = authum_sql_stmt_column_cstr(&stmt, 1);
        EXPECT_EQ(authum_sql_stmt_column_i64(&stmt, 0), 76561198000000001LL);
        EXPECT_STREQ(name, "O'Brien");
        EXPECT_STREQ(authum_sql_stmt_column_cstr(&stmt, 2), "");
        EXPECT_STREQ(authum_sql_stmt_column_cstr(&stmt, 7), "");
        EXPECT_STREQ(name, "O'Brien");
        EXPECT_FALSE(authum_sql_stmt_step_row(&stmt));
    }
    else
    {
        ADD_FAILURE() << "Inserted row was not returned";
    }
    authum_sql_stmt_finalize(&stmt);
    authum_sql_stmt_finalize(&stmt);
}

TEST_F(authum_sql, reports_query_and_prepare_errors)
{
    EXPECT_FALSE(authum_sql_exec(&db, "SELECT * FROM nowhere"));
    EXPECT_NE(strstr(authum_sql_errmsg(&db), "no such table"), authum_null);
    authum_sql_stmt_t stmt = {};
    EXPECT_FALSE(authum_sql_prepare(&db, &stmt, "SELEKT"));
    EXPECT_NE(authum_sql_errmsg(&db)[0], 0);
    authum_sql_stmt_finalize(&stmt);
}

TEST(authum_sql_handle, initialized_handle_is_closed_and_refuses_queries)
{
    authum_sql_t sql;
    authum_sql_init(&sql);
    EXPECT_FALSE(authum_sql_is_open(&sql));
    EXPECT_FALSE(authum_sql_exec(&sql, "SELECT 1"));
    authum_sql_close(&sql);
}

TEST_F(authum_sql, close_is_idempotent)
{
    authum_sql_close(&db);
    EXPECT_FALSE(authum_sql_is_open(&db));
    authum_sql_close(&db);
}

TEST(authum_sql_handle, failed_mysql_open_reports_error_without_retaining_handle)
{
    authum_sql_t sql;
    authum_sql_init(&sql);
    EXPECT_FALSE(authum_sql_open_mysql(&sql, "127.0.0.1", "u", "p", "d", 1u));
    EXPECT_FALSE(authum_sql_is_open(&sql));
    EXPECT_NE(authum_sql_errmsg(&sql)[0], 0);
    authum_sql_close(&sql);
}
} // namespace tests
