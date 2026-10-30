/**
 * scriptlike — a module that behaves the way a script will.
 *
 * It reads its rule from a file, watches the connect event and decides with a
 * hook. A script layer does the same through a bridge, so this module shows
 * that the public API of Authum is already everything a script needs: files,
 * events, hooks, the log and the players.
 *
 * The rule is the first character of the file: the players whose name starts
 * with it are not let in. No file, no rule — everybody is let in.
 */

#include <authum/client.h>
#include <authum/file.h>

authum_host_t g_scriptlike_host;
authum_logger_t g_scriptlike_log;
authum_char_t g_scriptlike_rule[2];

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "scriptlike",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-30",
    .author = "Authum examples",
    .url = "",
    .tag = "SCRIPTLIKE",
};

authum_bool_t
scriptlike_read_rule(void)
{
    authum_file_t file;
    authum_path_t path;
    authum_usize_t size = 0u;

    if (!authum_host_join_data_path(&g_scriptlike_host, "rules.txt", &path))
    {
        return authum_bool_false;
    }
    authum_file_init(&file);
    if (!authum_file_open(&file, &path, authum_file_mode_read))
    {
        return authum_bool_false;
    }
    if (!authum_file_read_all(&file, &g_scriptlike_rule, sizeof g_scriptlike_rule - 1u, &size))
    {
        authum_file_close(&file);
        return authum_bool_false;
    }
    authum_file_close(&file);
    g_scriptlike_rule[size] = '\0';
    return authum_bool_true;
}

/** A player whose name starts with the rule is not welcome. */
authum_bool_t
scriptlike_on_connect(authum_ptr user, authum_hook_call_t *call)
{
    authum_unused(user);
    if (g_scriptlike_rule[0] == '\0' || call->name[0] == '\0')
    {
        return authum_bool_true;
    }
    if (call->name[0] != g_scriptlike_rule[0])
    {
        return authum_bool_true;
    }
    call->proceed = authum_bool_false;
    authum_cstr_copy(call->reject, sizeof call->reject, "This server does not welcome you.");
    return authum_bool_false;
}

authum_event_result_t
scriptlike_on_event(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = (const authum_event_client_t *)payload;

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev))
    {
        return authum_event_result_continue;
    }
    authum_logger_info(&g_scriptlike_log, "#%d %s connected", ev->index, ev->name);
    return authum_event_result_continue;
}

authum_bool_t
scriptlike_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_scriptlike_host, host);
    authum_logger_init(&g_scriptlike_log, &g_scriptlike_host);
    if (!scriptlike_read_rule())
    {
        authum_logger_info(&g_scriptlike_log, "no rule, everybody is let in");
    }
    else
    {
        authum_logger_info(&g_scriptlike_log, "rule: %c", g_scriptlike_rule[0]);
    }
    return authum_host_subscribe(&g_scriptlike_host, AUTHUM_EVENT_CLIENT_CONNECT,
                                 scriptlike_on_event, authum_null) &&
           authum_hooks_add(authum_host_get_hooks(&g_scriptlike_host),
                            authum_hook_point_client_connect, scriptlike_on_connect, authum_null);
}

void
scriptlike_stop(void)
{
    /* Authum removes this module's handlers when the module unloads. */
    authum_logger_info(&g_scriptlike_log, "stop");
    authum_logger_close(&g_scriptlike_log);
    authum_host_clear(&g_scriptlike_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = scriptlike_start,
    .stop = scriptlike_stop,
};
