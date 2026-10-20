/**
 * @file sql.h
 * @brief Module SQL brick — SQLite (linked) + MySQL/MariaDB (dlopen).
 *
 * Modules never `#include <sqlite3.h>` / `<mysql.h>`. Schema stays in the module.
 */

#ifndef AUTHUM_SQL_H
#define AUTHUM_SQL_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/sql/driver.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
	authum_sql_errmsg_max = 256
};

struct authum_sql
{
	authum_sql_driver_t driver;
	/** Backend handle (`sqlite3 *` or `MYSQL *`). */
	void *db;
	authum_char_t errmsg[authum_sql_errmsg_max];
};
typedef struct authum_sql authum_sql_t;

struct authum_sql_stmt
{
	authum_sql_t *sql;
	void *stmt;
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

const authum_char_t *authum_sql_errmsg(const authum_sql_t *self);

authum_bool_t authum_sql_exec(authum_sql_t *self, const authum_char_t *sql);

authum_i64_t authum_sql_last_insert_id(authum_sql_t *self);

authum_bool_t authum_sql_prepare(authum_sql_t *self, authum_sql_stmt_t *stmt,
				 const authum_char_t *sql);

void authum_sql_stmt_finalize(authum_sql_stmt_t *stmt);

authum_bool_t authum_sql_stmt_bind_i32(authum_sql_stmt_t *stmt, authum_i32_t index,
				       authum_i32_t value);

authum_bool_t authum_sql_stmt_bind_cstr(authum_sql_stmt_t *stmt, authum_i32_t index,
					const authum_char_t *text);

authum_bool_t authum_sql_stmt_step_row(authum_sql_stmt_t *stmt);

authum_bool_t authum_sql_stmt_step_done(authum_sql_stmt_t *stmt);

authum_i32_t authum_sql_stmt_column_i32(authum_sql_stmt_t *stmt, authum_i32_t index);

const authum_char_t *authum_sql_stmt_column_cstr(authum_sql_stmt_t *stmt, authum_i32_t index);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_SQL_H */
