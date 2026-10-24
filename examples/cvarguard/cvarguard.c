/**
 * cvarguard — ask a client for a cvar and act on the answer.
 *
 * authum_host_query_cvar sends the question; the answer comes back later as
 * authum.client.cvar_value with the same request id. Here every player who
 * enters is asked for `fps_max` and kicked above a limit.
 *
 * The request id is the player's userid: if the slot has changed hands by
 * the time the answer arrives, the ids no longer match and it is ignored.
 */

#include <authum/client.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CVARGUARD_CVAR "fps_max"
#define CVARGUARD_LIMIT 101.0

authum_host_t g_cvarguard_host;
authum_logger_t g_cvarguard_log;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "cvarguard",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "CVARGUARD",
};

authum_event_result_t
cvarguard_on_put_in_server(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = (const authum_event_client_t *)payload;

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev) || ev->size < sizeof *ev)
    {
        return authum_event_result_continue;
    }
    if (!authum_host_query_cvar(&g_cvarguard_host, ev->index, CVARGUARD_CVAR, ev->userid))
    {
        authum_logger_warning(&g_cvarguard_log, "could not ask %s for %s", ev->name,
                              CVARGUARD_CVAR);
    }
    return authum_event_result_continue;
}

authum_event_result_t
cvarguard_on_cvar_value(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_cvar_t *ev = (const authum_event_client_cvar_t *)payload;
    char *end;
    double value;
    authum_char_t reason[96];

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev) || ev->size < sizeof *ev || strcmp(ev->cvar, CVARGUARD_CVAR) != 0)
    {
        return authum_event_result_continue;
    }
    /* Someone else's question, or the player has left since. */
    if (ev->request_id != authum_host_client_userid(&g_cvarguard_host, ev->client.index))
    {
        return authum_event_result_continue;
    }

    value = strtod(ev->value, &end);
    if (end == ev->value)
    {
        /* The engine answers "Bad CVAR request" / "Bad Player" instead of a value. */
        authum_logger_notice(&g_cvarguard_log, "%s: no %s (%s)", ev->client.name, ev->cvar,
                             ev->value);
        return authum_event_result_continue;
    }
    authum_logger_debug(&g_cvarguard_log, "%s: %s = %s", ev->client.name, ev->cvar, ev->value);
    if (value > CVARGUARD_LIMIT)
    {
        snprintf(reason, sizeof reason, "Set %s to %d or lower", CVARGUARD_CVAR,
                 (int)CVARGUARD_LIMIT);
        authum_logger_info(&g_cvarguard_log, "kick %s <%s>: %s = %s", ev->client.name,
                           ev->client.auth_id, ev->cvar, ev->value);
        authum_host_kick(&g_cvarguard_host, ev->request_id, reason);
    }
    return authum_event_result_continue;
}

authum_bool_t
cvarguard_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_cvarguard_host, host);
    authum_logger_init(&g_cvarguard_log, &g_cvarguard_host);
    return authum_host_subscribe(&g_cvarguard_host, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER,
                                 cvarguard_on_put_in_server, authum_null) &&
           authum_host_subscribe(&g_cvarguard_host, AUTHUM_EVENT_CLIENT_CVAR_VALUE,
                                 cvarguard_on_cvar_value, authum_null);
}

void
cvarguard_stop(void)
{
    authum_logger_close(&g_cvarguard_log);
    authum_host_clear(&g_cvarguard_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = cvarguard_start,
    .stop = cvarguard_stop,
};
