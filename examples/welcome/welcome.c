/**
 * welcome — player and map events plus the engine facade.
 *
 *   - authum.client.put_in_server: greet the player in chat, log who they are
 *     (name, auth id, address) through authum_host_client_* calls;
 *   - authum.client.disconnect: log how long they stayed;
 *   - authum.server.activate: log the new map;
 *   - `authum welcome players`: list the connected players.
 *
 * Players are addressed by entity index (1 … maxClients), the same number
 * every authum.client.* event carries. No HLSDK, no engine_funcs.
 */

#include <authum/client.h>

#include <stdio.h>

enum
{
    welcome_slots = 33 /* 1 … 32 */
};

authum_host_t g_welcome_host;
authum_logger_t g_welcome_log;
/** game_time of each slot's put_in_server, to tell how long a player stayed. */
authum_float_t g_welcome_since[welcome_slots];

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "welcome",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "WELCOME",
};

authum_event_result_t
welcome_on_put_in_server(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = (const authum_event_client_t *)payload;
    authum_char_t address[64];
    authum_char_t map[64];
    authum_char_t text[192];

    (void)name;
    (void)user;
    if (authum_null_eq(ev) || ev->size < sizeof *ev || ev->index < 1 || ev->index >= welcome_slots)
    {
        return authum_event_result_continue;
    }
    g_welcome_since[ev->index] = ev->game_time;

    /* The payload has name and auth id; the address comes from the engine facade. */
    authum_host_client_get_address(&g_welcome_host, ev->index, address, sizeof address);
    authum_logger_info(&g_welcome_log, "#%d %s <%s> from %s", (int)ev->index, ev->name, ev->auth_id,
                       address);

    authum_host_get_map_name(&g_welcome_host, map, sizeof map);
    snprintf(text, sizeof text, "Welcome, %s! You are playing %s.\n", ev->name, map);
    authum_host_client_print(&g_welcome_host, ev->index, authum_engine_print_chat, text);
    return authum_event_result_continue;
}

authum_event_result_t
welcome_on_disconnect(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = (const authum_event_client_t *)payload;

    (void)name;
    (void)user;
    if (authum_null_eq(ev) || ev->size < sizeof *ev || ev->index < 1 || ev->index >= welcome_slots)
    {
        return authum_event_result_continue;
    }
    authum_logger_info(&g_welcome_log, "#%d %s left after %d s", (int)ev->index, ev->name,
                       (int)(ev->game_time - g_welcome_since[ev->index]));
    g_welcome_since[ev->index] = 0.0f;
    return authum_event_result_continue;
}

authum_event_result_t
welcome_on_activate(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_server_t *ev = (const authum_event_server_t *)payload;

    (void)name;
    (void)user;
    if (authum_null_eq(ev) || ev->size < sizeof *ev)
    {
        return authum_event_result_continue;
    }
    authum_logger_info(&g_welcome_log, "map %s, %d slots", ev->map, (int)ev->max_clients);
    return authum_event_result_continue;
}

/* authum welcome players */
authum_bool_t
welcome_cmd_players(const authum_command_t *cmd, authum_ptr user)
{
    authum_char_t player[64];
    authum_char_t auth_id[64];
    authum_i32_t max = authum_host_max_clients(&g_welcome_host);
    authum_i32_t index;
    authum_usize_t count = 0u;

    (void)user;
    for (index = 1; index <= max; ++index)
    {
        if (!authum_host_client_is_connected(&g_welcome_host, index))
        {
            continue;
        }
        authum_host_client_get_name(&g_welcome_host, index, player, sizeof player);
        authum_host_client_get_auth_id(&g_welcome_host, index, auth_id, sizeof auth_id);
        authum_command_printf(cmd, "  #%-2d userid %-4d %-24s %s", (int)index,
                              (int)authum_host_client_userid(&g_welcome_host, index), player,
                              auth_id);
        ++count;
    }
    authum_command_printf(cmd, "%u of %d slots in use", (unsigned)count, (int)max);
    return authum_bool_true;
}

const authum_command_def_t g_welcome_commands[] = {
    {"players", authum_null, "list connected players", welcome_cmd_players, authum_null},
};

authum_bool_t
welcome_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_welcome_host, host);
    authum_logger_init(&g_welcome_log, &g_welcome_host);
    return authum_host_subscribe(&g_welcome_host, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER,
                                 welcome_on_put_in_server, authum_null) &&
           authum_host_subscribe(&g_welcome_host, AUTHUM_EVENT_CLIENT_DISCONNECT,
                                 welcome_on_disconnect, authum_null) &&
           authum_host_subscribe(&g_welcome_host, AUTHUM_EVENT_SERVER_ACTIVATE, welcome_on_activate,
                                 authum_null) &&
           authum_host_command_add_all(&g_welcome_host, g_welcome_commands,
                                       sizeof g_welcome_commands / sizeof g_welcome_commands[0]);
}

void
welcome_stop(void)
{
    authum_logger_close(&g_welcome_log);
    authum_host_clear(&g_welcome_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = welcome_start,
    .stop = welcome_stop,
};
