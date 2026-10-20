/**
 * @file driver.h
 * @brief Which backend ::authum_sql_t talks to.
 */

#ifndef AUTHUM_SQL_DRIVER_H
#define AUTHUM_SQL_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

enum authum_sql_driver {
	/** Local file via vendored SQLite (default). */
	authum_sql_driver_sqlite = 0,
	/** Reserved — not linked yet. */
	authum_sql_driver_mysql = 1
};
typedef enum authum_sql_driver authum_sql_driver_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_SQL_DRIVER_H */
