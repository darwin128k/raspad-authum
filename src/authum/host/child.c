#include <authum/host/commands.h>
#include <authum/memory.h>
#include <authum/unused.h>
#include <lh/memory/std.h>
#include <authum/host/errors.h>
#include <authum/host/events.h>
#include <authum/host/logs.h>
#include <authum/host/sandbox.h>
#include <authum/host/module.h>
#include <authum/host/tasks.h>
#include <authum/host/threads.h>
#include <authum/host/http.h>
#include <authum/host/clock.h>
#include <authum/host/placeholders.h>
#include <authum/module/api.h>
#include <lh/os.h>
#include <lh/os/error/code.h>
#include <lh/os/system/fs/path.h>
#include <lh/runtime/allocator.h>

typedef struct authum_child_paths
{
    authum_path_t game;
    authum_path_t models;
    authum_path_t sprites;
    authum_path_t addons;
    authum_path_t authum;
    authum_path_t modules;
    authum_path_t module;
    authum_path_t module_config;
    authum_path_t module_lang;
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
    authum_tasks_t tasks;
    authum_threads_t threads;
    authum_thread_owner_t thread_owner;
    authum_http_owner_t http_owner;
    authum_http_service_t http;
    authum_placeholder_owner_t placeholders;
    authum_placeholder_service_t placeholder_service;
    authum_logger_channel_t logger;
} authum_child_data_t;

/** @brief Level-1 mutable event facade member; data must be nonnull. */
authum_events_t *
authum_child_data_get_events_mut(authum_child_data_t *data)
{
    return lh_addr_of(data->events);
}

const authum_module_info_t *
authum_child_peek_info(authum_cptr data)
{
    if (lh_null_eq(data))
    {
        return lh_null;
    }
    return lh_ptr_ccast(authum_child_data_t, data)->info;
}

void
authum_child_bind_path(authum_path_t *slot, const authum_path_t **field, const authum_char_t *text)
{
    if (authum_path_set_cstr(slot, text) && !authum_path_is_empty(slot))
    {
        lh_ptr_deref(field) = slot;
    }
    else
    {
        authum_path_clear(slot);
        lh_ptr_deref(field) = lh_null;
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
        lh_ptr_deref(field) = lh_null;
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
        lh_ptr_deref(field) = lh_null;
        return;
    }
    if (authum_path_join(slot, dir, leaf) && !authum_path_is_empty(slot))
    {
        lh_ptr_deref(field) = slot;
    }
    else
    {
        authum_path_clear(slot);
        lh_ptr_deref(field) = lh_null;
    }
}

