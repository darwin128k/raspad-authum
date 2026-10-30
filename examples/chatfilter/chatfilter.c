/**
 * chatfilter — catch a player's command before the game sees it.
 *
 * The module stands in the command itself: a hook
 * (::authum_hook_point_client_command) hands the handler the call — the
 * command and its arguments — before the game DLL runs it. A chat line with a
 * banned word stops the call, so the game never sees it, and the player is
 * told why.
 */

#include <authum/client.h>

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
    .tag = "CHATFILTER",
};

/** The banned word found in @p text (case-insensitive), or null. */
const authum_char_t *
chatfilter_find_word(const authum_char_t *text)
{
    authum_char_t lower[authum_hook_args_max];
    authum_usize_t i;

    for (i = 0u; text[i] != '\0' && i < sizeof lower - 1u; ++i)
    {
        lower[i] = authum_char_to_lower(text[i]);
    }
    lower[i] = '\0';
    for (i = 0u; i < sizeof g_chatfilter_words / sizeof g_chatfilter_words[0]; ++i)
    {
        if (authum_cstr_contains(lower, g_chatfilter_words[i]))
        {
            return g_chatfilter_words[i];
        }
    }
    return authum_null;
}

/** The chat line is dropped when it carries a banned word. */
authum_bool_t
chatfilter_on_command(authum_ptr user, authum_hook_call_t *call)
{
    authum_char_t auth_id[authum_event_client_auth_id_max];
    const authum_char_t *word;

    authum_unused(user);
    if (!authum_cstr_is_equal(call->command, "say", authum_bool_false) &&
        !authum_cstr_is_equal(call->command, "say_team", authum_bool_false))
    {
        return authum_bool_true;
    }
    word = chatfilter_find_word(call->args);
    if (authum_null_eq(word))
    {
        return authum_bool_true;
    }

    call->proceed = authum_bool_false;
    authum_host_client_print(&g_chatfilter_host, call->index, authum_engine_print_chat,
                             "Your message was not sent: it contains a banned word.\n");
    if (!authum_host_client_copy_auth_id(&g_chatfilter_host, call->index, auth_id, sizeof auth_id))
    {
        authum_cstr_copy(auth_id, sizeof auth_id, "");
    }
    authum_logger_warning(&g_chatfilter_log, "blocked %s <%s>: \"%s\" (word \"%s\")", call->name,
                          auth_id, call->args, word);
    return authum_bool_false;
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
    return authum_hooks_add(authum_host_get_hooks(&g_chatfilter_host),
                            authum_hook_point_client_command, chatfilter_on_command, authum_null);
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
