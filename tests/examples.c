/**
 * @file examples.c
 * @brief Smoke test of examples/: load every built module with the real event
 *        bus, commands, sandbox and logs, a fake engine with three players,
 *        then play a short scenario and check what each module did.
 */

#include <check.h>

#include <authum/client.h>
#include <authum/commands/host.h>
#include <authum/events/host.h>
#include <authum/logger/host.h>
#include <authum/sandbox/host.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

#define TEST_ROOT "authum_examples_test_root"

/* --- fake engine ------------------------------------------------------------ */

enum
{
    test_max_clients = 4,
    test_out_max = 8192
};

typedef struct test_player
{
    authum_bool_t connected;
    authum_i32_t userid;
    const authum_char_t *name;
    const authum_char_t *auth_id;
    const authum_char_t *address;
} test_player_t;

test_player_t g_players[test_max_clients + 1] = {
    {0},
    {authum_bool_true, 11, "Vega", "STEAM_0:1:111", "10.0.0.1:27005"},
    {authum_bool_true, 12, "Bot Alice", "BOT", "loopback"},
    {authum_bool_true, 13, "Gordon", "STEAM_0:0:313", "10.0.0.3:27005"},
    {0},
};

authum_char_t g_prints[test_out_max];   /* client_print: "<index>|<text>" */
authum_char_t g_commands[test_out_max]; /* server_command */
authum_char_t g_out[test_out_max];      /* console command output */
authum_i32_t g_query_index;
authum_i32_t g_query_request;
authum_char_t g_query_cvar[64];

void
test_append(authum_char_t *buf, const authum_char_t *text)
{
    authum_usize_t used = strlen(buf);

    snprintf(buf + used, test_out_max - used, "%s", text);
}

test_player_t *
test_player(authum_i32_t index)
{
    return (index >= 1 && index <= test_max_clients && g_players[index].connected)
               ? &g_players[index]
               : authum_null;
}

void
test_get_game_dir(authum_char_t *buf, authum_usize_t size)
{
    authum_cstr_copy(buf, size, "cstrike");
}

void
test_server_print(const authum_char_t *text)
{
    (void)text;
}

void
test_server_command(const authum_char_t *cmd)
{
    test_append(g_commands, cmd);
}

authum_i32_t
test_max_clients_fn(void)
{
    return test_max_clients;
}

authum_float_t
test_game_time(void)
{
    return 42.0f;
}

void
test_get_map_name(authum_char_t *buf, authum_usize_t size)
{
    authum_cstr_copy(buf, size, "de_dust2");
}

authum_bool_t
test_client_is_connected(authum_i32_t index)
{
    return test_player(index) != authum_null ? authum_bool_true : authum_bool_false;
}

authum_i32_t
test_client_userid(authum_i32_t index)
{
    return test_player(index) != authum_null ? test_player(index)->userid : 0;
}

authum_bool_t
test_copy_field(authum_i32_t index, authum_usize_t offset, authum_char_t *buf, authum_usize_t size)
{
    test_player_t *player = test_player(index);

    if (player == authum_null)
    {
        return authum_bool_false;
    }
    authum_cstr_copy(buf, size, *(const authum_char_t *const *)((const char *)player + offset));
    return authum_bool_true;
}

authum_bool_t
test_client_get_name(authum_i32_t index, authum_char_t *buf, authum_usize_t size)
{
    return test_copy_field(index, offsetof(test_player_t, name), buf, size);
}

authum_bool_t
test_client_get_auth_id(authum_i32_t index, authum_char_t *buf, authum_usize_t size)
{
    return test_copy_field(index, offsetof(test_player_t, auth_id), buf, size);
}

authum_bool_t
test_client_get_address(authum_i32_t index, authum_char_t *buf, authum_usize_t size)
{
    return test_copy_field(index, offsetof(test_player_t, address), buf, size);
}

authum_bool_t
test_client_print(authum_i32_t index, authum_i32_t kind, const authum_char_t *text)
{
    authum_char_t line[256];

    (void)kind;
    snprintf(line, sizeof line, "%d|%s", (int)index, text);
    test_append(g_prints, line);
    return authum_bool_true;
}

