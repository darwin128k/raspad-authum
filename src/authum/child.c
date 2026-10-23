#include <authum/commands/host.h>
#include <authum/error/code.h>
#include <authum/events/host.h>
#include <authum/host.h>
#include <authum/logger/host.h>
#include <authum/sandbox/host.h>
#include <authum/module.h>
#include <authum/module/api.h>
#include <authum/path.h>
#include <authum/version.h>
#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/list.h>
#include <lh/null.h>
#include <lh/os.h>
#include <lh/os/error/code.h>
#include <lh/os/loader.h>
#include <lh/os/system/fs/path.h>
#include <lh/runtime/allocator.h>
#include <lh/str.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>

#include <string.h>

typedef struct authum_child_paths
{
    authum_path_t game;
    authum_path_t models;
    authum_path_t sprites;
    authum_path_t addons;
    authum_path_t authum;
    authum_path_t modules;
    authum_path_t module;
    authum_path_t configs;
    authum_path_t data;
    authum_path_t lang;
    authum_path_t module_config;
    authum_path_t module_data;
} authum_child_paths_t;

typedef struct authum_child_data
{
    const authum_module_info_t *info;
    const authum_module_api_t *api;
    authum_host_t host;
    authum_env_t env;
    authum_child_paths_t paths;
    authum_sandbox_t sandbox;
    authum_events_t events;
    authum_commands_t commands;
    authum_logger_channel_t logger;
} authum_child_data_t;

const authum_module_info_t *
authum_child_peek_info(const void *data)
{
    if (lh_null_eq(data))
    {
        return lh_null;
    }
    return lh_ptr_ccast(authum_child_data_t, data)->info;
}

void
authum_child_bind_path(authum_path_t *slot, const authum_path_t **field, const char *text)
{
    if (authum_path_set_cstr(slot, text) && !authum_path_is_empty(slot))
    {
        *field = slot;
    }
    else
    {
        authum_path_clear(slot);
        *field = lh_null;
    }
}

void
authum_child_bind_lh_path(authum_path_t *slot, const authum_path_t **field,
                          const lh_fs_path_t *path, lh_fs_path_style_t style)
{
    lh_str_t scratch;

    if (lh_fs_path_is_empty(path))
    {
        authum_path_clear(slot);
        *field = lh_null;
        return;
    }
    authum_child_bind_path(slot, field, lh_fs_path_to_cstr(path, style, lh_addr_of(scratch)));
    lh_str_deinit(lh_addr_of(scratch));
}

void
authum_child_bind_join(authum_path_t *slot, const authum_path_t **field, const authum_path_t *dir,
                       const authum_char_t *leaf)
{
    if (authum_null_eq(dir) || authum_path_is_empty(dir))
    {
        authum_path_clear(slot);
        *field = lh_null;
        return;
    }
    if (authum_path_join(slot, dir, leaf) && !authum_path_is_empty(slot))
    {
        *field = slot;
    }
    else
    {
        authum_path_clear(slot);
        *field = lh_null;
    }
}

void
authum_child_fill_under_game(authum_child_data_t *data)
{
    authum_env_t *env = &data->env;
    authum_child_paths_t *paths = &data->paths;

    if (lh_null_eq(env->game))
    {
        return;
    }

    if (lh_null_eq(env->models))
    {
        authum_child_bind_join(&paths->models, &env->models, env->game, "models");
    }
    if (lh_null_eq(env->sprites))
    {
        authum_child_bind_join(&paths->sprites, &env->sprites, env->game, "sprites");
    }
    if (lh_null_eq(env->addons))
    {
        authum_child_bind_join(&paths->addons, &env->addons, env->game, "addons");
    }
    if (lh_null_eq(env->authum) && !lh_null_eq(env->addons))
    {
        authum_child_bind_join(&paths->authum, &env->authum, env->addons, "authum");
    }
    if (lh_null_eq(env->modules) && !lh_null_eq(env->authum))
    {
        authum_child_bind_join(&paths->modules, &env->modules, env->authum, "modules");
    }
}

