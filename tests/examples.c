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

/* Chat through SayText: "chat|<index>|<text>" lines in g_prints. */
authum_i32_t g_msg_index;
authum_i32_t g_msg_id;
authum_char_t g_msg_text[192];

authum_i32_t
test_message_id(const authum_char_t *name)
{
    return strcmp(name, "SayText") == 0 ? 76 : 0;
}

authum_bool_t
test_message_begin(authum_i32_t dest, authum_i32_t msg_id, authum_i32_t index)
{
    (void)dest;
    g_msg_id = msg_id;
    g_msg_index = index;
    g_msg_text[0] = '\0';
    return authum_bool_true;
}

void
test_message_write(authum_i32_t kind, authum_i32_t value, authum_float_t real,
                   const authum_char_t *text)
{
    (void)value;
    (void)real;
    if (kind == authum_message_arg_string)
    {
        authum_cstr_copy(g_msg_text, sizeof g_msg_text, text);
    }
}

void
test_message_end(void)
{
    authum_char_t line[256];

    if (g_msg_id == 76)
    {
        snprintf(line, sizeof line, "chat|%d|%s\n", (int)g_msg_index, g_msg_text);
        test_append(g_prints, line);
    }
}

/* Server cvars: a small table. */
enum
{
    test_cvars_max = 16
};

authum_char_t g_cvar_names[test_cvars_max][64];
authum_char_t g_cvar_values[test_cvars_max][64];
int g_cvar_count;

int
test_cvar_find(const authum_char_t *name)
{
    int i;

    for (i = 0; i < g_cvar_count; ++i)
    {
        if (strcmp(g_cvar_names[i], name) == 0)
        {
            return i;
        }
    }
    return -1;
}

authum_bool_t
test_cvar_register(const authum_char_t *name, const authum_char_t *value, authum_i32_t flags)
{
    (void)flags;
    if (test_cvar_find(name) >= 0)
    {
        return authum_bool_true;
    }
    if (g_cvar_count >= test_cvars_max)
    {
        return authum_bool_false;
    }
    authum_cstr_copy(g_cvar_names[g_cvar_count], sizeof g_cvar_names[0], name);
    authum_cstr_copy(g_cvar_values[g_cvar_count], sizeof g_cvar_values[0], value);
    ++g_cvar_count;
    return authum_bool_true;
}

authum_bool_t
test_cvar_get(const authum_char_t *name, authum_char_t *buf, authum_usize_t size)
{
    int i = test_cvar_find(name);

    if (i < 0)
    {
        return authum_bool_false;
    }
    authum_cstr_copy(buf, size, g_cvar_values[i]);
    return authum_bool_true;
}

authum_bool_t
test_cvar_set(const authum_char_t *name, const authum_char_t *value)
{
    int i = test_cvar_find(name);

    if (i < 0)
    {
        return authum_bool_false;
    }
    authum_cstr_copy(g_cvar_values[i], sizeof g_cvar_values[0], value);
    return authum_bool_true;
}

authum_char_t g_precached[128];

authum_i32_t
test_precache_sound(const authum_char_t *path)
{
    test_append(g_precached, path);
    return 1;
}

/* Vega 100 hp, the bot dead, Gordon 50 hp. */
authum_float_t g_health[test_max_clients + 1] = {0.0f, 100.0f, 0.0f, 50.0f, 0.0f};

authum_bool_t
test_entity_get_float(authum_i32_t index, authum_i32_t field, authum_float_t *out)
{
    if (test_player(index) == authum_null)
    {
        return authum_bool_false;
    }
    *out = field == authum_entity_float_health ? g_health[index] : 0.0f;
    return authum_bool_true;
}

