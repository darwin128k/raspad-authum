/**
 * @file driver.h
 * @brief Which backend ::authum_sql_t talks to.
 */

#ifndef AUTHUM_SQL_DRIVER_H
#define AUTHUM_SQL_DRIVER_H

#include <authum/extern.h>

enum authum_sql_driver
{
    /** Local file via vendored SQLite (default). */
    authum_sql_driver_sqlite = 0,
    /** MySQL/MariaDB via dlopen of the host’s client library. */
    authum_sql_driver_mysql = 1
};
typedef enum authum_sql_driver authum_sql_driver_t;

#endif /* AUTHUM_SQL_DRIVER_H */
