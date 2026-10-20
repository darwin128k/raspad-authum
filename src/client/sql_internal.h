/**
 * @file sql_internal.h
 * @brief Shared helpers between sql.c and sql_mysql.c (not public ABI).
 */

#ifndef AUTHUM_SQL_INTERNAL_H
#define AUTHUM_SQL_INTERNAL_H

#include <authum/sql.h>

void authum_sql_set_errmsg(authum_sql_t *self, const authum_char_t *text);

authum_bool_t authum_sql_mysql_exec(authum_sql_t *self, const authum_char_t *sql);
authum_i64_t authum_sql_mysql_last_insert_id(authum_sql_t *self);
void authum_sql_mysql_close(authum_sql_t *self);

authum_bool_t authum_sql_mysql_prepare(authum_sql_t *self, authum_sql_stmt_t *stmt,
				       const authum_char_t *sql);
void authum_sql_mysql_stmt_finalize(authum_sql_stmt_t *stmt);
authum_bool_t authum_sql_mysql_stmt_bind_i32(authum_sql_stmt_t *stmt, authum_i32_t index,
					     authum_i32_t value);
authum_bool_t authum_sql_mysql_stmt_bind_cstr(authum_sql_stmt_t *stmt, authum_i32_t index,
					      const authum_char_t *text);
authum_bool_t authum_sql_mysql_stmt_step_row(authum_sql_stmt_t *stmt);
authum_bool_t authum_sql_mysql_stmt_step_done(authum_sql_stmt_t *stmt);
authum_i32_t authum_sql_mysql_stmt_column_i32(authum_sql_stmt_t *stmt, authum_i32_t index);
const authum_char_t *authum_sql_mysql_stmt_column_cstr(authum_sql_stmt_t *stmt,
						       authum_i32_t index);

#endif /* AUTHUM_SQL_INTERNAL_H */