/** Bind @p field to the directory holding @p image (an lh path), or leave it null. */
void
authum_child_bind_parent(authum_path_t *slot, const authum_path_t **field,
                         const lh_fs_path_t *image)
{
    lh_fs_path_t parent;

    lh_fs_path_init(lh_addr_of(parent));
    if (!lh_fs_path_is_empty(image) && lh_fs_path_parent(lh_addr_of(parent), image))
    {
        authum_child_bind_lh_path(slot, field, lh_addr_of(parent),
                                  lh_os_system_fs_path_style_native());
    }
    lh_fs_path_deinit(lh_addr_of(parent));
}

/**
 * Roots come from authum_mm's own directory (addons/authum), so a module in
 * modules/qproto.so and one in the old modules/qproto/qproto.so see the same
 * configs/, data/ and logs/. `module` stays the directory of the module's
 * image. The engine game dir is a fallback when the walk cannot reach game.
 */
void
authum_child_fill_env(authum_module_t *child, authum_child_data_t *data)
{
    authum_env_t *env = &data->env;
    authum_child_paths_t *paths = &data->paths;
    authum_path_t up;
    const authum_char_t *name = data->info->name;

    memset(env, 0, sizeof(*env));
    memset(paths, 0, sizeof(*paths));
    env->size = sizeof(*env);
    data->host.env = env;
    data->host.module_dir = lh_null;

    authum_child_bind_parent(&paths->module, &env->module, lh_os_module_get_path(child));
    authum_child_bind_parent(&paths->authum, &env->authum, lh_os_module_get_path(&authum));
    if (!lh_null_eq(env->authum) && authum_path_parent(&up, env->authum))
    {
        authum_child_bind_path(&paths->addons, &env->addons, authum_path_as_cstr(&up));
        if (authum_path_parent(&up, env->addons))
        {
            authum_child_bind_path(&paths->game, &env->game, authum_path_as_cstr(&up));
        }
    }

    if (lh_null_eq(env->game) && !lh_null_eq(data->host.engine))
    {
        authum_char_t buf[authum_path_capacity];

        memset(buf, 0, sizeof(buf));
        authum_engine_get_game_dir(data->host.engine, buf, (authum_usize_t)sizeof(buf));
        authum_child_bind_path(&paths->game, &env->game, buf);
    }

    authum_child_fill_under_game(data);

    authum_child_bind_join(&paths->configs, &env->configs, env->authum, "configs");
    authum_child_bind_join(&paths->data, &env->data, env->authum, "data");
    authum_child_bind_join(&paths->lang, &env->lang, env->data, "lang");
    authum_child_bind_join(&paths->module_config, &env->module_config, env->configs, name);
    authum_child_bind_join(&paths->module_data, &env->module_data, env->data, name);
    /* What the module writes always has a place; configs are the admin's. */
    if (!lh_null_eq(env->module_data))
    {
        (void)authum_path_ensure_dir(env->data);
        (void)authum_path_ensure_dir(env->module_data);
    }

    data->host.module_dir = authum_path_as_cstr_or_null(env->module);
}

/* Implemented in crypto.c / engine_server.cpp (linked into authum_mm). */
const authum_crypto_t *
authum_crypto(void);
const authum_engine_t *
authum_engine(void);

void
authum_child_fill_host(authum_module_t *child, authum_child_data_t *data)
{
    authum_module_t *parent;
    authum_host_t *out = &data->host;

    out->size = sizeof(*out);
    out->engine_funcs = lh_null;
    out->globals = lh_null;
    out->env = lh_null;
    out->module_dir = lh_null;
    out->crypto = authum_crypto();
    out->engine = authum_engine();

    data->sandbox.ops = authum_sandbox_host_ops();
    data->sandbox.root = authum_sandbox_host_root_create();
    out->sandbox = &data->sandbox;

    data->events.ops = authum_events_host_ops();
    data->events.owner = data;
    out->events = &data->events;

    data->logger.ops = authum_logs_ops();
    data->logger.owner = data;
    (void)authum_logs_open(data, data->info->name);
    out->logger = &data->logger;

    data->commands.ops = authum_commands_host_ops();
    data->commands.owner = data;
    data->commands.scope = data->info->name;
    out->commands = &data->commands;

    parent = lh_os_module_get_parent(child);
    if (!lh_null_eq(parent))
    {
        out->engine_funcs =
            (authum_host_get_ptr_fn)lh_os_module_get_sym(parent, "authum_engine_funcs");
        out->globals = (authum_host_get_ptr_fn)lh_os_module_get_sym(parent, "authum_globals");
    }

    authum_child_fill_env(child, data);
}

