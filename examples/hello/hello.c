/**
 * hello — a short tour of what almost every module needs:
 *   - the logger (logs/hello/YYYY-MM-DD.log),
 *   - an event subscription (authum.client.connect),
 *   - memory from the module sandbox (nothing to free in stop),
 *   - a console / RCON command (`authum hello seen`).
 *
 * Start from ../skeleton for an empty module; come here for these pieces.
 */

#include <authum/client.h>

authum_host_t g_hello_host;
authum_logger_t g_hello_log;
authum_array_t g_hello_seen;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "hello",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "HELLO",
};

authum_event_result_t
hello_on_connect(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = (const authum_event_client_t *)payload;

    authum_unused(name);
    authum_unused(user);
    /* An older host may send a shorter payload: read only what is there. */
    if (authum_null_eq(ev) || ev->size < sizeof *ev)
    {
        return authum_event_result_continue;
    }
    authum_array_push(&g_hello_seen, ev->name);
    authum_logger_info(&g_hello_log, "#%d %s connected (%u seen since load)", ev->index,
                       ev->name, authum_array_size(&g_hello_seen));
    return authum_event_result_continue;
}

/* authum hello seen  (server console or RCON) */
authum_bool_t
hello_cmd_seen(const authum_command_t *cmd, authum_ptr user)
{
    authum_usize_t i;

    authum_unused(user);
    authum_command_printf(cmd,
                          "%u connects since load:", authum_array_size(&g_hello_seen));
    for (i = 0u; i < authum_array_size(&g_hello_seen); ++i)
    {
        authum_command_printf(cmd, "  %s",
                              (const authum_char_t *)authum_array_at(&g_hello_seen, i));
    }
    return authum_bool_true;
}

const authum_command_def_t g_hello_commands[] = {
    {"seen", authum_null, "names that connected since load", hello_cmd_seen, authum_null},
};

authum_bool_t
hello_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_hello_host, host);
    authum_logger_init(&g_hello_log, &g_hello_host);
    authum_logger_info(&g_hello_log, "start, data dir %s",
                       authum_path_as_cstr(authum_host_env_module_data(&g_hello_host)));

    /* Each item is a name buffer; the storage lives in this module's sandbox. */
    authum_array_init(&g_hello_seen, authum_host_sandbox(&g_hello_host),
                      authum_event_client_name_max);
    return authum_host_subscribe(&g_hello_host, AUTHUM_EVENT_CLIENT_CONNECT, hello_on_connect,
                                 authum_null) &&
           authum_host_command_add_all(&g_hello_host, g_hello_commands,
                                       sizeof g_hello_commands / sizeof g_hello_commands[0]);
}

void
hello_stop(void)
{
    /* Subscription, command and g_hello_seen storage are released by Authum after this. */
    authum_logger_info(&g_hello_log, "stop");
    authum_logger_close(&g_hello_log);
    authum_host_clear(&g_hello_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = hello_start,
    .stop = hello_stop,
};
