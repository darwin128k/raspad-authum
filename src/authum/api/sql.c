#include <authum/sql.h>

#include <lh/sql/mysql.h>
#include <lh/sql/sqlite.h>

#include <stdlib.h>
#include <string.h>

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
	return (lh_sql_db_t *)self->db;
}

static struct authum_sql_stmt_impl *
authum_sql_stmt_get_impl(const authum_sql_stmt_t *stmt)
{
	return (struct authum_sql_stmt_impl *)stmt->stmt;
}

static void
authum_sql_set_errmsg(authum_sql_t *self, lh_str_view_t text)
{
	authum_usize_t n = (authum_usize_t)lh_str_view_get_size(&text);

	if (n > authum_sql_errmsg_max - 1u)
	{
		n = authum_sql_errmsg_max - 1u;
	}
	if (n > 0u)
	{
		memcpy(self->errmsg, lh_str_view_get_data(&text), n);
	}
	self->errmsg[n] = '\0';
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

	lh_str_view_init(&view, authum_null_eq(text) ? "" : text);
	return view;
}

/* Forget the column copies of the row a step is about to leave. */
static void
authum_sql_stmt_drop_columns(struct authum_sql_stmt_impl *impl)
{
	authum_usize_t i;

	for (i = 0u; i < impl->column_count; ++i)
	{
		lh_str_deinit(&impl->columns[i]);
	}
	free(impl->columns);
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
	self->errmsg[0] = '\0';
}

/* A closed lh connection for @p self, ready for a backend to open. */
static lh_sql_db_t *
authum_sql_begin_open(authum_sql_t *self, authum_sql_driver_t driver)
{
	lh_sql_db_t *db;

	authum_sql_close(self);
	authum_sql_init(self);
	self->driver = driver;
	db = (lh_sql_db_t *)calloc(1u, sizeof *db);
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
		free(self->db);
		self->db = authum_null;
		return authum_bool_false;
	}
	return authum_bool_true;
}

authum_bool_t
authum_sql_open_sqlite(authum_sql_t *self, const authum_char_t *path)
{
	lh_sql_db_t *db;

	if (authum_null_eq(self) || authum_null_eq(path) || path[0] == '\0')
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
					   (lh_uint_t)port));
}

authum_bool_t
authum_sql_mysql_available(void)
{
	return authum_null_eq(lh_sql_mysql_api_get_default()) ? authum_bool_false
							      : authum_bool_true;
}

void
authum_sql_close(authum_sql_t *self)
{
	if (authum_null_eq(self) || authum_null_eq(self->db))
	{
		return;
	}
	lh_sql_db_deinit(authum_sql_get_db(self));
	free(self->db);
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
		return authum_bool_false;
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
	return (authum_i64_t)lh_sql_db_get_last_insert_id(authum_sql_get_db(self));
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

	impl = (struct authum_sql_stmt_impl *)calloc(1u, sizeof *impl);
	if (authum_null_eq(impl))
	{
		return authum_bool_false;
	}
	ok = lh_sql_db_prepare(authum_sql_get_db(self), &impl->stmt, authum_sql_view(sql));
	authum_sql_sync_errmsg(self);
	if (!ok)
	{
		lh_sql_stmt_finalize(&impl->stmt);
		free(impl);
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
	lh_sql_stmt_finalize(&impl->stmt);
	free(impl);
	stmt->stmt = authum_null;
	stmt->sql = authum_null;
}

/* The lh statement behind @p stmt for a parameter @p index, or null. */
static lh_sql_stmt_t *
authum_sql_stmt_for_bind(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || index < 1)
	{
		return authum_null;
	}
	return &authum_sql_stmt_get_impl(stmt)->stmt;
}

authum_bool_t
authum_sql_stmt_bind_i32(authum_sql_stmt_t *stmt, authum_i32_t index, authum_i32_t value)
{
	return authum_sql_stmt_bind_i64(stmt, index, (authum_i64_t)value);
}

authum_bool_t
authum_sql_stmt_bind_i64(authum_sql_stmt_t *stmt, authum_i32_t index, authum_i64_t value)
{
	lh_sql_stmt_t *native = authum_sql_stmt_for_bind(stmt, index);

	return (!authum_null_eq(native) &&
		lh_sql_stmt_bind_sint(native, (lh_usize_t)index, (lh_sllong_t)value))
		       ? authum_bool_true
		       : authum_bool_false;
}

authum_bool_t
authum_sql_stmt_bind_cstr(authum_sql_stmt_t *stmt, authum_i32_t index, const authum_char_t *text)
{
	lh_sql_stmt_t *native = authum_sql_stmt_for_bind(stmt, index);

	return (!authum_null_eq(native) &&
		lh_sql_stmt_bind_str(native, (lh_usize_t)index, authum_sql_view(text)))
		       ? authum_bool_true
		       : authum_bool_false;
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
	step = lh_sql_stmt_step(&impl->stmt);
	authum_sql_sync_errmsg(stmt->sql);
	return step;
}

authum_bool_t
authum_sql_stmt_step_row(authum_sql_stmt_t *stmt)
{
	return authum_sql_stmt_step(stmt) == lh_sql_step_row ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_sql_stmt_step_done(authum_sql_stmt_t *stmt)
{
	return authum_sql_stmt_step(stmt) == lh_sql_step_done ? authum_bool_true : authum_bool_false;
}

authum_i32_t
authum_sql_stmt_column_i32(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	return (authum_i32_t)authum_sql_stmt_column_i64(stmt, index);
}

authum_i64_t
authum_sql_stmt_column_i64(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || index < 0)
	{
		return 0;
	}
	return (authum_i64_t)lh_sql_stmt_get_column_sint(&authum_sql_stmt_get_impl(stmt)->stmt,
							 (lh_usize_t)index);
}

const authum_char_t *
authum_sql_stmt_column_cstr(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	struct authum_sql_stmt_impl *impl;
	lh_str_view_t text;
	authum_usize_t at;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || index < 0)
	{
		return "";
	}
	impl = authum_sql_stmt_get_impl(stmt);
	text = lh_sql_stmt_get_column_str(&impl->stmt, (lh_usize_t)index);
	if (lh_str_view_is_empty(&text))
	{
		return "";
	}

	/* A copy that ends in a NUL, one per column, kept until the next step. */
	at = (authum_usize_t)index;
	if (at >= impl->column_count)
	{
		lh_str_t *grown =
			(lh_str_t *)realloc(impl->columns, (at + 1u) * sizeof *impl->columns);
		authum_usize_t i;

		if (authum_null_eq(grown))
		{
			return "";
		}
		for (i = impl->column_count; i <= at; ++i)
		{
			lh_str_init(&grown[i]);
		}
		impl->columns = grown;
		impl->column_count = at + 1u;
	}
	lh_str_assign_view(&impl->columns[at], text);
	return lh_str_get_data(&impl->columns[at]);
}