authum_bool_t
test_entity_get_i32(authum_i32_t index, authum_i32_t field, authum_i32_t *out)
{
    if (test_player(index) == authum_null)
    {
        return authum_bool_false;
    }
    if (field == authum_entity_i32_dead)
    {
        *out = g_health[index] > 0.0f ? authum_entity_dead_no : authum_entity_dead_dead;
    }
    else if (field == authum_entity_i32_flags)
    {
        *out =
            authum_entity_flag_client |
            (strcmp(test_player(index)->auth_id, "BOT") == 0 ? authum_entity_flag_fake_client : 0);
    }
    else
    {
        *out = 0;
    }
    return authum_bool_true;
}

authum_bool_t
test_client_command(authum_i32_t index, const authum_char_t *cmd)
{
    authum_char_t line[192];

    if (test_player(index) == authum_null)
    {
        return authum_bool_false;
    }
    snprintf(line, sizeof line, "cmd|%d|%s\n", (int)index, cmd);
    test_append(g_commands, line);
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
    .cvar_get = test_cvar_get,
    .cvar_set = test_cvar_set,
    .cvar_register = test_cvar_register,
    .message_id = test_message_id,
    .message_begin = test_message_begin,
    .message_write = test_message_write,
    .message_end = test_message_end,
    .precache_sound = test_precache_sound,
    .entity_get_float = test_entity_get_float,
    .entity_get_i32 = test_entity_get_i32,
    .client_command = test_client_command,
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
    authum_path_t modules;
    authum_path_t module;
    authum_path_t module_config;
    authum_path_t module_lang;
    authum_path_t module_data;
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
    memset(&m->env, 0, sizeof m->env);
    m->env.size = sizeof m->env;
    m->env.authum = &m->root;
    authum_path_assign(&m->modules, &modules);
    m->env.modules = &m->modules;
    authum_path_join(&m->module, &modules, m->name);
    authum_path_ensure_dir(&m->module);
    m->env.module = &m->module;
    authum_path_join(&m->module_config, &m->module, "configs");
    authum_path_ensure_dir(&m->module_config);
    m->env.module_config = &m->module_config;
    authum_path_join(&m->module_lang, &m->module, "lang");
    m->env.module_lang = &m->module_lang;
    authum_path_join(&m->module_data, &m->module, "data");
    m->env.module_data = &m->module_data;

    m->sandbox.ops = authum_sandbox_host_ops();
    m->sandbox.root = authum_sandbox_host_root_create();
    m->events.ops = authum_events_host_ops();
    m->events.owner = m;
    m->commands.ops = authum_commands_host_ops();
    m->commands.owner = m;
    m->commands.scope = m->info->name;
    m->logger.ops = authum_logs_ops();
    m->logger.owner = m;
    authum_logs_open(m, m->info->name, authum_path_as_cstr(&m->module));

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

authum_bool_t
test_exists(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        return authum_bool_false;
    }
    fclose(file);
    return authum_bool_true;
}

