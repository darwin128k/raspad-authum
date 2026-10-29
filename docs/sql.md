# SQL: synchronous handles and worker queries

Include `<authum/sql.h>` for the existing synchronous API, or
`<authum/sql/thread.h>` for background queries. Neither exposes lh or native
SQLite/MySQL types.

`authum_sql_init`, `authum_sql_open_sqlite` / `authum_sql_open_mysql` (or the
shared-description helper `authum_sql_connect`), then prepare/bind/step/finalize,
and `authum_sql_close` remain available. A synchronous handle belongs to one
thread; do not share it with a worker. These calls can block their calling
thread. `authum_sql_is_open` only checks handle ownership; `authum_sql_ping`
performs an actual `SELECT 1` round trip and may also block. A successful ping
cannot guarantee that a later query will succeed.

## Threaded queries

`authum_sql_connection_t` describes a connection; it is not a connected handle.
`authum_sql_thread_query(host, connection, work, complete, destroy, user)` copies
the connection strings and queues a SQL task. It opens a fresh private
connection in the worker, calls `work(db, user)`, closes the connection, then
delivers `complete(result, user)` on a server frame. A task can execute a whole
logical batch on that connection (including a transaction).

```c
authum_bool_t mymod_work(authum_sql_t *db, authum_ptr user)
{
    mymod_request_t *request = user;
    authum_sql_stmt_t stmt;
    if (!authum_sql_prepare(db, &stmt, "SELECT score FROM player WHERE id=?"))
        return authum_bool_false;
    authum_sql_stmt_bind_i32(&stmt, 1, request->player_id);
    if (authum_sql_stmt_step_row(&stmt))
        request->score = authum_sql_stmt_column_i32(&stmt, 0);
    authum_sql_stmt_finalize(&stmt);
    return authum_sql_errmsg(db)[0] == '\0';
}

void mymod_complete(const authum_sql_thread_result_t *result, authum_ptr user)
{
    mymod_request_t *request = user;
    /* Game thread: check the request's userid/generation before touching a player. */
    if (!result->success) { /* result->connected distinguishes connect/query errors */ }
    else { /* consume request->score */ }
}
```

Use bound parameters for untrusted values. Copy needed rows into request-owned
storage inside `work`; the database and statement pointers never survive the
worker callback. Worker code must not call engine, event, console, host logging,
commands or sandbox APIs, or mutate game-thread state. Read-only environment/path
getters are safe while the module is loaded. Return data/log messages to the
game thread instead. Check every operation: no rows and a failed query are
different outcomes. A failed transaction must be abandoned, not continued on a
replacement connection.

There is one FIFO worker per module and a limit of 64 outstanding tasks per
module, including results waiting for delivery. Submission returns false when
unsupported, full, or out of memory; the caller then retains its user payload.
On success, `destroy(user)` runs exactly once on the game thread after completion
or cancellation. Up to 16 completions total are dispatched per frame, rotating
between owners. Keep completion callbacks short.

Before module stop/dlclose, Authum cancels queued work/results, waits for the
active worker, and calls destructors without calling completion callbacks.
Unload may therefore wait for active I/O; it is not a cancellation of an
in-flight database transaction. Unloading a module from a task callback is
rejected. MySQL connect/read/write timeouts are set to five seconds (the client
may internally retry I/O); DNS timing still depends on the OS. Custom worker
code must also bound its I/O.

SQLite `:memory:` creates a separate database per task; use a file for persisted
or shared state. Tasks serialize within a module. Different modules may run
concurrently, so SQLite file contention must still be handled as an error.

## Connection loss

The synchronous MySQL driver reconnects/retries once on 2006/4031. For 2013, the
outcome of the current statement is unknown: it returns an error without replay,
marks the connection broken, and reconnects before the next operation. Never
blindly retry non-idempotent writes. Threaded tasks use new connections, so an
idle or failed previous task does not poison later tasks. No background task is
silently replayed.

General non-SQL jobs can use `authum_tasks_submit(authum_host_get_tasks(host), ...)`
under the same ownership and callback rules.
