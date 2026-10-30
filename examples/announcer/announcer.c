/**
 * announcer — timers, precache, the HLDS log and entity fields.
 *
 *   - a timer posts a message in chat every `announcer_interval` seconds;
 *   - authum.server.precache: the sound it plays is precached while the map
 *     loads (the only time the engine allows it);
 *   - authum.server.log: on the game's `World triggered "Round_Start"` line
 *     every player hears the sound (needs `log on`);
 *   - `authum announcer hp`: the living players and their health, read from
 *     entity fields.
 */

#include <authum/client.h>

#define ANNOUNCER_SOUND "buttons/bell1.wav"

authum_host_t g_announcer_host;
authum_logger_t g_announcer_log;
authum_u32_t g_announcer_timer;
authum_i32_t g_announcer_count;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "announcer",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-23",
    .author = "Authum examples",
    .url = "",
    .tag = "ANNOUNCER",
};

void
announcer_tick(authum_ptr user)
{
    authum_char_t text[128];

    authum_unused(user);
    ++g_announcer_count;
    authum_cstr_format(text, sizeof text, "\x04[Info]\x01 This server runs Authum. Message #%d.",
                       g_announcer_count);
    authum_host_client_chat(&g_announcer_host, 0, 0, text);
}

authum_event_result_t
announcer_on_precache(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_unused(name);
    authum_unused(payload);
    authum_unused(user);
    if (authum_host_precache_sound(&g_announcer_host, ANNOUNCER_SOUND) == 0)
    {
        authum_logger_warning(&g_announcer_log, "could not precache %s", ANNOUNCER_SOUND);
    }
    return authum_event_result_continue;
}

authum_event_result_t
announcer_on_log(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_server_log_t *ev = (const authum_event_server_log_t *)payload;
    authum_i32_t max = authum_host_get_max_clients(&g_announcer_host);
    authum_i32_t index;

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev) || !authum_cstr_contains(ev->text, "World triggered \"Round_Start\""))
    {
        return authum_event_result_continue;
    }
    for (index = 1; index <= max; ++index)
    {
        if (authum_host_client_is_connected(&g_announcer_host, index) &&
            !authum_host_client_is_bot(&g_announcer_host, index))
        {
            authum_host_client_command(&g_announcer_host, index, "spk " ANNOUNCER_SOUND);
        }
    }
    return authum_event_result_continue;
}

/* authum announcer hp */
authum_bool_t
announcer_cmd_hp(const authum_command_t *cmd, authum_ptr user)
{
    authum_i32_t max = authum_host_get_max_clients(&g_announcer_host);
    authum_i32_t index;
    authum_char_t player[64];
    authum_float_t health;
    authum_float_t armor;
    authum_usize_t alive = 0u;

    authum_unused(user);
    for (index = 1; index <= max; ++index)
    {
        if (!authum_host_client_is_alive(&g_announcer_host, index))
        {
            continue;
        }
        authum_host_client_copy_name(&g_announcer_host, index, player, sizeof player);
        authum_host_entity_try_get_float(&g_announcer_host, index, authum_entity_float_health,
                                         &health);
        authum_host_entity_try_get_float(&g_announcer_host, index, authum_entity_float_armor,
                                         &armor);
        authum_command_printf(cmd, "  %-24s %3d hp %3d armor", player, (int)health, (int)armor);
        ++alive;
    }
    authum_command_printf(cmd, "%u alive", alive);
    return authum_bool_true;
}

const authum_command_def_t g_announcer_commands[] = {
    {"hp", authum_null, "living players and their health", announcer_cmd_hp, authum_null},
};

authum_bool_t
announcer_start(authum_host_t *host)
{
    authum_float_t interval;

    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_announcer_host, host);
    authum_logger_init(&g_announcer_log, &g_announcer_host);
    authum_host_cvar_register(&g_announcer_host, "announcer_interval", "120", 0);
    interval = authum_host_cvar_get_float(&g_announcer_host, "announcer_interval", 120.0f);
    if (interval < 10.0f)
    {
        interval = 10.0f;
    }
    g_announcer_timer =
        authum_timer_add(&g_announcer_host, interval, interval, announcer_tick, authum_null);
    return g_announcer_timer != 0u &&
           authum_host_subscribe(&g_announcer_host, AUTHUM_EVENT_SERVER_PRECACHE,
                                 announcer_on_precache, authum_null) &&
           authum_host_subscribe(&g_announcer_host, AUTHUM_EVENT_SERVER_LOG, announcer_on_log,
                                 authum_null) &&
           authum_host_command_add_all(&g_announcer_host, g_announcer_commands,
                                       sizeof g_announcer_commands /
                                           sizeof g_announcer_commands[0]);
}

void
announcer_stop(void)
{
    /* The timer goes with the module; removing it is only for symmetry. */
    authum_timer_remove(g_announcer_timer);
    authum_logger_close(&g_announcer_log);
    authum_host_clear(&g_announcer_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = announcer_start,
    .stop = announcer_stop,
};
