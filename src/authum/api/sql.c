#include <authum/sql.h>
#include <lh/char/map.h>
#include <authum/memory.h>
#include <lh/memory/std.h>

#include <lh/sql/mysql.h>
#include <lh/sql/sqlite.h>

#include <stdlib.h>
#include <string.h>

/* Private storage: keep the public SQL handle layout unchanged. */
struct authum_sql_db_impl
{
    lh_sql_db_t db;
    authum_char_t first_error[authum_sql_errmsg_max];
};

const authum_char_t *
authum_sql_first_error(const authum_sql_t *self)
{
    if (!self)
    {
        return "";
    }
    return self->db
               ? (lh_cast_reinterpret(const struct authum_sql_db_impl *, self->db))->first_error
               : self->errmsg;
}

void
authum_sql_clear_errors(authum_sql_t *self)
{
    if (!self)
    {
        return;
    }
    self->errmsg[0] = lh_char_map_nul;
    if (self->db)
    {
        struct authum_sql_db_impl *impl = self->db;
        impl->first_error[0] = lh_char_map_nul;
        lh_sql_db_clear_error(lh_addr_of(impl->db));
    }
}

/* What authum_sql_stmt_t::stmt points at: the lh statement, and a NUL-terminated
   copy of each text column read from the current row (lh hands out views). */
struct authum_sql_stmt_impl
{
    lh_sql_stmt_t stmt;
    lh_str_t *columns;
    authum_usize_t column_count;
};

static lh_sql_db_t *
authum_sql_get_db(const authum_sql_t *self)
{
    return lh_cast_reinterpret(lh_sql_db_t *, self->db);
}

static struct authum_sql_stmt_impl *
authum_sql_stmt_get_impl(const authum_sql_stmt_t *stmt)
{
    return lh_cast_reinterpret(struct authum_sql_stmt_impl *, stmt->stmt);
}

static void
authum_sql_set_errmsg(authum_sql_t *self, lh_str_view_t text)
{
    authum_usize_t n = lh_cast_static(authum_usize_t, lh_str_view_get_size(lh_addr_of(text)));

    if (lh_math_gt(n, lh_math_sub(authum_sql_errmsg_max, 1u)))
    {
        n = lh_math_sub(authum_sql_errmsg_max, 1u);
    }
    if (lh_math_gt(n, 0u))
    {
        lh_memory_std_copy(self->errmsg, lh_str_view_get_data(lh_addr_of(text)), n);
    }
    self->errmsg[n] = lh_char_map_nul;
    if (n && self->db)
    {
        struct authum_sql_db_impl *impl = self->db;
        if (!impl->first_error[0])
        {
            lh_memory_std_copy(impl->first_error, self->errmsg, lh_math_add(n, 1));
        }
    }
}

/* The connection's last message (empty after a success) into errmsg. */
static void
authum_sql_sync_errmsg(authum_sql_t *self)
{
    authum_sql_set_errmsg(self, lh_sql_db_get_error(authum_sql_get_db(self)));
}

static lh_str_view_t
authum_sql_view(const authum_char_t *text)
{
    lh_str_view_t view;

    lh_str_view_init(lh_addr_of(view), authum_null_eq(text) ? "" : text);
    return view;
}

static authum_bool_t
authum_sql_fail(authum_sql_t *self, const authum_char_t *message)
{
    if (self)
    {
        authum_sql_set_errmsg(self, authum_sql_view(message));
    }
    return authum_bool_false;
}

/* Forget the column copies of the row a step is about to leave. */
static void
authum_sql_stmt_drop_columns(struct authum_sql_stmt_impl *impl)
{
    authum_usize_t i;

    for (i = 0u; lh_math_lt(i, impl->column_count); i = lh_math_add_one(i))
    {
        lh_str_deinit(lh_addr_of(impl->columns[i]));
    }
    authum_memory_free(impl->columns);
    impl->columns = authum_null;
    impl->column_count = 0u;
}