authum_bool_t
test_query_client_cvar(authum_i32_t index, const authum_char_t *cvar, authum_i32_t request_id)
{
    if (test_player(index) == authum_null)
    {
        return authum_bool_false;
    }
    g_query_index = index;
    g_query_request = request_id;
    authum_cstr_copy(g_query_cvar, sizeof g_query_cvar, cvar);
    return authum_bool_true;
}

const authum_engine_t g_engine = {
    .size = sizeof(authum_engine_t),
    .get_game_dir = test_get_game_dir,
    .server_print = test_server_print,
    .server_command = test_server_command,
    .max_clients = test_max_clients_fn,
    .game_time = test_game_time,
    .get_map_name = test_get_map_name,
    .client_is_connected = test_client_is_connected,
    .client_userid = test_client_userid,
    .client_get_name = test_client_get_name,
    .client_get_auth_id = test_client_get_auth_id,
    .client_get_address = test_client_get_address,
    .client_print = test_client_print,
    .query_client_cvar = test_query_client_cvar,
};

/* --- loading a module the way the host does --------------------------------- */

typedef struct test_module
{
    const char *name;
    void *library;
    const authum_module_info_t *info;
    const authum_module_api_t *api;
    authum_host_t host;
    authum_env_t env;
    authum_path_t root;
    authum_path_t dir;
    authum_sandbox_t sandbox;
    authum_events_t events;
    authum_commands_t commands;
    authum_logger_channel_t logger;
    authum_bool_t started;
} test_module_t;

void *
test_dlopen(const char *path)
{
#if defined(_WIN32)
    return (void *)LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void *
test_dlsym(void *library, const char *name)
{
#if defined(_WIN32)
    return (void *)GetProcAddress((HMODULE)library, name);
#else
    return dlsym(library, name);
#endif
}

void
test_dlclose(void *library)
{
#if defined(_WIN32)
    FreeLibrary((HMODULE)library);
#else
    dlclose(library);
#endif
}

authum_bool_t
test_load(test_module_t *m)
{
    char path[512];
    authum_path_t modules;

    snprintf(path, sizeof path, "%s/%s/%s%s", AUTHUM_TEST_EXAMPLES_DIR, m->name, m->name,
             AUTHUM_TEST_MODULE_SUFFIX);
    m->library = test_dlopen(path);
    if (m->library == authum_null)
    {
        fprintf(stderr, "cannot load %s\n", path);
        return authum_bool_false;
    }
    m->info = (const authum_module_info_t *)test_dlsym(m->library, AUTHUM_MODULE_INFO_SYM);
    m->api = (const authum_module_api_t *)test_dlsym(m->library, AUTHUM_MODULE_API_SYM);
    if (m->info == authum_null || m->api == authum_null)
    {
        return authum_bool_false;
    }
    {
        const authum_version_t host_abi = AUTHUM_MODULE_ABI;
        CHECK(authum_version_is_abi_compatible(&m->info->abi, &host_abi));
    }
    CHECK_STR(m->info->name, m->name);

    authum_path_set_cstr(&m->root, TEST_ROOT);
    authum_path_join(&modules, &m->root, "modules");
    authum_path_ensure_dir(&modules);
    authum_path_join(&m->dir, &modules, m->name);
    authum_path_ensure_dir(&m->dir);
    memset(&m->env, 0, sizeof m->env);
    m->env.authum = &m->root;
    m->env.module = &m->dir;

    m->sandbox.ops = authum_sandbox_host_ops();
    m->sandbox.root = authum_sandbox_host_root_create();
    m->events.ops = authum_events_host_ops();
    m->events.owner = m;
    m->commands.ops = authum_commands_host_ops();
    m->commands.owner = m;
    m->commands.scope = m->info->name;
    m->logger.ops = authum_logs_ops();
    m->logger.owner = m;
    authum_logs_open(m, m->info->name);

    memset(&m->host, 0, sizeof m->host);
    m->host.size = sizeof m->host;
    m->host.logger = &m->logger;
    m->host.env = &m->env;
    m->host.engine = &g_engine;
    m->host.sandbox = &m->sandbox;
    m->host.events = &m->events;
    m->host.commands = &m->commands;

    m->started = m->api->start(&m->host);
    return m->started;
}

void
test_unload(test_module_t *m)
{
    if (m->started && m->api->stop != 0)
    {
        m->api->stop();
    }
    authum_commands_host_drop_owner(m);
    authum_events_host_drop_owner(m);
    authum_logs_close(m);
    if (m->sandbox.root != authum_null)
    {
        authum_sandbox_host_root_destroy(m->sandbox.root);
    }
    if (m->library != authum_null)
    {
        test_dlclose(m->library);
    }
}

/* --- scenario ------------------------------------------------------------- */

void
test_print_out(authum_ptr out, const authum_char_t *text)
{
    (void)out;
    test_append(g_out, text);
    test_append(g_out, "\n");
}

/** `authum <scope> <name> [args]`; output in g_out. */
authum_bool_t
test_run(const char *scope, const char *name, authum_usize_t argc, const authum_char_t *const *argv)
{
    g_out[0] = '\0';
    return authum_commands_host_run(scope, name, argc, argv, test_print_out, authum_null);
}

void
test_client_event(const char *event, authum_i32_t index)
{
    authum_event_client_t ev;
    test_player_t *player = test_player(index);

    memset(&ev, 0, sizeof ev);
    ev.size = sizeof ev;
    ev.allow = authum_bool_true;
    ev.index = index;
    ev.userid = player->userid;
    ev.game_time = 10.0f;
    ev.unix_time = 1792929600;
    authum_cstr_copy(ev.name, sizeof ev.name, player->name);
    authum_cstr_copy(ev.auth_id, sizeof ev.auth_id, player->auth_id);
    authum_cstr_copy(ev.address, sizeof ev.address, player->address);
    (void)authum_events_host_publish(event, &ev);
}

/** A chat line @p player types; returns whether something blocked it. */
authum_bool_t
test_say(authum_i32_t index, const char *text)
{
    authum_event_client_command_t ev;
    test_player_t *player = test_player(index);

    memset(&ev, 0, sizeof ev);
    ev.size = sizeof ev;
    ev.client.size = sizeof ev.client;
    ev.client.index = index;
    ev.client.userid = player->userid;
    authum_cstr_copy(ev.client.name, sizeof ev.client.name, player->name);
    authum_cstr_copy(ev.client.auth_id, sizeof ev.client.auth_id, player->auth_id);
    ev.argc = 2;
    authum_cstr_copy(ev.argv[0], sizeof ev.argv[0], "say");
    authum_cstr_copy(ev.argv[1], sizeof ev.argv[1], text);
    authum_cstr_copy(ev.args, sizeof ev.args, text);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_COMMAND, &ev);
    return ev.block;
}

