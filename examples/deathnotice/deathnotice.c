/**
 * deathnotice — read a game message and answer in colored chat.
 *
 *   - authum.message.DeathMsg: the game's kill notice (killer, victim,
 *     headshot, weapon); we announce it in chat with colors;
 *   - its own server cvars: `deathnotice_enabled`, `deathnotice_allow_kill`
 *     (set them in server.cfg like any other cvar);
 *   - authum.client.kill: refuse the `kill` command when it is not allowed.
 *
 * Counter-Strike chat colors: \x01 default, \x04 green, \x03 the team color of
 * the player given as sender.
 */

#include <authum/client.h>

#include <stdio.h>

authum_host_t g_deathnotice_host;
authum_logger_t g_deathnotice_log;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "deathnotice",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-23",
    .author = "Authum examples",
    .url = "",
    .logtag = "DEATHNOTICE",
};

authum_event_result_t
deathnotice_on_death(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_message_t *ev = (const authum_event_message_t *)payload;
    authum_i32_t killer;
    authum_i32_t victim;
    authum_char_t killer_name[64];
    authum_char_t victim_name[64];
    authum_char_t text[192];

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev) || ev->size < sizeof *ev ||
        authum_host_cvar_get_i32(&g_deathnotice_host, "deathnotice_enabled", 1) == 0)
    {
        return authum_event_result_continue;
    }
    /* DeathMsg: byte killer, byte victim, byte headshot, string weapon. */
    killer = authum_event_message_get_i32(ev, 0);
    victim = authum_event_message_get_i32(ev, 1);
    if (!authum_host_client_get_name(&g_deathnotice_host, victim, victim_name, sizeof victim_name))
    {
        return authum_event_result_continue;
    }
    if (killer == victim ||
        !authum_host_client_get_name(&g_deathnotice_host, killer, killer_name, sizeof killer_name))
    {
        snprintf(text, sizeof text, "\x04[Death]\x01 %s died", victim_name);
    }
    else
    {
        snprintf(text, sizeof text, "\x04[Death]\x03 %s\x01 killed %s with %s%s", killer_name,
                 victim_name, authum_event_message_get_string(ev, 3),
                 authum_event_message_get_i32(ev, 2) != 0 ? " (headshot)" : "");
    }
    /* Sender = the killer: \x03 takes their team color. 0 = everyone. */
    authum_host_client_chat(&g_deathnotice_host, 0, killer, text);
    return authum_event_result_continue;
}

authum_event_result_t
deathnotice_on_kill(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_event_client_t *ev = (authum_event_client_t *)payload;

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev) || ev->size < sizeof *ev ||
        authum_host_cvar_get_i32(&g_deathnotice_host, "deathnotice_allow_kill", 1) != 0)
    {
        return authum_event_result_continue;
    }
    ev->allow = authum_bool_false;
    authum_host_client_chat(&g_deathnotice_host, ev->index, ev->index,
                            "\x04[Death]\x01 Suicide is disabled on this server.");
    authum_logger_info(&g_deathnotice_log, "refused kill from %s", ev->name);
    return authum_event_result_handled;
}

authum_bool_t
deathnotice_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_deathnotice_host, host);
    authum_logger_init(&g_deathnotice_log, &g_deathnotice_host);
    /* Created once; they keep their value across module reloads. */
    authum_host_cvar_register(&g_deathnotice_host, "deathnotice_enabled", "1",
                              authum_engine_cvar_server);
    authum_host_cvar_register(&g_deathnotice_host, "deathnotice_allow_kill", "1", 0);
    return authum_host_subscribe(&g_deathnotice_host, AUTHUM_EVENT_MESSAGE_PREFIX "DeathMsg",
                                 deathnotice_on_death, authum_null) &&
           authum_host_subscribe(&g_deathnotice_host, AUTHUM_EVENT_CLIENT_KILL, deathnotice_on_kill,
                                 authum_null);
}

void
deathnotice_stop(void)
{
    authum_logger_close(&g_deathnotice_log);
    authum_host_clear(&g_deathnotice_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = deathnotice_start,
    .stop = deathnotice_stop,
};
