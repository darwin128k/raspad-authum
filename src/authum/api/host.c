#include <lh/util/ptr.h>
#include <lh/memory/std.h>
#include <authum/logger.h>

#include <stdio.h>
#include <string.h>

const authum_http_service_t *
authum_host_get_http(const authum_host_t *host)
{
    return host && !authum_null_eq((host)->http) ? host->http : authum_null;
}

const authum_clock_t *
authum_host_get_clock(const authum_host_t *host)
{
    return host && !authum_null_eq((host)->clock) ? host->clock : authum_null;
}

const authum_placeholder_service_t *
authum_host_get_placeholders(const authum_host_t *host)
{
    return host && !authum_null_eq((host)->placeholders) ? host->placeholders : authum_null;
}

const authum_threads_t *
authum_host_get_threads(const authum_host_t *host)
{
    return host && !authum_null_eq((host)->threads) ? host->threads : authum_null;
}

const authum_tasks_t *
authum_host_get_tasks(const authum_host_t *host)
{
    return host && !authum_null_eq((host)->tasks) ? host->tasks : authum_null;
}

const authum_logger_channel_t *
authum_host_get_logger(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->logger)) ? host->logger : authum_null;
}

authum_host_get_ptr_fn
authum_host_get_engine_funcs(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->engine_funcs)) ? host->engine_funcs
                                                                            : 0;
}

authum_host_get_ptr_fn
authum_host_get_globals(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->globals)) ? host->globals : 0;
}

const authum_env_t *
authum_host_get_env(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->env)) ? host->env : authum_null;
}

const authum_crypto_t *
authum_host_get_crypto(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->crypto)) ? host->crypto : authum_null;
}

const authum_engine_t *
authum_host_get_engine(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->engine)) ? host->engine : authum_null;
}

const authum_path_t *
authum_host_get_game_path(const authum_host_t *host)
{
    return authum_env_get_game_path(authum_host_get_env(host));
}

const authum_path_t *
authum_host_get_module_path(const authum_host_t *host)
{
    return authum_env_get_module_path(authum_host_get_env(host));
}

const authum_path_t *
authum_host_get_module_config_path(const authum_host_t *host)
{
    return authum_env_get_module_config_path(authum_host_get_env(host));
}

const authum_path_t *
authum_host_get_module_lang_path(const authum_host_t *host)
{
    return authum_env_get_module_lang_path(authum_host_get_env(host));
}

const authum_path_t *
authum_host_get_module_data_path(const authum_host_t *host)
{
    return authum_env_get_module_data_path(authum_host_get_env(host));
}

authum_bool_t
authum_host_join_config_path(const authum_host_t *host, const authum_char_t *leaf,
                             authum_path_t *out)
{
    return authum_env_join_config_path(authum_host_get_env(host), leaf, out);
}

authum_bool_t
authum_host_join_data_path(const authum_host_t *host, const authum_char_t *leaf, authum_path_t *out)
{
    return authum_env_join_data_path(authum_host_get_env(host), leaf, out);
}

void
authum_host_set_logger(authum_host_t *host, const authum_logger_channel_t *logger)
{
    if (!authum_null_eq(host))
    {
        host->logger = logger;
    }
}

void
authum_host_set_engine_funcs(authum_host_t *host, authum_host_get_ptr_fn fn)
{
    if (!authum_null_eq(host))
    {
        host->engine_funcs = fn;
    }
}

void
authum_host_set_globals(authum_host_t *host, authum_host_get_ptr_fn fn)
{
    if (!authum_null_eq(host))
    {
        host->globals = fn;
    }
}

void
authum_host_set_env(authum_host_t *host, const authum_env_t *env)
{
    if (!authum_null_eq(host))
    {
        host->env = env;
    }
}

void
authum_host_set_crypto(authum_host_t *host, const authum_crypto_t *crypto)
{
    if (!authum_null_eq(host))
    {
        host->crypto = crypto;
    }
}

void
authum_host_set_engine(authum_host_t *host, const authum_engine_t *engine)
{
    if (!authum_null_eq(host))
    {
        host->engine = engine;
    }
}