void
authum_sql_init(authum_sql_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    self->driver = authum_sql_driver_sqlite;
    self->db = authum_null;
    self->errmsg[0] = lh_char_map_nul;
}

/* A closed lh connection for @p self, ready for a backend to open. */
static lh_sql_db_t *
authum_sql_begin_open(authum_sql_t *self, authum_sql_driver_t driver)
{
    lh_sql_db_t *db;

    authum_sql_close(self);
    authum_sql_init(self);
    self->driver = driver;
    db = lh_cast_reinterpret(lh_sql_db_t *,
                             authum_memory_alloc_zero(1u, sizeof(struct authum_sql_db_impl)));
    if (authum_null_eq(db))
    {
        authum_sql_set_errmsg(self, lh_str_view_lit("out of memory"));
        return authum_null;
    }
    lh_sql_db_init(db);
    self->db = db;
    return db;
}

/* Keep the connection if @p opened, drop it otherwise; the message either way. */
static authum_bool_t
authum_sql_end_open(authum_sql_t *self, lh_bool_t opened)
{
    authum_sql_sync_errmsg(self);
    if (!opened)
    {
        lh_sql_db_deinit(authum_sql_get_db(self));
        authum_memory_free(self->db);
        self->db = authum_null;
        return authum_bool_false;
    }
    return authum_bool_true;
}

authum_bool_t
authum_sql_open_sqlite(authum_sql_t *self, const authum_char_t *path)
{
    lh_sql_db_t *db;

    if (authum_null_eq(self) || authum_null_eq(path) || lh_math_eq(path[0], lh_char_map_nul))
    {
        return authum_bool_false;
    }
    db = authum_sql_begin_open(self, authum_sql_driver_sqlite);
    if (authum_null_eq(db))
    {
        return authum_bool_false;
    }
    return authum_sql_end_open(self, lh_sql_db_open_sqlite(db, authum_sql_view(path)));
}

authum_bool_t
authum_sql_open_mysql(authum_sql_t *self, const authum_char_t *host, const authum_char_t *user,
                      const authum_char_t *pass, const authum_char_t *db_name, authum_u32_t port)
{
    lh_sql_db_t *db;

    if (authum_null_eq(self) || authum_null_eq(host) || authum_null_eq(user) ||
        authum_null_eq(db_name))
    {
        return authum_bool_false;
    }
    db = authum_sql_begin_open(self, authum_sql_driver_mysql);
    if (authum_null_eq(db))
    {
        return authum_bool_false;
    }
    return authum_sql_end_open(
        self, lh_sql_db_open_mysql(db, authum_sql_view(host), authum_sql_view(user),
                                   authum_sql_view(pass), authum_sql_view(db_name),
                                   lh_cast_static(lh_uint_t, port)));
}

authum_bool_t
authum_sql_mysql_available(void)
{
    return authum_null_eq(lh_sql_mysql_api_get_default()) ? authum_bool_false : authum_bool_true;
}

void
authum_sql_close(authum_sql_t *self)
{
    if (authum_null_eq(self) || authum_null_eq(self->db))
    {
        return;
    }
    lh_sql_db_deinit(authum_sql_get_db(self));
    authum_memory_free(self->db);
    self->db = authum_null;
}

authum_bool_t
authum_sql_ping(authum_sql_t *self)
{
    return authum_sql_exec(self, "SELECT 1");
}

authum_bool_t
authum_sql_is_open(const authum_sql_t *self)
{
    return (!authum_null_eq(self) && !authum_null_eq(self->db) &&
            lh_sql_db_is_open(authum_sql_get_db(self)))
               ? authum_bool_true
               : authum_bool_false;
}

const authum_char_t *
authum_sql_errmsg(const authum_sql_t *self)
{
    if (authum_null_eq(self))
    {
        return "";
    }
    return self->errmsg;
}

