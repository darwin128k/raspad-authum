#include "sql_internal.h"

#include <authum/cstr.h>

#include <sqlite3.h>
#include <string.h>

void
authum_sql_set_errmsg(authum_sql_t *self, const authum_char_t *text)
{
	if (authum_null_eq(self))
	{
		return;
	}
	if (authum_null_eq(text))
	{
		self->errmsg[0] = '\0';
	}
	else
	{
		authum_cstr_copy(self->errmsg, sizeof self->errmsg, text);
	}
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

authum_bool_t
authum_sql_open_sqlite(authum_sql_t *self, const authum_char_t *path)
{
	sqlite3 *db = authum_null;
	int rc;

	if (authum_null_eq(self) || authum_null_eq(path) || path[0] == '\0')
	{
		return authum_bool_false;
	}

	authum_sql_close(self);
	authum_sql_init(self);
	self->driver = authum_sql_driver_sqlite;

	rc = sqlite3_open(path, &db);
	if (rc != SQLITE_OK)
	{
		authum_sql_set_errmsg(self, db ? sqlite3_errmsg(db) : "sqlite3_open failed");
		if (!authum_null_eq(db))
		{
			sqlite3_close(db);
		}
		return authum_bool_false;
	}
	self->db = db;
	self->errmsg[0] = '\0';
	return authum_bool_true;
}

void
authum_sql_close(authum_sql_t *self)
{
	if (authum_null_eq(self) || authum_null_eq(self->db))
	{
		return;
	}
	if (self->driver == authum_sql_driver_sqlite)
	{
		sqlite3_close((sqlite3 *)self->db);
		self->db = authum_null;
	}
	else if (self->driver == authum_sql_driver_mysql)
	{
		authum_sql_mysql_close(self);
	}
	else
	{
		self->db = authum_null;
	}
}

authum_bool_t
authum_sql_is_open(const authum_sql_t *self)
{
	return (!authum_null_eq(self) && !authum_null_eq(self->db)) ? authum_bool_true
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
	authum_char_t *err = authum_null;
	int rc;

	if (!authum_sql_is_open(self) || authum_null_eq(sql))
	{
		return authum_bool_false;
	}
	if (self->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_exec(self, sql);
	}
	if (self->driver != authum_sql_driver_sqlite)
	{
		return authum_bool_false;
	}

	rc = sqlite3_exec((sqlite3 *)self->db, sql, authum_null, authum_null, &err);
	if (rc != SQLITE_OK)
	{
		authum_sql_set_errmsg(self, err ? err : "exec failed");
		sqlite3_free(err);
		return authum_bool_false;
	}
	self->errmsg[0] = '\0';
	return authum_bool_true;
}

authum_i64_t
authum_sql_last_insert_id(authum_sql_t *self)
{
	if (!authum_sql_is_open(self))
	{
		return 0;
	}
	if (self->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_last_insert_id(self);
	}
	if (self->driver != authum_sql_driver_sqlite)
	{
		return 0;
	}
	return (authum_i64_t)sqlite3_last_insert_rowid((sqlite3 *)self->db);
}

authum_bool_t
authum_sql_prepare(authum_sql_t *self, authum_sql_stmt_t *stmt, const authum_char_t *sql)
{
	sqlite3_stmt *raw = authum_null;

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
	if (self->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_prepare(self, stmt, sql);
	}
	if (self->driver != authum_sql_driver_sqlite)
	{
		return authum_bool_false;
	}

	if (sqlite3_prepare_v2((sqlite3 *)self->db, sql, -1, &raw, authum_null) != SQLITE_OK)
	{
		authum_sql_set_errmsg(self, sqlite3_errmsg((sqlite3 *)self->db));
		return authum_bool_false;
	}
	stmt->stmt = raw;
	return authum_bool_true;
}

void
authum_sql_stmt_finalize(authum_sql_stmt_t *stmt)
{
	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt))
	{
		return;
	}
	if (!authum_null_eq(stmt->sql) && stmt->sql->driver == authum_sql_driver_mysql)
	{
		authum_sql_mysql_stmt_finalize(stmt);
		return;
	}
	if (!authum_null_eq(stmt->sql) && stmt->sql->driver == authum_sql_driver_sqlite)
	{
		sqlite3_finalize((sqlite3_stmt *)stmt->stmt);
	}
	stmt->stmt = authum_null;
	stmt->sql = authum_null;
}

authum_bool_t
authum_sql_stmt_bind_i32(authum_sql_stmt_t *stmt, authum_i32_t index, authum_i32_t value)
{
	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql))
	{
		return authum_bool_false;
	}
	if (stmt->sql->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_stmt_bind_i32(stmt, index, value);
	}
	if (stmt->sql->driver != authum_sql_driver_sqlite)
	{
		return authum_bool_false;
	}
	return sqlite3_bind_int((sqlite3_stmt *)stmt->stmt, (int)index, (int)value) == SQLITE_OK
		   ? authum_bool_true
		   : authum_bool_false;
}

authum_bool_t
authum_sql_stmt_bind_cstr(authum_sql_stmt_t *stmt, authum_i32_t index, const authum_char_t *text)
{
	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql))
	{
		return authum_bool_false;
	}
	if (stmt->sql->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_stmt_bind_cstr(stmt, index, text);
	}
	if (stmt->sql->driver != authum_sql_driver_sqlite)
	{
		return authum_bool_false;
	}
	if (authum_null_eq(text))
	{
		text = "";
	}
	return sqlite3_bind_text((sqlite3_stmt *)stmt->stmt, (int)index, text, -1, SQLITE_TRANSIENT) ==
			   SQLITE_OK
		   ? authum_bool_true
		   : authum_bool_false;
}

authum_bool_t
authum_sql_stmt_step_row(authum_sql_stmt_t *stmt)
{
	int rc;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql))
	{
		return authum_bool_false;
	}
	if (stmt->sql->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_stmt_step_row(stmt);
	}
	if (stmt->sql->driver != authum_sql_driver_sqlite)
	{
		return authum_bool_false;
	}
	rc = sqlite3_step((sqlite3_stmt *)stmt->stmt);
	return rc == SQLITE_ROW ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_sql_stmt_step_done(authum_sql_stmt_t *stmt)
{
	int rc;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql))
	{
		return authum_bool_false;
	}
	if (stmt->sql->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_stmt_step_done(stmt);
	}
	if (stmt->sql->driver != authum_sql_driver_sqlite)
	{
		return authum_bool_false;
	}
	rc = sqlite3_step((sqlite3_stmt *)stmt->stmt);
	if (rc != SQLITE_DONE && !authum_null_eq(stmt->sql))
	{
		authum_sql_set_errmsg(stmt->sql, sqlite3_errmsg((sqlite3 *)stmt->sql->db));
	}
	return rc == SQLITE_DONE ? authum_bool_true : authum_bool_false;
}

authum_i32_t
authum_sql_stmt_column_i32(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql))
	{
		return 0;
	}
	if (stmt->sql->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_stmt_column_i32(stmt, index);
	}
	return (authum_i32_t)sqlite3_column_int((sqlite3_stmt *)stmt->stmt, (int)index);
}

const authum_char_t *
authum_sql_stmt_column_cstr(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	const authum_char_t *p;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql))
	{
		return "";
	}
	if (stmt->sql->driver == authum_sql_driver_mysql)
	{
		return authum_sql_mysql_stmt_column_cstr(stmt, index);
	}
	p = (const authum_char_t *)sqlite3_column_text((sqlite3_stmt *)stmt->stmt, (int)index);
	return authum_null_eq(p) ? "" : p;
}