void
authum_host_clear(authum_host_t *host)
{
    if (!authum_null_eq(host))
    {
        lh_memory_std_set(host, 0, sizeof lh_ptr_deref(host));
    }
}

void
authum_host_copy(authum_host_t *dst, const authum_host_t *src)
{
    if (authum_null_eq(dst))
    {
        return;
    }
    if (authum_null_eq(src))
    {
        lh_memory_std_set(dst, 0, sizeof lh_ptr_deref(dst));
        return;
    }
    lh_memory_std_copy(dst, src, sizeof lh_ptr_deref(dst));
}

const authum_commands_t *
authum_host_get_commands(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->commands)) ? host->commands
                                                                        : authum_null;
}

void
authum_host_set_commands(authum_host_t *host, const authum_commands_t *commands)
{
    if (!authum_null_eq(host))
    {
        host->commands = commands;
    }
}

authum_bool_t
authum_host_command_add(const authum_host_t *host, const authum_command_def_t *def)
{
    return authum_commands_add(authum_host_get_commands(host), def);
}

authum_bool_t
authum_host_command_add_all(const authum_host_t *host, const authum_command_def_t *defs,
                            authum_usize_t count)
{
    return authum_commands_add_all(authum_host_get_commands(host), defs, count);
}

const authum_sandbox_t *
authum_host_get_sandbox(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->sandbox)) ? host->sandbox
                                                                       : authum_null;
}

const authum_events_t *
authum_host_get_events(const authum_host_t *host)
{
    return (!authum_null_eq(host) && !authum_null_eq((host)->events)) ? host->events : authum_null;
}

void
authum_host_set_sandbox(authum_host_t *host, const authum_sandbox_t *sandbox)
{
    if (!authum_null_eq(host))
    {
        host->sandbox = sandbox;
    }
}

void
authum_host_set_events(authum_host_t *host, const authum_events_t *events)
{
    if (!authum_null_eq(host))
    {
        host->events = events;
    }
}

void
authum_host_kick(const authum_host_t *host, authum_i32_t userid, const authum_char_t *reason)
{
    authum_engine_kick(authum_host_get_engine(host), userid, reason);
}

authum_i32_t
authum_host_get_max_clients(const authum_host_t *host)
{
    return authum_engine_get_max_clients(authum_host_get_engine(host));
}

authum_float_t
authum_host_get_game_time(const authum_host_t *host)
{
    return authum_engine_get_game_time(authum_host_get_engine(host));
}

void
authum_host_copy_map_name(const authum_host_t *host, authum_char_t *buf, authum_usize_t buf_size)
{
    authum_engine_copy_map_name(authum_host_get_engine(host), buf, buf_size);
}

authum_bool_t
authum_host_cvar_copy(const authum_host_t *host, const authum_char_t *name, authum_char_t *buf,
                      authum_usize_t buf_size)
{
    return authum_engine_cvar_copy(authum_host_get_engine(host), name, buf, buf_size);
}

authum_bool_t
authum_host_cvar_set(const authum_host_t *host, const authum_char_t *name,
                     const authum_char_t *value)
{
    return authum_engine_cvar_set(authum_host_get_engine(host), name, value);
}

authum_bool_t
authum_host_client_is_connected(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_is_connected(authum_host_get_engine(host), index);
}

authum_i32_t
authum_host_client_get_userid(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_get_userid(authum_host_get_engine(host), index);
}

authum_i32_t
authum_host_client_get_index_by_userid(const authum_host_t *host, authum_i32_t userid)
{
    return authum_engine_client_get_index_by_userid(authum_host_get_engine(host), userid);
}

authum_bool_t
authum_host_client_copy_name(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                             authum_usize_t buf_size)
{
    return authum_engine_client_copy_name(authum_host_get_engine(host), index, buf, buf_size);
}

authum_bool_t
authum_host_client_copy_auth_id(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                                authum_usize_t buf_size)
{
    return authum_engine_client_copy_auth_id(authum_host_get_engine(host), index, buf, buf_size);
}