test_module_t g_modules[] = {
    {"skeleton"}, {"hello"},  {"welcome"},    {"chatfilter"},  {"cvarguard"}, {"settings"},
    {"stats"},    {"scores"}, {"scoreboard"}, {"deathnotice"}, {"announcer"},
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
    remove(TEST_ROOT "/modules/stats/data/stats.db");
    remove(TEST_ROOT "/modules/settings/configs/settings.cfg");
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
    test_write_file(TEST_ROOT "/modules/settings/configs/settings.cfg",
                    "greeting = hi there\nmax_warnings = 7\nmode = HARD\n");
    CHECK(test_run("settings", "reload", 0u, authum_null));
    CHECK(test_run("settings", "show", 0u, authum_null));
    CHECK(strstr(g_out, "mode         = hard") != NULL);
    CHECK(strstr(g_out, "max_warnings = 7") != NULL);
    CHECK(strstr(g_out, "greeting     = hi there") != NULL);
    test_write_file(TEST_ROOT "/modules/settings/configs/settings.cfg", "mode = impossible\n");
    CHECK(test_run("settings", "reload", 0u, authum_null));
    CHECK(test_run("settings", "show", 0u, authum_null));
    CHECK(strstr(g_out, "mode         = normal") != NULL); /* defaults again, bad mode ignored */

    /* stats keeps its database under data/stats/. */
    CHECK(test_exists(TEST_ROOT "/modules/stats/data/stats.db"));

    /* Lists. */
    CHECK(test_run("welcome", "players", 0u, authum_null));
    CHECK(strstr(g_out, "Vega") != NULL && strstr(g_out, "3 of 4 slots in use") != NULL);
    CHECK(test_run("hello", "seen", 0u, authum_null));
    CHECK(strstr(g_out, "Vega") != NULL);
    CHECK(test_run("stats", "top", 0u, authum_null));
    CHECK(strstr(g_out, "Vega") != NULL && strstr(g_out, "STEAM_0:1:111") != NULL);

    /* deathnotice: the game's DeathMsg becomes a colored chat line. */
    {
        authum_event_message_t death;

        memset(&death, 0, sizeof death);
        death.size = sizeof death;
        authum_cstr_copy(death.name, sizeof death.name, "DeathMsg");
        death.argc = 4;
        death.args[0].kind = authum_message_arg_byte;
        death.args[0].value = 1;
        death.args[1].kind = authum_message_arg_byte;
        death.args[1].value = 3;
        death.args[2].kind = authum_message_arg_byte;
        death.args[2].value = 1;
        death.args[3].kind = authum_message_arg_string;
        memcpy(death.text, "ak47", 5);
        death.text_used = 5u;
        g_prints[0] = '\0';
        (void)authum_events_host_publish(AUTHUM_EVENT_MESSAGE_PREFIX "DeathMsg", &death);
        CHECK(strstr(g_prints,
                     "chat|0|\x04[Death]\x03 Vega\x01 killed Gordon with ak47 (headshot)") != NULL);
        CHECK(!death.block);
    }
    CHECK(test_cvar_find("deathnotice_enabled") >= 0);

    /* deathnotice: kill is allowed until its cvar says otherwise. */
    {
        authum_event_client_t kill;

        memset(&kill, 0, sizeof kill);
        kill.size = sizeof kill;
        kill.index = 1;
        kill.allow = authum_bool_true;
        (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_KILL, &kill);
        CHECK(kill.allow);
        CHECK(test_cvar_set("deathnotice_allow_kill", "0"));
        (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_KILL, &kill);
        CHECK(!kill.allow);
    }

    /* announcer: precache, the round start line, the timer, the hp list. */
    {
        authum_event_server_t server;
        authum_event_server_log_t line;
        int i;

        memset(&server, 0, sizeof server);
        server.size = sizeof server;
        (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_PRECACHE, &server);
        CHECK_STR(g_precached, "buttons/bell1.wav");

        memset(&line, 0, sizeof line);
        line.size = sizeof line;
        authum_cstr_copy(line.text, sizeof line.text, "World triggered \"Round_Start\"");
        g_commands[0] = '\0';
        (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_LOG, &line);
        CHECK(strstr(g_commands, "cmd|1|spk buttons/bell1.wav") != NULL);
        CHECK(strstr(g_commands, "cmd|2|") == NULL); /* the bot */
        CHECK(strstr(g_commands, "cmd|3|spk buttons/bell1.wav") != NULL);

        g_prints[0] = '\0';
        server.frame_time = 1.0f;
        for (i = 0; i < 119; ++i)
        {
            (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_FRAME, &server);
        }
        CHECK(strstr(g_prints, "Message #1") == NULL);
        (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_FRAME, &server); /* 120 s */
        CHECK(strstr(g_prints, "chat|0|\x04[Info]\x01 This server runs Authum. Message #1.") !=
              NULL);

        CHECK(test_run("announcer", "hp", 0u, authum_null));
        CHECK(strstr(g_out, "Vega") != NULL && strstr(g_out, "100 hp") != NULL);
        CHECK(strstr(g_out, "Gordon") != NULL && strstr(g_out, "Bot Alice") == NULL);
        CHECK(strstr(g_out, "2 alive") != NULL);
    }

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