authum_bool_t
authum_sql_exec(authum_sql_t *self, const authum_char_t *sql)
{
    lh_bool_t ok;

    if (!authum_sql_is_open(self) || authum_null_eq(sql))
    {
        return authum_sql_fail(self, "database is not open or SQL text is missing");
    }
    ok = lh_sql_db_exec(authum_sql_get_db(self), authum_sql_view(sql));
    authum_sql_sync_errmsg(self);
    return ok ? authum_bool_true : authum_bool_false;
}

authum_i64_t
authum_sql_last_insert_id(authum_sql_t *self)
{
    if (!authum_sql_is_open(self))
    {
        return 0;
    }
    return lh_cast_static(authum_i64_t, lh_sql_db_get_last_insert_id(authum_sql_get_db(self)));
}

authum_bool_t
authum_sql_prepare(authum_sql_t *self, authum_sql_stmt_t *stmt, const authum_char_t *sql)
{
    struct authum_sql_stmt_impl *impl;
    lh_bool_t ok;

    if (authum_null_eq(stmt))
    {
        return authum_bool_false;
    }
    stmt->sql = self;
    stmt->stmt = authum_null;
    if (!authum_sql_is_open(self) || authum_null_eq(sql))
    {
        return authum_bool_false;
    }

    impl = lh_cast_reinterpret(struct authum_sql_stmt_impl *,
                               authum_memory_alloc_zero(1u, sizeof lh_ptr_deref(impl)));
    if (authum_null_eq(impl))
    {
        return authum_bool_false;
    }
    ok = lh_sql_db_prepare(authum_sql_get_db(self), lh_addr_of(impl->stmt), authum_sql_view(sql));
    authum_sql_sync_errmsg(self);
    if (!ok)
    {
        lh_sql_stmt_finalize(lh_addr_of(impl->stmt));
        authum_memory_free(impl);
        return authum_bool_false;
    }
    stmt->stmt = impl;
    return authum_bool_true;
}

void
authum_sql_stmt_finalize(authum_sql_stmt_t *stmt)
{
    struct authum_sql_stmt_impl *impl;

    if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt))
    {
        return;
    }
    impl = authum_sql_stmt_get_impl(stmt);
    authum_sql_stmt_drop_columns(impl);
    lh_sql_stmt_finalize(lh_addr_of(impl->stmt));
    authum_memory_free(impl);
    stmt->stmt = authum_null;
    stmt->sql = authum_null;
}

/* The lh statement behind @p stmt for a parameter @p index, or null. */
static lh_sql_stmt_t *
authum_sql_stmt_for_bind(authum_sql_stmt_t *stmt, authum_i32_t index)
{
    if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || lh_math_lt(index, 1))
    {
        return authum_null;
    }
    return lh_addr_of(authum_sql_stmt_get_impl(stmt)->stmt);
}

authum_bool_t
authum_sql_stmt_bind_i32(authum_sql_stmt_t *stmt, authum_i32_t index, authum_i32_t value)
{
    return authum_sql_stmt_bind_i64(stmt, index, lh_cast_static(authum_i64_t, value));
}

authum_bool_t
authum_sql_stmt_bind_i64(authum_sql_stmt_t *stmt, authum_i32_t index, authum_i64_t value)
{
    lh_sql_stmt_t *native = authum_sql_stmt_for_bind(stmt, index);
    authum_bool_t ok = native && lh_sql_stmt_bind_sint(native, lh_cast_static(lh_usize_t, index),
                                                       lh_cast_static(lh_sllong_t, value));
    if (!ok && stmt && stmt->sql)
    {
        if (stmt->sql->db)
        {
            authum_sql_sync_errmsg(stmt->sql);
        }
        if (!authum_sql_errmsg(stmt->sql)[0])
        {
            authum_sql_fail(stmt->sql, "SQL parameter bind failed");
        }
    }
    return ok;
}