/** After `stop` (or a failed `start`): drop the module's subscriptions and
 *  commands, free its sandbox. */
void
authum_child_release(authum_child_data_t *data)
{
    authum_commands_host_drop_owner(data);
    authum_events_host_drop_owner(data);
    authum_logs_close(data);
    authum_sandbox_host_root_destroy(data->sandbox.root);
    lh_runtime_allocator_free(data);
}

lh_bool_t
authum_child_start(authum_module_t *self)
{
    const authum_version_t required = AUTHUM_MODULE_ABI;
    authum_child_data_t *data;
    lh_ptr sym_info;
    lh_ptr sym_api;

    sym_info = lh_os_module_get_sym(self, AUTHUM_MODULE_INFO_SYM);
    sym_api = lh_os_module_get_sym(self, AUTHUM_MODULE_API_SYM);
    if (lh_null_eq(sym_info) || lh_null_eq(sym_api))
    {
        lh_os_set_last_error_lit(authum_error_code_missing_info, "missing authum_module_info/api");
        return lh_bool_false;
    }

    data = lh_ptr_cast(authum_child_data_t, lh_runtime_allocator_alloc(sizeof(*data)));
    if (lh_null_eq(data))
    {
        lh_os_set_last_error_lit(lh_os_error_code_out_of_memory, "child data");
        return lh_bool_false;
    }
    memset(data, 0, sizeof(*data));

    data->info = lh_ptr_ccast(authum_module_info_t, sym_info);
    data->api = lh_ptr_ccast(authum_module_api_t, sym_api);
    if (lh_null_eq(data->info->name) || lh_null_eq(data->api->start))
    {
        lh_runtime_allocator_free(data);
        lh_os_set_last_error_lit(authum_error_code_invalid_info, "invalid module info/api");
        return lh_bool_false;
    }
    if (!authum_version_is_abi_compatible(&data->info->abi, &required))
    {
        lh_runtime_allocator_free(data);
        lh_os_set_last_error_lit(authum_error_code_incompatible_abi, "incompatible module abi");
        return lh_bool_false;
    }

    authum_child_fill_host(self, data);
    lh_os_module_set_data(self, data);

    if (!data->api->start(&data->host))
    {
        lh_os_module_set_data(self, lh_null);
        authum_child_release(data);
        return lh_bool_false;
    }
    return lh_bool_true;
}

void
authum_child_stop(authum_module_t *self)
{
    authum_child_data_t *data;

    data = lh_ptr_cast(authum_child_data_t, lh_os_module_get_data(self));
    if (lh_null_eq(data))
    {
        return;
    }
    if (!lh_null_eq(data->api) && !lh_null_eq(data->api->stop))
    {
        data->api->stop();
    }
    lh_os_module_set_data(self, lh_null);
    authum_child_release(data);
}

static const lh_os_module_ops_t authum_child_ops = {
    .start = authum_child_start,
    .stop = authum_child_stop,
};

lh_os_module_t *
authum_child_load(lh_os_loader_t *loader, const lh_fs_path_t *path)
{
    lh_os_module_t *child;
    const lh_os_module_ops_t *ops;

    child = lh_os_module_create();
    if (lh_null_eq(child))
    {
        return lh_null;
    }

    lh_os_module_set_owner(child, loader);
    if (!lh_os_module_open(child, path))
    {
        lh_os_module_destroy(child);
        return lh_null;
    }

    if (!lh_null_eq(lh_os_module_get_sym(child, AUTHUM_MODULE_API_SYM)))
    {
        lh_os_module_set_ops(child, &authum_child_ops);
    }
    else
    {
        ops = lh_ptr_cast(const lh_os_module_ops_t,
                          lh_os_module_get_sym(child, AUTHUM_MODULE_OPS_SYM));
        if (lh_null_eq(ops))
        {
            lh_os_module_destroy(child);
            return lh_null;
        }
        lh_os_module_set_ops(child, ops);
    }

    if (!lh_os_module_start(child))
    {
        lh_os_module_destroy(child);
        return lh_null;
    }

    lh_list_push_back(lh_os_loader_get_modules_mut(loader), lh_os_module_get_node(child));
    return child;
}
