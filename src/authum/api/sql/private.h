/**
 * @file private.h
 * @brief Private to `sql.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_SQL_PRIVATE_H
#define AUTHUM_API_SQL_PRIVATE_H

#include <authum/sql.h>
#include <lh/core/char/map.h>
#include <lh/core/memory/std.h>
#include <lh/os/sql/mysql.h>
#include <lh/os/sql/sqlite.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The lh database @p self wraps, or ::authum_null when it is closed.
 */
lh_sql_db_t *
authum_sql_get_db(const authum_sql_t *self);

/**
 * @brief The private state of @p stmt, or ::authum_null when it is finished.
 */
struct authum_sql_stmt_impl *
authum_sql_stmt_get_impl(const authum_sql_stmt_t *stmt);

/**
 * @brief Copy @p text into @p self's error message, cut to its room.
 */
void
authum_sql_set_errmsg(authum_sql_t *self, lh_str_view_t text);

/**
 * @brief The connection's last message (empty after a success) into errmsg.
 */
void
authum_sql_sync_errmsg(authum_sql_t *self);

/**
 * @brief A view of @p text; an empty one for ::authum_null.
 */
lh_str_view_t
authum_sql_view(const authum_char_t *text);

/**
 * @brief Store @p message as @p self's error and return false, for a failing call to return.
 */
authum_bool_t
authum_sql_fail(authum_sql_t *self, const authum_char_t *message);

/**
 * @brief Forget the column copies of the row a step is about to leave.
 */
void
authum_sql_stmt_drop_columns(struct authum_sql_stmt_impl *impl);

/**
 * @brief A closed lh connection for @p self, ready for a backend to open.
 */
lh_sql_db_t *
authum_sql_begin_open(authum_sql_t *self, authum_sql_driver_t driver);

/**
 * @brief Keep the connection if @p opened, drop it otherwise; the message either way.
 */
authum_bool_t
authum_sql_end_open(authum_sql_t *self, lh_bool_t opened);

/**
 * @brief The lh statement behind @p stmt for a parameter @p index, or null.
 */
lh_sql_stmt_t *
authum_sql_stmt_for_bind(authum_sql_stmt_t *stmt, authum_i32_t index);

/**
 * @brief One step, column copies of the old row dropped, errmsg kept current.
 */
lh_sql_step_t
authum_sql_stmt_step(authum_sql_stmt_t *stmt);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_SQL_PRIVATE_H */
