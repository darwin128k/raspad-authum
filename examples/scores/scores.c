/**
 * scores — the owner side of a module-to-module conversation.
 *
 * Keeps a score per player and offers it to other modules through the event
 * bus, as declared in scores_event.h:
 *   - publishes `scores.changed` (a forward) when a score changes;
 *   - answers `scores.query` (fills the payload, returns handled).
 *
 * `authum scores add <index> <points>` changes a score by hand. See
 * ../scoreboard for the module on the other side.
 */

#include "scores_event.h"

enum
{
    scores_slots = 33 /* 1 … 32 */
};

authum_host_t g_scores_host;
authum_logger_t g_scores_log;
authum_i32_t g_scores[scores_slots];

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "scores",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "SCORES",
};

void
scores_add(authum_i32_t index, authum_i32_t delta)
{
    scores_changed_t ev;

    g_scores[index] += delta;
    /* Publish a copy: subscribers cannot touch g_scores through it. */
    ev.size = sizeof ev;
    ev.index = index;
    ev.score = g_scores[index];
    ev.delta = delta;
    (void)authum_host_publish(&g_scores_host, SCORES_EVENT_CHANGED, &ev);
}

authum_event_result_t
scores_on_query(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    scores_query_t *q = (scores_query_t *)payload;

    (void)name;
    (void)user;
    if (authum_null_eq(q) || q->size < sizeof *q || q->index < 1 || q->index >= scores_slots)
    {
        return authum_event_result_continue; /* not ours to answer */
    }
    q->found = authum_bool_true;
    q->score = g_scores[q->index];
    return authum_event_result_handled;
}

/* A slot's score starts over with its next player. */
authum_event_result_t
scores_on_disconnect(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = (const authum_event_client_t *)payload;

    (void)name;
    (void)user;
    if (!authum_null_eq(ev) && ev->size >= sizeof *ev && ev->index >= 1 && ev->index < scores_slots)
    {
        g_scores[ev->index] = 0;
    }
    return authum_event_result_continue;
}

/* authum scores add <index> <points> */
authum_bool_t
scores_cmd_add(const authum_command_t *cmd, authum_ptr user)
{
    authum_i32_t index;
    authum_i32_t points;

    (void)user;
    if (!authum_command_arg_i32(cmd, 0u, &index) || !authum_command_arg_i32(cmd, 1u, &points))
    {
        return authum_bool_false;
    }
    if (!authum_host_client_is_connected(&g_scores_host, index) || index >= scores_slots)
    {
        authum_command_printf(cmd, "no player in slot %d", (int)index);
        return authum_bool_true;
    }
    scores_add(index, points);
    authum_logger_info(&g_scores_log, "#%d %+d -> %d", (int)index, (int)points,
                       (int)g_scores[index]);
    authum_command_printf(cmd, "#%d now has %d", (int)index, (int)g_scores[index]);
    return authum_bool_true;
}

const authum_command_def_t g_scores_commands[] = {
    {"add", "<index> <points>", "change a player's score", scores_cmd_add, authum_null},
};

authum_bool_t
scores_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_scores_host, host);
    authum_logger_init(&g_scores_log, &g_scores_host);
    return authum_host_subscribe(&g_scores_host, SCORES_EVENT_QUERY, scores_on_query,
                                 authum_null) &&
           authum_host_subscribe(&g_scores_host, AUTHUM_EVENT_CLIENT_DISCONNECT,
                                 scores_on_disconnect, authum_null) &&
           authum_host_command_add_all(&g_scores_host, g_scores_commands,
                                       sizeof g_scores_commands / sizeof g_scores_commands[0]);
}

void
scores_stop(void)
{
    authum_logger_close(&g_scores_log);
    authum_host_clear(&g_scores_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = scores_start,
    .stop = scores_stop,
};
