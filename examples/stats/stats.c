/**
 * stats — SQLite plus a journal.
 *
 *   - data/stats/stats.db: one row per auth id with its visit count;
 *   - every visit is written to a journal, logs/stats/visits/YYYY-MM-DD.log,
 *     apart from the module's own log;
 *   - the player sees "visit #N" in chat;
 *   - `authum stats top [count]` lists the most frequent players.
 *
 * SQL goes through <authum/sql.h>: the module never includes sqlite3.h.
 * Parameters (`?`) count from 1, result columns from 0.
 */

#include <authum/client.h>

#include <stdio.h>
#include <string.h>

authum_host_t g_stats_host;
authum_logger_t g_stats_log;
authum_logger_t g_stats_visits;
authum_sql_t g_stats_db;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "stats",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "STATS",
};

/** Count one more visit for @p auth_id; the new count, or 0 on a database error. */
authum_i32_t
stats_count_visit(const authum_char_t *auth_id, const authum_char_t *name, authum_i64_t now)
{
    authum_sql_stmt_t stmt;
    authum_i32_t visits = 0;

    if (!authum_sql_prepare(&g_stats_db, &stmt,
                            "INSERT INTO visits (auth_id, name, visits, last_seen) "
                            "VALUES (?, ?, 1, ?) "
                            "ON CONFLICT (auth_id) DO UPDATE SET visits = visits + 1, "
                            "name = excluded.name, last_seen = excluded.last_seen"))
    {
        authum_logger_error(&g_stats_log, "prepare: %s", authum_sql_errmsg(&g_stats_db));
        return 0;
    }
    authum_sql_stmt_bind_cstr(&stmt, 1, auth_id);
    authum_sql_stmt_bind_cstr(&stmt, 2, name);
    authum_sql_stmt_bind_i64(&stmt, 3, now);
    if (!authum_sql_stmt_step_done(&stmt))
    {
        authum_logger_error(&g_stats_log, "insert: %s", authum_sql_errmsg(&g_stats_db));
    }
    authum_sql_stmt_finalize(&stmt);

    if (authum_sql_prepare(&g_stats_db, &stmt, "SELECT visits FROM visits WHERE auth_id = ?"))
    {
        authum_sql_stmt_bind_cstr(&stmt, 1, auth_id);
        if (authum_sql_stmt_step_row(&stmt))
        {
            visits = authum_sql_stmt_column_i32(&stmt, 0);
        }
        authum_sql_stmt_finalize(&stmt);
    }
    return visits;
}

authum_event_result_t
stats_on_put_in_server(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = (const authum_event_client_t *)payload;
    authum_char_t text[96];
    authum_i32_t visits;

    (void)name;
    (void)user;
    if (authum_null_eq(ev) || ev->size < sizeof *ev)
    {
        return authum_event_result_continue;
    }
    /* Bots and players whose auth id is not known yet are not counted. */
    if (ev->auth_id[0] == '\0' || strcmp(ev->auth_id, "BOT") == 0)
    {
        return authum_event_result_continue;
    }
    visits = stats_count_visit(ev->auth_id, ev->name, ev->unix_time);
    if (visits == 0)
    {
        return authum_event_result_continue;
    }
    authum_logger_info(&g_stats_visits, "%s <%s> visit #%d", ev->name, ev->auth_id, (int)visits);
    snprintf(text, sizeof text, "Visit #%d. Welcome back!\n", (int)visits);
    authum_host_client_print(&g_stats_host, ev->index, authum_engine_print_chat, text);
    return authum_event_result_continue;
}

/* authum stats top [count] */
authum_bool_t
stats_cmd_top(const authum_command_t *cmd, authum_ptr user)
{
    authum_sql_stmt_t stmt;
    authum_i32_t count = 5;
    authum_i32_t place = 0;

    (void)user;
    if (authum_command_argc(cmd) > 0u && (!authum_command_arg_i32(cmd, 0u, &count) || count < 1))
    {
        return authum_bool_false; /* the host prints the usage line */
    }
    if (!authum_sql_prepare(
            &g_stats_db, &stmt,
            "SELECT name, auth_id, visits FROM visits ORDER BY visits DESC LIMIT ?"))
    {
        authum_command_printf(cmd, "database error: %s", authum_sql_errmsg(&g_stats_db));
        return authum_bool_true;
    }
    authum_sql_stmt_bind_i32(&stmt, 1, count);
    while (authum_sql_stmt_step_row(&stmt))
    {
        authum_command_printf(
            cmd, "%2d. %-24s %-22s %d", (int)++place, authum_sql_stmt_column_cstr(&stmt, 0),
            authum_sql_stmt_column_cstr(&stmt, 1), (int)authum_sql_stmt_column_i32(&stmt, 2));
    }
    authum_sql_stmt_finalize(&stmt);
    if (place == 0)
    {
        authum_command_print(cmd, "no visits yet");
    }
    return authum_bool_true;
}

const authum_command_def_t g_stats_commands[] = {
    {"top", "[count]", "most frequent players", stats_cmd_top, authum_null},
};

authum_bool_t
stats_start(authum_host_t *host)
{
    authum_path_t path;

    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_stats_host, host);
    authum_logger_init(&g_stats_log, &g_stats_host);
    authum_logger_open(&g_stats_visits, &g_stats_host, "visits");

    authum_sql_init(&g_stats_db);
    /* data/stats/stats.db: what a module writes lives under data/. */
    if (!authum_host_data_path(&g_stats_host, "stats.db", &path) ||
        !authum_sql_open_sqlite(&g_stats_db, authum_path_as_cstr(&path)) ||
        !authum_sql_exec(&g_stats_db, "CREATE TABLE IF NOT EXISTS visits ("
                                      "auth_id TEXT PRIMARY KEY, name TEXT NOT NULL, "
                                      "visits INTEGER NOT NULL, last_seen INTEGER NOT NULL)"))
    {
        /* Without its database the module is useless: refuse to load. */
        authum_logger_error(&g_stats_log, "stats.db: %s", authum_sql_errmsg(&g_stats_db));
        authum_sql_close(&g_stats_db);
        return authum_bool_false;
    }
    return authum_host_subscribe(&g_stats_host, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER,
                                 stats_on_put_in_server, authum_null) &&
           authum_host_command_add_all(&g_stats_host, g_stats_commands,
                                       sizeof g_stats_commands / sizeof g_stats_commands[0]);
}

void
stats_stop(void)
{
    /* The database is not sandbox memory: close it here. */
    authum_sql_close(&g_stats_db);
    authum_logger_close(&g_stats_visits);
    authum_logger_close(&g_stats_log);
    authum_host_clear(&g_stats_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = stats_start,
    .stop = stats_stop,
};