authum_bool_t
authum_host_client_copy_address(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                                authum_usize_t buf_size)
{
    return authum_engine_client_copy_address(authum_host_get_engine(host), index, buf, buf_size);
}

authum_bool_t
authum_host_client_copy_info(const authum_host_t *host, authum_i32_t index,
                             const authum_char_t *key, authum_char_t *buf, authum_usize_t buf_size)
{
    return authum_engine_client_copy_info(authum_host_get_engine(host), index, key, buf, buf_size);
}

authum_bool_t
authum_host_client_print(const authum_host_t *host, authum_i32_t index, authum_i32_t kind,
                         const authum_char_t *text)
{
    return authum_engine_client_print(authum_host_get_engine(host), index, kind, text);
}

authum_bool_t
authum_host_client_command(const authum_host_t *host, authum_i32_t index, const authum_char_t *cmd)
{
    return authum_engine_client_command(authum_host_get_engine(host), index, cmd);
}

authum_bool_t
authum_host_client_try_get_stats(const authum_host_t *host, authum_i32_t index, authum_i32_t *ping,
                                 authum_i32_t *loss)
{
    return authum_engine_client_try_get_stats(authum_host_get_engine(host), index, ping, loss);
}

authum_bool_t
authum_host_client_set_info(const authum_host_t *host, authum_i32_t index, const authum_char_t *key,
                            const authum_char_t *value)
{
    return authum_engine_client_set_info(authum_host_get_engine(host), index, key, value);
}

authum_bool_t
authum_host_client_chat(const authum_host_t *host, authum_i32_t index, authum_i32_t sender,
                        const authum_char_t *text)
{
    return authum_engine_client_chat(authum_host_get_engine(host), index, sender, text);
}

authum_bool_t
authum_host_cvar_register(const authum_host_t *host, const authum_char_t *name,
                          const authum_char_t *value, authum_i32_t flags)
{
    return authum_engine_cvar_register(authum_host_get_engine(host), name, value, flags);
}

authum_i32_t
authum_host_cvar_get_i32(const authum_host_t *host, const authum_char_t *name,
                         authum_i32_t fallback)
{
    return authum_engine_cvar_get_i32(authum_host_get_engine(host), name, fallback);
}

authum_float_t
authum_host_cvar_get_float(const authum_host_t *host, const authum_char_t *name,
                           authum_float_t fallback)
{
    return authum_engine_cvar_get_float(authum_host_get_engine(host), name, fallback);
}

authum_i32_t
authum_host_message_get_id(const authum_host_t *host, const authum_char_t *name)
{
    return authum_engine_message_get_id(authum_host_get_engine(host), name);
}

authum_bool_t
authum_host_message_begin(const authum_host_t *host, authum_i32_t dest, authum_i32_t msg_id,
                          authum_i32_t index)
{
    return authum_engine_message_begin(authum_host_get_engine(host), dest, msg_id, index);
}

void
authum_host_message_write_byte(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_byte(authum_host_get_engine(host), value);
}

void
authum_host_message_write_char(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_char(authum_host_get_engine(host), value);
}

void
authum_host_message_write_short(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_short(authum_host_get_engine(host), value);
}

void
authum_host_message_write_long(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_long(authum_host_get_engine(host), value);
}

void
authum_host_message_write_entity(const authum_host_t *host, authum_i32_t index)
{
    authum_engine_message_write_entity(authum_host_get_engine(host), index);
}

void
authum_host_message_write_angle(const authum_host_t *host, authum_float_t value)
{
    authum_engine_message_write_angle(authum_host_get_engine(host), value);
}

void
authum_host_message_write_coord(const authum_host_t *host, authum_float_t value)
{
    authum_engine_message_write_coord(authum_host_get_engine(host), value);
}

void
authum_host_message_write_string(const authum_host_t *host, const authum_char_t *text)
{
    authum_engine_message_write_string(authum_host_get_engine(host), text);
}

void
authum_host_message_end(const authum_host_t *host)
{
    authum_engine_message_end(authum_host_get_engine(host));
}