void
authum_child_fill_under_game(authum_child_data_t *data)
{
    authum_env_t *env = lh_addr_of(data->env);
    authum_child_paths_t *paths = lh_addr_of(data->paths);

    if (lh_null_eq(env->game))
    {
        return;
    }

    if (lh_null_eq(env->models))
    {
        authum_child_bind_join(lh_addr_of(paths->models), lh_addr_of(env->models), env->game,
                               "models");
    }
    if (lh_null_eq(env->sprites))
    {
        authum_child_bind_join(lh_addr_of(paths->sprites), lh_addr_of(env->sprites), env->game,
                               "sprites");
    }
    if (lh_null_eq(env->addons))
    {
        authum_child_bind_join(lh_addr_of(paths->addons), lh_addr_of(env->addons), env->game,
                               "addons");
    }
    if (lh_null_eq(env->authum) && !lh_null_eq(env->addons))
    {
        authum_child_bind_join(lh_addr_of(paths->authum), lh_addr_of(env->authum), env->addons,
                               "authum");
    }
    if (lh_null_eq(env->modules) && !lh_null_eq(env->authum))
    {
        authum_child_bind_join(lh_addr_of(paths->modules), lh_addr_of(env->modules), env->authum,
                               "modules");
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
 * Roots come from authum's own directory (addons/authum), game two levels
 * up; the engine game dir is a fallback. The module's own folder is the one
 * its image is in (modules/<module>/), with configs/, lang/ and data/ inside.
 */
void
authum_child_fill_env(authum_module_t *child, authum_child_data_t *data)
{
    authum_env_t *env = lh_addr_of(data->env);
    authum_child_paths_t *paths = lh_addr_of(data->paths);
    authum_path_t up;

    lh_memory_std_set(env, 0, sizeof(lh_ptr_deref(env)));
    lh_memory_std_set(paths, 0, sizeof(lh_ptr_deref(paths)));
    data->host.env = env;

    authum_child_bind_parent(lh_addr_of(paths->authum), lh_addr_of(env->authum),
                             lh_os_module_get_path(lh_addr_of(authum)));
    if (!lh_null_eq(env->authum) && authum_path_assign_parent(lh_addr_of(up), env->authum))
    {
        authum_child_bind_path(lh_addr_of(paths->addons), lh_addr_of(env->addons),
                               authum_path_get_cstr(lh_addr_of(up)));
        if (authum_path_assign_parent(lh_addr_of(up), env->addons))
        {
            authum_child_bind_path(lh_addr_of(paths->game), lh_addr_of(env->game),
                                   authum_path_get_cstr(lh_addr_of(up)));
        }
    }

    if (lh_null_eq(env->game) && !lh_null_eq(data->host.engine))
    {
        authum_char_t buf[authum_path_capacity];

        lh_memory_std_set(buf, 0, sizeof(buf));
        authum_engine_copy_game_dir(data->host.engine, buf,
                                    lh_cast_static(authum_usize_t, sizeof(buf)));
        authum_child_bind_path(lh_addr_of(paths->game), lh_addr_of(env->game), buf);
    }

    authum_child_fill_under_game(data);

    authum_child_bind_parent(lh_addr_of(paths->module), lh_addr_of(env->module),
                             lh_os_module_get_path(child));
    authum_child_bind_join(lh_addr_of(paths->module_config), lh_addr_of(env->module_config),
                           env->module, "configs");
    authum_child_bind_join(lh_addr_of(paths->module_lang), lh_addr_of(env->module_lang),
                           env->module, "lang");
    authum_child_bind_join(lh_addr_of(paths->module_data), lh_addr_of(env->module_data),
                           env->module, "data");
    /* Roots describe locations; writers create directories when needed. */
}

/* Implemented in crypto.c / engine_server.cpp (linked into authum). */
const authum_crypto_t *
authum_crypto(void);

const authum_engine_t *
authum_engine(void);

void
authum_child_fill_host(authum_module_t *child, authum_child_data_t *data)
{
    authum_module_t *parent;
    authum_host_t *out = lh_addr_of(data->host);

    out->engine_funcs = lh_null;
    out->globals = lh_null;
    out->env = lh_null;
    out->crypto = authum_crypto();
    out->engine = authum_engine();

    data->sandbox.ops = authum_sandbox_host_ops();
    data->sandbox.root = authum_sandbox_host_root_create();
    out->sandbox = lh_addr_of(data->sandbox);

    authum_events_set_ops(authum_child_data_get_events_mut(data), authum_events_host_ops());
    authum_events_set_owner(authum_child_data_get_events_mut(data), data);
    out->events = authum_child_data_get_events_mut(data);

    data->logger.ops = authum_logs_ops();
    data->logger.owner = data;
    out->logger = lh_addr_of(data->logger);

    data->commands.ops = authum_commands_host_ops();
    data->commands.owner = data;
    data->commands.scope = data->info->name;
    out->commands = lh_addr_of(data->commands);
    data->tasks.ops = authum_tasks_host_ops();
    data->tasks.owner = data;
    out->tasks = lh_addr_of(data->tasks);
    authum_threads_host_init(lh_addr_of(data->thread_owner), lh_addr_of(data->threads));
    out->threads = lh_addr_of(data->threads);
    authum_http_host_init(lh_addr_of(data->http_owner), lh_addr_of(data->http));
    out->http = lh_addr_of(data->http);
    out->clock = authum_clock_host();
    parent = lh_os_module_get_parent(child);
    if (!lh_null_eq(parent))
    {
        out->engine_funcs = lh_cast_static(authum_host_get_ptr_fn,
                                           lh_os_module_get_sym(parent, "authum_engine_funcs"));
        out->globals =
            lh_cast_static(authum_host_get_ptr_fn, lh_os_module_get_sym(parent, "authum_globals"));
    }

    authum_child_fill_env(child, data);
    /* Standard placeholder names come from the module's own environment. */
    authum_placeholder_host_init_env(lh_addr_of(data->placeholders), lh_addr_of(data->env),
                                     lh_addr_of(data->placeholder_service));
    out->placeholders = lh_addr_of(data->placeholder_service);
    authum_unused(
        authum_logs_open(data, data->info->name, authum_path_get_cstr_or_null(data->env.module)));
}

/** After `stop` (or a failed `start`): drop the module's subscriptions and
 *  commands, free its sandbox. */
void
authum_child_release(authum_child_data_t *data)
{
    authum_threads_host_stop_owner(lh_addr_of(data->thread_owner));
    authum_http_host_drop_owner(lh_addr_of(data->http_owner));
    authum_threads_host_drop_owner(lh_addr_of(data->thread_owner));
    authum_placeholder_host_drop_owner(lh_addr_of(data->placeholders));
    authum_tasks_host_drop_owner(data);
    authum_commands_host_drop_owner(data);
    authum_events_host_drop_owner(data);
    authum_logs_close(data);
    authum_sandbox_host_root_destroy(data->sandbox.root);
    authum_memory_free(data);
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

    data = lh_ptr_cast(authum_child_data_t, authum_memory_alloc(sizeof(lh_ptr_deref(data))));
    if (lh_null_eq(data))
    {
        lh_os_set_last_error_lit(lh_os_error_code_out_of_memory, "child data");
        return lh_bool_false;
    }
    lh_memory_std_set(data, 0, sizeof(lh_ptr_deref(data)));

    data->info = lh_ptr_ccast(authum_module_info_t, sym_info);
    data->api = lh_ptr_ccast(authum_module_api_t, sym_api);
    if (lh_null_eq(data->info->name) || lh_null_eq(data->api->start))
    {
        authum_memory_free(data);
        lh_os_set_last_error_lit(authum_error_code_invalid_info, "invalid module info/api");
        return lh_bool_false;
    }
    if (!authum_version_is_abi_compatible(lh_addr_of(data->info->abi), lh_addr_of(required)))
    {
        authum_memory_free(data);
        lh_os_set_last_error_lit(authum_error_code_incompatible_abi, "incompatible module abi");
        return lh_bool_false;
    }

    authum_child_fill_host(self, data);
    lh_os_module_set_data(self, data);

    if (!data->api->start(lh_addr_of(data->host)))
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
        authum_threads_host_stop_owner(lh_addr_of(data->thread_owner));
        authum_tasks_host_drop_owner(data);
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

    /* A module exports authum_module_api; anything else is not one. */
    if (lh_null_eq(lh_os_module_get_sym(child, AUTHUM_MODULE_API_SYM)))
    {
        lh_os_set_last_error_lit(authum_error_code_missing_info, "missing authum_module_api");
        lh_os_module_destroy(child);
        return lh_null;
    }
    lh_os_module_set_ops(child, lh_addr_of(authum_child_ops));

    if (!lh_os_module_start(child))
    {
        lh_os_module_destroy(child);
        return lh_null;
    }

    lh_list_push_back(lh_os_loader_get_modules_mut(loader), lh_os_module_get_node(child));
    return child;
}
