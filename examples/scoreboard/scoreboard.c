/**
 * scoreboard — the other side of ../scores: uses its events without linking it.
 *
 *   - listens to `scores.changed` and announces it in chat;
 *   - `authum scoreboard show <index>` asks through `scores.query`.
 *
 * It only includes scores/event.h (types and names). If `scores` is not
 * loaded, nothing is announced and the query comes back unanswered — which
 * the command reports instead of failing.
 */

#include <authum/client.h>
#include <authum/unused.h>
#include <scores/event.h>

#include <stdio.h>

authum_host_t g_scoreboard_host;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "scoreboard",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "SCOREBOARD",
};

authum_event_result_t
scoreboard_on_changed(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const scores_changed_t *ev = (const scores_changed_t *)payload;
    authum_char_t player[64];
    authum_char_t text[128];

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev))
    {
        return authum_event_result_continue;
    }
    authum_host_client_copy_name(&g_scoreboard_host, ev->index, player, sizeof player);
    snprintf(text, sizeof text, "%s: %+d, now %d points\n", player, ev->delta, ev->score);
    authum_host_client_print(&g_scoreboard_host, 0, authum_engine_print_chat, text); /* 0 = all */
    return authum_event_result_continue;
}

/* authum scoreboard show <index> */
authum_bool_t
scoreboard_cmd_show(const authum_command_t *cmd, authum_ptr user)
{
    scores_query_t q;

    authum_unused(user);
    q.size = sizeof q;
    q.found = authum_bool_false;
    q.score = 0;
    if (!authum_command_try_get_arg_i32(cmd, 0u, &q.index))
    {
        return authum_bool_false;
    }
    /* continue = nobody answered: the owner is not loaded, or it did not know. */
    if (authum_host_publish(&g_scoreboard_host, SCORES_EVENT_QUERY, &q) ==
            authum_event_result_continue ||
        !q.found)
    {
        authum_command_print(cmd, "no answer (is the scores module loaded?)");
        return authum_bool_true;
    }
    authum_command_printf(cmd, "#%d has %d points", q.index, q.score);
    return authum_bool_true;
}

const authum_command_def_t g_scoreboard_commands[] = {
    {"show", "<index>", "ask the scores module for a score", scoreboard_cmd_show, authum_null},
};

authum_bool_t
scoreboard_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_scoreboard_host, host);
    /* Subscribing to an event nobody publishes yet is fine: load order does not matter. */
    return authum_host_subscribe(&g_scoreboard_host, SCORES_EVENT_CHANGED, scoreboard_on_changed,
                                 authum_null) &&
           authum_host_command_add_all(&g_scoreboard_host, g_scoreboard_commands,
                                       sizeof g_scoreboard_commands /
                                           sizeof g_scoreboard_commands[0]);
}

void
scoreboard_stop(void)
{
    authum_host_clear(&g_scoreboard_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = scoreboard_start,
    .stop = scoreboard_stop,
};
