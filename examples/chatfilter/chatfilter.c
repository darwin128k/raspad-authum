/**
 * chatfilter — catch a player's command before the game sees it.
 *
 * authum.client.command carries every client command (`say`, `say_team`,
 * `menuselect`, …) with its arguments. Setting `block` swallows it: the game
 * DLL never runs it. Here: chat lines with a banned word are dropped and the
 * player is told why.
 */

#include <authum/client.h>

#include <string.h>

authum_host_t g_chatfilter_host;
authum_logger_t g_chatfilter_log;

/* Lower case; a real module would read them from its cfg (see ../settings). */
const authum_char_t *const g_chatfilter_words[] = {"cheat", "wallhack", "aimbot"};

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "chatfilter",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "CHATFILTER",
};

/** The banned word found in @p text (case-insensitive), or null. */
const authum_char_t *
chatfilter_find_word(const authum_char_t *text)
{
    authum_char_t lower[authum_event_client_command_args_max];
    authum_usize_t i;

    for (i = 0u; text[i] != '\0' && i < sizeof lower - 1u; ++i)
    {
        lower[i] = authum_char_ascii_lower(text[i]);
    }
    lower[i] = '\0';
    for (i = 0u; i < sizeof g_chatfilter_words / sizeof g_chatfilter_words[0]; ++i)
    {
        if (strstr(lower, g_chatfilter_words[i]) != authum_null)
        {
            return g_chatfilter_words[i];
        }
    }
    return authum_null;
}

authum_event_result_t
chatfilter_on_command(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_event_client_command_t *ev = (authum_event_client_command_t *)payload;
    const authum_char_t *word;

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev) || ev->argc < 1)
    {
        return authum_event_result_continue;
    }
    if (strcmp(ev->argv[0], "say") != 0 && strcmp(ev->argv[0], "say_team") != 0)
    {
        return authum_event_result_continue;
    }
    word = chatfilter_find_word(ev->args);
    if (authum_null_eq(word))
    {
        return authum_event_result_continue;
    }

    ev->block = authum_bool_true;
    authum_host_client_print(&g_chatfilter_host, ev->client.index, authum_engine_print_chat,
                             "Your message was not sent: it contains a banned word.\n");
    authum_logger_warning(&g_chatfilter_log, "blocked %s <%s>: \"%s\" (word \"%s\")",
                          ev->client.name, ev->client.auth_id, ev->args, word);
    /* Handled: later subscribers still see the event and can read `block`. */
    return authum_event_result_handled;
}

authum_bool_t
chatfilter_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_chatfilter_host, host);
    authum_logger_init(&g_chatfilter_log, &g_chatfilter_host);
    return authum_host_subscribe(&g_chatfilter_host, AUTHUM_EVENT_CLIENT_COMMAND,
                                 chatfilter_on_command, authum_null);
}

void
chatfilter_stop(void)
{
    authum_logger_close(&g_chatfilter_log);
    authum_host_clear(&g_chatfilter_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = chatfilter_start,
    .stop = chatfilter_stop,
};
