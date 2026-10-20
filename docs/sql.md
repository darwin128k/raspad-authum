# SQL (`<authum/sql.h>`)

Authum ships a **SQL brick** for modules: open a DB, exec DDL, prepare/bind/step.
Schema and queries stay in the module (e.g. gatekeeper’s IP / client tables).

SQLite amalgamation lives under `third_party/sqlite` and is linked through
`authum_client` → `authum_sqlite`. Modules never `#include <sqlite3.h>` and never
vendor their own copy.

## Drivers

| `authum_sql_driver_*` | Status |
|-----------------------|--------|
| `sqlite` | Open with `authum_sql_open_sqlite(path)` |
| `mysql` | Enum reserved — not linked yet |

## Sketch

```c
authum_sql_t db;
authum_sql_stmt_t stmt;

authum_sql_init(&db);
authum_sql_open_sqlite(&db, authum_path_as_cstr(&path));
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
