# SQL (`<authum/sql.h>`)

Authum ships a **SQL brick** for modules: open a DB, exec DDL, prepare/bind/step.
Schema and queries stay in the module (e.g. gatekeeper’s IP / client tables).

SQLite amalgamation lives under `lib/sqlite` and is linked through
`authum_client` → `authum_sqlite`. Modules never `#include <sqlite3.h>` /
`<mysql.h>` and never vendor their own SQL client.

## Drivers

| `authum_sql_driver_*` | How |
|-----------------------|-----|
| `sqlite` | `authum_sql_open_sqlite(path)` — always available |
| `mysql` | `authum_sql_open_mysql(host, user, pass, db, port)` — **dlopen** of `libmariadb` / `libmysqlclient` on the host (`port` 0 → 3306) |

Check `authum_sql_mysql_available()` if you want a soft-fallback to SQLite when
the client library is missing.

Prepared statements use `?` placeholders on both drivers. MySQL expands them
with `mysql_real_escape_string` (no dependency on MariaDB headers at build time).

DDL dialect differs (`AUTOINCREMENT` vs `AUTO_INCREMENT`, `TEXT` vs `VARCHAR`)
— the **module** chooses statements per `self->driver`.

## Sketch

```c
authum_sql_t db;
authum_sql_stmt_t stmt;

authum_sql_init(&db);
authum_sql_open_sqlite(&db, authum_path_as_cstr(&path));
/* or: authum_sql_open_mysql(&db, "127.0.0.1", "user", "pass", "gatekeeper", 0); */
authum_sql_exec(&db, "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, ip TEXT)");

authum_sql_prepare(&db, &stmt, "SELECT id FROM t WHERE ip=?");
authum_sql_stmt_bind_cstr(&stmt, 1, "1.2.3.4");
if (authum_sql_stmt_step_row(&stmt)) {
    authum_i32_t id = authum_sql_stmt_column_i32(&stmt, 0);
    (void)id;
}
authum_sql_stmt_finalize(&stmt);
authum_sql_close(&db);
```