void
test_cvar_answer(authum_i32_t index, authum_i32_t request, const char *cvar, const char *value)
{
    authum_event_client_cvar_t ev;

    memset(&ev, 0, sizeof ev);
    ev.size = sizeof ev;
    ev.client.size = sizeof ev.client;
    ev.client.index = index;
    ev.client.userid = test_player(index)->userid;
    authum_cstr_copy(ev.client.name, sizeof ev.client.name, test_player(index)->name);
    ev.request_id = request;
    authum_cstr_copy(ev.cvar, sizeof ev.cvar, cvar);
    authum_cstr_copy(ev.value, sizeof ev.value, value);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_CVAR_VALUE, &ev);
}

void
test_write_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");

    CHECK(file != NULL);
    if (file != NULL)
    {
        fputs(text, file);
        fclose(file);
    }
}

test_module_t g_modules[] = {
    {"skeleton"}, {"hello"}, {"welcome"}, {"chatfilter"}, {"cvarguard"},
    {"settings"}, {"stats"}, {"scores"},  {"scoreboard"},
};

int
main(void)
{
    const authum_char_t *add_args[] = {"1", "5"};
    const authum_char_t *show_args[] = {"1"};
    authum_path_t root;
    authum_usize_t i;
    authum_bool_t loaded = authum_bool_true;

    authum_path_set_cstr(&root, TEST_ROOT);
    authum_path_ensure_dir(&root);
    remove(TEST_ROOT "/modules/stats/stats.db");
    remove(TEST_ROOT "/modules/settings/settings.cfg");
    CHECK(authum_logs_init(TEST_ROOT, authum_null));

    for (i = 0u; i < sizeof g_modules / sizeof g_modules[0]; ++i)
    {
        if (!test_load(&g_modules[i]))
        {
            fprintf(stderr, "%s did not start\n", g_modules[i].name);
            loaded = authum_bool_false;
        }
    }
    CHECK(loaded);

    /* A player connects and enters. */
    test_client_event(AUTHUM_EVENT_CLIENT_CONNECT, 1);
    test_client_event(AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, 1);
    CHECK(strstr(g_prints, "1|Welcome, Vega! You are playing de_dust2.") != NULL); /* welcome */
    CHECK(strstr(g_prints, "1|Visit #1. Welcome back!") != NULL);                  /* stats */
    CHECK(g_query_index == 1 && g_query_request == 11);                            /* cvarguard */
    CHECK_STR(g_query_cvar, "fps_max");

    /* The same player comes back: the visit count grows. */
    test_client_event(AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, 1);
    CHECK(strstr(g_prints, "1|Visit #2.") != NULL);

    /* Bots are not counted. */
    g_prints[0] = '\0';
    test_client_event(AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, 2);
    CHECK(strstr(g_prints, "Visit") == NULL);

    /* cvarguard: a fine value, a stale answer, a value over the limit. */
    test_cvar_answer(1, 11, "fps_max", "100");
    CHECK_STR(g_commands, "");
    test_cvar_answer(1, 99, "fps_max", "1000");
    CHECK_STR(g_commands, "");
    test_cvar_answer(1, 11, "fps_max", "1000");
    CHECK(strstr(g_commands, "kick #11 ") == g_commands);

    /* chatfilter */
    g_prints[0] = '\0';
    CHECK(test_say(1, "who uses an AIMBOT here"));
    CHECK(strstr(g_prints, "1|Your message was not sent") != NULL);
    CHECK(!test_say(1, "good game"));

    /* scores and scoreboard talk over the bus. */
    g_prints[0] = '\0';
    CHECK(test_run("scores", "add", 2u, add_args));
    CHECK(strstr(g_out, "#1 now has 5") != NULL);
    CHECK(strstr(g_prints, "0|Vega: +5, now 5 points") != NULL);
    CHECK(test_run("scoreboard", "show", 1u, show_args));
    CHECK(strstr(g_out, "#1 has 5 points") != NULL);

    /* settings: defaults, then a file, then a bad value that is refused. */
    CHECK(test_run("settings", "show", 0u, authum_null));
    CHECK(strstr(g_out, "mode         = normal") != NULL);
    test_write_file(TEST_ROOT "/modules/settings/settings.cfg",
                    "greeting = hi there\nmax_warnings = 7\nmode = HARD\n");
    CHECK(test_run("settings", "reload", 0u, authum_null));
    CHECK(test_run("settings", "show", 0u, authum_null));
    CHECK(strstr(g_out, "mode         = hard") != NULL);
    CHECK(strstr(g_out, "max_warnings = 7") != NULL);
    CHECK(strstr(g_out, "greeting     = hi there") != NULL);
    test_write_file(TEST_ROOT "/modules/settings/settings.cfg", "mode = impossible\n");
    CHECK(test_run("settings", "reload", 0u, authum_null));
    CHECK(test_run("settings", "show", 0u, authum_null));
    CHECK(strstr(g_out, "mode         = normal") != NULL); /* defaults again, bad mode ignored */

    /* Lists. */
    CHECK(test_run("welcome", "players", 0u, authum_null));
    CHECK(strstr(g_out, "Vega") != NULL && strstr(g_out, "3 of 4 slots in use") != NULL);
    CHECK(test_run("hello", "seen", 0u, authum_null));
    CHECK(strstr(g_out, "Vega") != NULL);
    CHECK(test_run("stats", "top", 0u, authum_null));
    CHECK(strstr(g_out, "Vega") != NULL && strstr(g_out, "STEAM_0:1:111") != NULL);

    /* The player leaves: scores forgets the slot. */
    test_client_event(AUTHUM_EVENT_CLIENT_DISCONNECT, 1);
    CHECK(test_run("scoreboard", "show", 1u, show_args));
    CHECK(strstr(g_out, "#1 has 0 points") != NULL);

    for (i = sizeof g_modules / sizeof g_modules[0]; i > 0u; --i)
    {
        test_unload(&g_modules[i - 1u]);
    }
    CHECK(authum_events_host_count() == 0u);
    CHECK(authum_commands_host_count() == 0u);

    /* With scores gone, the journal and logs are on disk. */
    authum_logs_deinit();
    return authum_check_report("examples");
}
