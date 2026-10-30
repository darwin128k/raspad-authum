/**
 * hookguard — a module that stands in the call, not only in the event.
 *
 * An event tells the module that a player is connecting; a hook lets it act
 * on the call itself. The rule here is small on purpose: a player whose name
 * starts with "!" is not let in, and everybody else is. The handler sees the
 * call, may change it, and may stop it with the reason the player is shown.
 *
 * The chain belongs to Authum: the order of the handlers, the budget one of
 * them may spend, and the removal of this handler when the module unloads.
 */

#include <authum/client.h>

authum_host_t g_hookguard_host;
authum_logger_t g_hookguard_log;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "hookguard",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-30",
    .author = "Authum examples",
    .url = "",
    .tag = "HOOKGUARD",
};

/** A name that starts with "!" is not welcome here. */
authum_bool_t
hookguard_on_connect(authum_ptr user, authum_hook_call_t *call)
{
    authum_unused(user);
    if (call->name[0] != '!')
    {
        return authum_bool_true;
    }
    call->proceed = authum_bool_false;
    authum_cstr_copy(call->reject, sizeof call->reject, "This server does not welcome you.");
    return authum_bool_false;
}

authum_bool_t
hookguard_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_hookguard_host, host);
    authum_logger_init(&g_hookguard_log, &g_hookguard_host);
    authum_logger_info(&g_hookguard_log, "start, a name starting with ! is refused");
    return authum_hooks_add(authum_host_get_hooks(&g_hookguard_host),
                            authum_hook_point_client_connect, hookguard_on_connect, authum_null);
}

void
hookguard_stop(void)
{
    /* Authum removes this module's handlers when the module unloads. */
    authum_logger_info(&g_hookguard_log, "stop");
    authum_logger_close(&g_hookguard_log);
    authum_host_clear(&g_hookguard_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = hookguard_start,
    .stop = hookguard_stop,
};
