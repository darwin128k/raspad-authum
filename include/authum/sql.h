/**
 * @file sql.h
 * @brief Module SQL brick — SQLite (linked) + MySQL/MariaDB (dlopen).
 *
 * Modules never `#include <sqlite3.h>` / `<mysql.h>`. Schema stays in the module.
 * Runs on lh/sql (SQLite 3.46.1 inside lh; the host's MySQL client loaded at run
 * time). `?` parameters count from 1, columns from 0. A text column stays valid
 * until the next step or finalize. On MySQL, ::authum_sql_exec runs one statement.
 */

#ifndef AUTHUM_SQL_H
#define AUTHUM_SQL_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/sql/driver.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

enum
{
	authum_sql_errmsg_max = 256
};

struct authum_sql
{
	authum_sql_driver_t driver;
	/** The connection (opaque; owned by Authum while open). */
	authum_ptr db;
	authum_char_t errmsg[authum_sql_errmsg_max];
};
typedef struct authum_sql authum_sql_t;

struct authum_sql_stmt
{
	authum_sql_t *sql;
	/** The prepared statement (opaque; owned by Authum until finalize). */
	authum_ptr stmt;
};
typedef struct authum_sql_stmt authum_sql_stmt_t;

void authum_sql_init(authum_sql_t *self);

authum_bool_t authum_sql_open_sqlite(authum_sql_t *self, const authum_char_t *path);

/**
 * @brief Open MySQL/MariaDB via dlopen of the client library on the host.
 * @param port 0 → 3306.
 */
authum_bool_t authum_sql_open_mysql(authum_sql_t *self, const authum_char_t *host,
				    const authum_char_t *user, const authum_char_t *pass,
				    const authum_char_t *db, authum_u32_t port);

/** @return true when a MySQL client .so/.dll was loaded at least once this process. */
authum_bool_t authum_sql_mysql_available(void);

void authum_sql_close(authum_sql_t *self);

authum_bool_t authum_sql_is_open(const authum_sql_t *self);

/** Active round-trip (SELECT 1), unlike is_open which only checks the handle.
 * May block on synchronous connections; not a guarantee about future queries.
 */
authum_bool_t authum_sql_ping(authum_sql_t *self);

const authum_char_t *authum_sql_errmsg(const authum_sql_t *self);

/** First failure on this open connection since connect/clear, retained across success. */
const authum_char_t *authum_sql_first_error(const authum_sql_t *self);

/** Explicitly acknowledge handled failures before continuing a SQL batch. */
void authum_sql_clear_errors(authum_sql_t *self);

authum_bool_t authum_sql_exec(authum_sql_t *self, const authum_char_t *sql);

authum_i64_t authum_sql_last_insert_id(authum_sql_t *self);

authum_bool_t authum_sql_prepare(authum_sql_t *self, authum_sql_stmt_t *stmt,
				 const authum_char_t *sql);

void authum_sql_stmt_finalize(authum_sql_stmt_t *stmt);

authum_bool_t authum_sql_stmt_bind_i32(authum_sql_stmt_t *stmt, authum_i32_t index,
				       authum_i32_t value);

authum_bool_t authum_sql_stmt_bind_cstr(authum_sql_stmt_t *stmt, authum_i32_t index,
					const authum_char_t *text);

/** @brief Bind a 64-bit integer (a SteamID64, a row id) to parameter @p index. */
authum_bool_t authum_sql_stmt_bind_i64(authum_sql_stmt_t *stmt, authum_i32_t index,
				       authum_i64_t value);

authum_bool_t authum_sql_stmt_step_row(authum_sql_stmt_t *stmt);

authum_bool_t authum_sql_stmt_step_done(authum_sql_stmt_t *stmt);

authum_i32_t authum_sql_stmt_column_i32(authum_sql_stmt_t *stmt, authum_i32_t index);

/** @brief Column @p index of the current row as a 64-bit integer; `0` for NULL. */
authum_i64_t authum_sql_stmt_column_i64(authum_sql_stmt_t *stmt, authum_i32_t index);

const authum_char_t *authum_sql_stmt_column_cstr(authum_sql_stmt_t *stmt, authum_i32_t index);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_SQL_H */