authum_bool_t
authum_sql_stmt_bind_cstr(authum_sql_stmt_t *stmt, authum_i32_t index, const authum_char_t *text)
{
    lh_sql_stmt_t *native = authum_sql_stmt_for_bind(stmt, index);
    authum_bool_t ok = native && lh_sql_stmt_bind_str(native, lh_cast_static(lh_usize_t, index),
                                                      authum_sql_view(text));
    if (!ok && stmt && stmt->sql)
    {
        if (stmt->sql->db)
        {
            authum_sql_sync_errmsg(stmt->sql);
        }
        if (!authum_sql_errmsg(stmt->sql)[0])
        {
            authum_sql_fail(stmt->sql, "SQL parameter bind failed");
        }
    }
    return ok;
}

/* One step, column copies of the old row dropped, errmsg kept current. */
static lh_sql_step_t
authum_sql_stmt_step(authum_sql_stmt_t *stmt)
{
    struct authum_sql_stmt_impl *impl;
    lh_sql_step_t step;

    if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql))
    {
        return lh_sql_step_error;
    }
    impl = authum_sql_stmt_get_impl(stmt);
    authum_sql_stmt_drop_columns(impl);
    step = lh_sql_stmt_step(lh_addr_of(impl->stmt));
    authum_sql_sync_errmsg(stmt->sql);
    return step;
}

authum_bool_t
authum_sql_stmt_step_row(authum_sql_stmt_t *stmt)
{
    return lh_math_eq(authum_sql_stmt_step(stmt), lh_sql_step_row) ? authum_bool_true
                                                                   : authum_bool_false;
}

authum_bool_t
authum_sql_stmt_step_done(authum_sql_stmt_t *stmt)
{
    return lh_math_eq(authum_sql_stmt_step(stmt), lh_sql_step_done) ? authum_bool_true
                                                                    : authum_bool_false;
}

authum_i32_t
authum_sql_stmt_column_i32(authum_sql_stmt_t *stmt, authum_i32_t index)
{
    return lh_cast_static(authum_i32_t, authum_sql_stmt_column_i64(stmt, index));
}

authum_i64_t
authum_sql_stmt_column_i64(authum_sql_stmt_t *stmt, authum_i32_t index)
{
    if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || lh_math_lt(index, 0))
    {
        return 0;
    }
    return lh_cast_static(
        authum_i64_t, lh_sql_stmt_get_column_sint(lh_addr_of(authum_sql_stmt_get_impl(stmt)->stmt),
                                                  lh_cast_static(lh_usize_t, index)));
}

const authum_char_t *
authum_sql_stmt_column_cstr(authum_sql_stmt_t *stmt, authum_i32_t index)
{
    struct authum_sql_stmt_impl *impl;
    lh_str_view_t text;
    authum_usize_t at;

    if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || lh_math_lt(index, 0))
    {
        return "";
    }
    impl = authum_sql_stmt_get_impl(stmt);
    text = lh_sql_stmt_get_column_str(lh_addr_of(impl->stmt), lh_cast_static(lh_usize_t, index));
    if (lh_str_view_is_empty(lh_addr_of(text)))
    {
        return "";
    }

    /* A copy that ends in a NUL, one per column, kept until the next step. */
    at = lh_cast_static(authum_usize_t, index);
    if (lh_math_ge(at, impl->column_count))
    {
        lh_str_t *grown = lh_cast_reinterpret(
            lh_str_t *,
            authum_memory_realloc(
                impl->columns, lh_math_mul(impl->column_count, sizeof lh_ptr_deref(impl->columns)),
                lh_math_mul((lh_math_add(at, 1u)), sizeof lh_ptr_deref(impl->columns))));
        authum_usize_t i;

        if (authum_null_eq(grown))
        {
            return "";
        }
        for (i = impl->column_count; lh_math_le(i, at); i = lh_math_add_one(i))
        {
            lh_str_init(lh_addr_of(grown[i]));
        }
        impl->columns = grown;
        impl->column_count = lh_math_add(at, 1u);
    }
    lh_str_assign_view(lh_addr_of(impl->columns[at]), text);
    return lh_str_get_data(lh_addr_of(impl->columns[at]));
}