authum_i32_t
authum_host_precache_model(const authum_host_t *host, const authum_char_t *path)
{
    return authum_engine_precache_model(authum_host_get_engine(host), path);
}

authum_i32_t
authum_host_precache_sound(const authum_host_t *host, const authum_char_t *path)
{
    return authum_engine_precache_sound(authum_host_get_engine(host), path);
}

authum_i32_t
authum_host_precache_generic(const authum_host_t *host, const authum_char_t *path)
{
    return authum_engine_precache_generic(authum_host_get_engine(host), path);
}

authum_i32_t
authum_host_entity_get_max(const authum_host_t *host)
{
    return authum_engine_entity_get_max(authum_host_get_engine(host));
}

authum_bool_t
authum_host_entity_is_valid(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_entity_is_valid(authum_host_get_engine(host), index);
}

authum_bool_t
authum_host_entity_copy_classname(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                                  authum_usize_t buf_size)
{
    return authum_engine_entity_copy_classname(authum_host_get_engine(host), index, buf, buf_size);
}

authum_i32_t
authum_host_entity_find_by_classname(const authum_host_t *host, authum_i32_t start,
                                     const authum_char_t *classname)
{
    return authum_engine_entity_find_by_classname(authum_host_get_engine(host), start, classname);
}

authum_bool_t
authum_host_entity_try_get_float(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                                 authum_float_t *out)
{
    return authum_engine_entity_try_get_float(authum_host_get_engine(host), index, field, out);
}

authum_bool_t
authum_host_entity_set_float(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                             authum_float_t value)
{
    return authum_engine_entity_set_float(authum_host_get_engine(host), index, field, value);
}

authum_bool_t
authum_host_entity_try_get_i32(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                               authum_i32_t *out)
{
    return authum_engine_entity_try_get_i32(authum_host_get_engine(host), index, field, out);
}

authum_bool_t
authum_host_entity_set_i32(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                           authum_i32_t value)
{
    return authum_engine_entity_set_i32(authum_host_get_engine(host), index, field, value);
}

authum_bool_t
authum_host_entity_copy_vector(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                               authum_float_t *out)
{
    return authum_engine_entity_copy_vector(authum_host_get_engine(host), index, field, out);
}

authum_bool_t
authum_host_entity_set_vector(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                              const authum_float_t *value)
{
    return authum_engine_entity_set_vector(authum_host_get_engine(host), index, field, value);
}

authum_bool_t
authum_host_client_is_alive(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_is_alive(authum_host_get_engine(host), index);
}

authum_bool_t
authum_host_client_is_bot(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_is_bot(authum_host_get_engine(host), index);
}

authum_bool_t
authum_host_query_cvar(const authum_host_t *host, authum_i32_t index, const authum_char_t *cvar,
                       authum_i32_t request_id)
{
    return authum_engine_query_cvar(authum_host_get_engine(host), index, cvar, request_id);
}

authum_bool_t
authum_host_subscribe(const authum_host_t *host, const authum_char_t *name,
                      authum_event_handler_fn fn, authum_ptr user)
{
    return authum_events_subscribe(authum_host_get_events(host), name, fn, user);
}

authum_event_result_t
authum_host_publish(const authum_host_t *host, const authum_char_t *name, authum_ptr payload)
{
    return authum_events_publish(authum_host_get_events(host), name, payload);
}

void
authum_host_copy_game_dir(const authum_host_t *host, authum_char_t *buf, authum_usize_t buf_size)
{
    authum_engine_copy_game_dir(authum_host_get_engine(host), buf, buf_size);
}

void
authum_host_server_print(const authum_host_t *host, const authum_char_t *text)
{
    authum_engine_server_print(authum_host_get_engine(host), text);
}

void
authum_host_server_command(const authum_host_t *host, const authum_char_t *cmd)
{
    authum_engine_server_command(authum_host_get_engine(host), cmd);
}

void
authum_host_log_line(const authum_host_t *host, authum_bool_t is_error, const authum_char_t *line)
{
    authum_logger_channel_write(authum_host_get_logger(host),
                                is_error ? authum_logger_level_error : authum_logger_level_info,
                                line);
}
