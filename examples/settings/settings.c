/**
 * settings — a module's own config file.
 *
 * addons/authum/modules/settings/configs/settings.cfg, flat `key = value` (found with
 * authum_host_config_path, which also reads an old modules/settings/ copy):
 *   - defaults live in the C struct, so a missing file or key is fine;
 *   - plain keys come from one table (authum_cfg_apply_items);
 *   - a key with its own rules (`mode`, one of three words) is read after the
 *     table and checked by hand;
 *   - `authum settings show` prints what is in effect, `authum settings
 *     reload` reads the file again without reloading the module.
 *
 * Logging levels are not module settings: they live in addons/authum/configs/authum.cfg.
 */

#include <authum/client.h>

#include <string.h>

typedef struct settings_config
{
    authum_bool_t enabled;
    authum_u32_t max_warnings;
    authum_char_t greeting[64];
    authum_char_t mode[16];
} settings_config_t;

/* The defaults: what the module does with no settings.cfg at all. */
const settings_config_t g_settings_defaults = {
    .enabled = authum_bool_true,
    .max_warnings = 3u,
    .greeting = "Have fun!",
    .mode = "normal",
};

authum_host_t g_settings_host;
authum_logger_t g_settings_log;
settings_config_t g_settings;

const authum_cfg_item_t g_settings_items[] = {
    {"enabled", authum_cfg_kind_bool, &g_settings.enabled, 0},
    {"max_warnings", authum_cfg_kind_u32, &g_settings.max_warnings, 0},
    {"greeting", authum_cfg_kind_cstr, g_settings.greeting, sizeof g_settings.greeting},
};

const authum_char_t *const g_settings_modes[] = {"easy", "normal", "hard"};

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "settings",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .logtag = "SETTINGS",
};

/** `mode` is one of g_settings_modes; anything else keeps the current value. */
void
settings_apply_mode(authum_cfg_t *cfg)
{
    const authum_char_t *value = authum_cfg_get_value(cfg, "mode");
    authum_usize_t i;

    if (authum_null_eq(value))
    {
        return;
    }
    for (i = 0u; i < sizeof g_settings_modes / sizeof g_settings_modes[0]; ++i)
    {
        if (authum_cstr_eq(value, g_settings_modes[i], authum_bool_true))
        {
            authum_cstr_copy(g_settings.mode, sizeof g_settings.mode, g_settings_modes[i]);
            return;
        }
    }
    authum_logger_warning(&g_settings_log, "mode \"%s\" is not easy / normal / hard; keeping %s",
                          value, g_settings.mode);
}

/** Defaults, then whatever settings.cfg says. False only when the file is missing. */
authum_bool_t
settings_load(void)
{
    authum_path_t path;
    authum_cfg_t cfg;

    g_settings = g_settings_defaults;
    /* modules/settings/configs/settings.cfg */
    if (!authum_host_config_path(&g_settings_host, "settings.cfg", &path) ||
        !authum_cfg_open(&cfg, &path))
    {
        authum_logger_notice(&g_settings_log, "no settings.cfg, using defaults");
        return authum_bool_false;
    }
    authum_cfg_apply_items(&cfg, g_settings_items,
                           sizeof g_settings_items / sizeof g_settings_items[0]);
    settings_apply_mode(&cfg);
    authum_cfg_close(&cfg);
    authum_logger_info(&g_settings_log, "loaded %s", authum_path_as_cstr(&path));
    return authum_bool_true;
}

/* authum settings show */
authum_bool_t
settings_cmd_show(const authum_command_t *cmd, authum_ptr user)
{
    authum_unused(user);
    authum_command_printf(cmd, "enabled      = %s", g_settings.enabled ? "true" : "false");
    authum_command_printf(cmd, "max_warnings = %u", g_settings.max_warnings);
    authum_command_printf(cmd, "greeting     = %s", g_settings.greeting);
    authum_command_printf(cmd, "mode         = %s", g_settings.mode);
    return authum_bool_true;
}

/* authum settings reload */
authum_bool_t
settings_cmd_reload(const authum_command_t *cmd, authum_ptr user)
{
    authum_unused(user);
    authum_command_print(cmd, settings_load() ? "settings.cfg read again"
                                              : "no settings.cfg, defaults in effect");
    return authum_bool_true;
}

const authum_command_def_t g_settings_commands[] = {
    {"show", authum_null, "settings in effect", settings_cmd_show, authum_null},
    {"reload", authum_null, "read settings.cfg again", settings_cmd_reload, authum_null},
};

authum_bool_t
settings_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_settings_host, host);
    authum_logger_init(&g_settings_log, &g_settings_host);
    (void)settings_load(); /* a missing file is not a reason to refuse loading */
    return authum_host_command_add_all(&g_settings_host, g_settings_commands,
                                       sizeof g_settings_commands / sizeof g_settings_commands[0]);
}

void
settings_stop(void)
{
    authum_logger_close(&g_settings_log);
    authum_host_clear(&g_settings_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = settings_start,
    .stop = settings_stop,
};
