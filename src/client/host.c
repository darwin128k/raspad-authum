#include <authum/host.h>
#include <authum/logger.h>

#include <stdio.h>
#include <string.h>

const authum_logger_channel_t *
authum_host_logger(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, logger)) ? host->logger : authum_null;
}

authum_host_get_ptr_fn
authum_host_engine_funcs(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, engine_funcs)) ? host->engine_funcs : 0;
}

authum_host_get_ptr_fn
authum_host_globals(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, globals)) ? host->globals : 0;
}

const authum_env_t *
authum_host_env(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, env)) ? host->env : authum_null;
}

const authum_crypto_t *
authum_host_crypto(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, crypto)) ? host->crypto : authum_null;
}

const authum_engine_t *
authum_host_engine(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, engine)) ? host->engine : authum_null;
}

const authum_char_t *
authum_host_module_dir(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, module_dir)) ? host->module_dir
                                                                       : authum_null;
}

const authum_path_t *
authum_host_env_module(const authum_host_t *host)
{
    return authum_env_module(authum_host_env(host));
}

const authum_path_t *
authum_host_env_game(const authum_host_t *host)
{
    return authum_env_game(authum_host_env(host));
}

const authum_char_t *
authum_host_env_module_cstr(const authum_host_t *host)
{
    return authum_env_module_cstr(authum_host_env(host));
}

const authum_path_t *
authum_host_env_configs(const authum_host_t *host)
{
    return authum_env_configs(authum_host_env(host));
}

const authum_path_t *
authum_host_env_data(const authum_host_t *host)
{
    return authum_env_data(authum_host_env(host));
}

const authum_path_t *
authum_host_env_lang(const authum_host_t *host)
{
    return authum_env_lang(authum_host_env(host));
}

const authum_path_t *
authum_host_env_module_config(const authum_host_t *host)
{
    return authum_env_module_config(authum_host_env(host));
}

const authum_path_t *
authum_host_env_module_data(const authum_host_t *host)
{
    return authum_env_module_data(authum_host_env(host));
}

authum_bool_t
authum_host_config_path(const authum_host_t *host, const authum_char_t *leaf, authum_path_t *out)
{
    authum_env_found_t found = authum_env_config_path(authum_host_env(host), leaf, out);
    authum_char_t line[authum_path_capacity + 96];

    if (found == authum_env_found_legacy)
    {
        snprintf(line, sizeof line, "config %s is in the old module folder (%s); move it to %s",
                 leaf, authum_path_as_cstr(out),
                 authum_path_as_cstr_or_null(authum_host_env_configs(host)) != authum_null
                     ? authum_path_as_cstr(authum_host_env_configs(host))
                     : "configs/");
        authum_logger_channel_write(authum_host_logger(host), authum_logger_level_warning, line);
    }
    return found != authum_env_found_none ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_host_data_path(const authum_host_t *host, const authum_char_t *leaf, authum_path_t *out)
{
    authum_env_found_t found = authum_env_data_path(authum_host_env(host), leaf, out);
    authum_char_t line[authum_path_capacity + 64];

    if (found == authum_env_found_legacy)
    {
        snprintf(line, sizeof line, "moved %s from the old module folder to %s", leaf,
                 authum_path_as_cstr(out));
        authum_logger_channel_write(authum_host_logger(host), authum_logger_level_notice, line);
    }
    return !authum_path_is_empty(out) ? authum_bool_true : authum_bool_false;
}

void
authum_host_set_logger(authum_host_t *host, const authum_logger_channel_t *logger)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, logger))
    {
        host->logger = logger;
    }
}

void
authum_host_set_engine_funcs(authum_host_t *host, authum_host_get_ptr_fn fn)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, engine_funcs))
    {
        host->engine_funcs = fn;
    }
}

void
authum_host_set_globals(authum_host_t *host, authum_host_get_ptr_fn fn)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, globals))
    {
        host->globals = fn;
    }
}

void
authum_host_set_env(authum_host_t *host, const authum_env_t *env)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, env))
    {
        host->env = env;
    }
}

void
authum_host_set_module_dir(authum_host_t *host, const authum_char_t *module_dir)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, module_dir))
    {
        host->module_dir = module_dir;
    }
}

void
authum_host_set_crypto(authum_host_t *host, const authum_crypto_t *crypto)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, crypto))
    {
        host->crypto = crypto;
    }
}

void
authum_host_set_engine(authum_host_t *host, const authum_engine_t *engine)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, engine))
    {
        host->engine = engine;
    }
}

void
authum_host_clear(authum_host_t *host)
{
    authum_usize_t size;

    if (authum_null_eq(host) || host->size < sizeof host->size)
    {
        return;
    }
    size = host->size < sizeof *host ? host->size : sizeof *host;
    memset((char *)host + sizeof host->size, 0, size - sizeof host->size);
}

void
authum_host_copy(authum_host_t *dst, const authum_host_t *src)
{
    authum_usize_t size;

    if (authum_null_eq(dst))
    {
        return;
    }
    memset(dst, 0, sizeof *dst);
    if (authum_null_eq(src) || src->size < sizeof src->size)
    {
        return;
    }
    size = src->size < sizeof *dst ? src->size : sizeof *dst;
    memcpy(dst, src, size);
    dst->size = size;
}

const authum_commands_t *
authum_host_commands(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, commands)) ? host->commands : authum_null;
}

void
authum_host_set_commands(authum_host_t *host, const authum_commands_t *commands)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, commands))
    {
        host->commands = commands;
    }
}

authum_bool_t
authum_host_command_add(const authum_host_t *host, const authum_command_def_t *def)
{
    return authum_commands_add(authum_host_commands(host), def);
}

authum_bool_t
authum_host_command_add_all(const authum_host_t *host, const authum_command_def_t *defs,
                            authum_usize_t count)
{
    return authum_commands_add_all(authum_host_commands(host), defs, count);
}

const authum_sandbox_t *
authum_host_sandbox(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, sandbox)) ? host->sandbox : authum_null;
}

const authum_events_t *
authum_host_events(const authum_host_t *host)
{
    return (!authum_null_eq(host) && AUTHUM_ABI_HAS(host, events)) ? host->events : authum_null;
}

void
authum_host_set_sandbox(authum_host_t *host, const authum_sandbox_t *sandbox)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, sandbox))
    {
        host->sandbox = sandbox;
    }
}

void
authum_host_set_events(authum_host_t *host, const authum_events_t *events)
{
    if (!authum_null_eq(host) && AUTHUM_ABI_FITS(host, events))
    {
        host->events = events;
    }
}

void
authum_host_kick(const authum_host_t *host, authum_i32_t userid, const authum_char_t *reason)
{
    authum_engine_kick(authum_host_engine(host), userid, reason);
}

authum_i32_t
authum_host_max_clients(const authum_host_t *host)
{
    return authum_engine_max_clients(authum_host_engine(host));
}

authum_float_t
authum_host_game_time(const authum_host_t *host)
{
    return authum_engine_game_time(authum_host_engine(host));
}

void
authum_host_get_map_name(const authum_host_t *host, authum_char_t *buf, authum_usize_t buf_size)
{
    authum_engine_get_map_name(authum_host_engine(host), buf, buf_size);
}

authum_bool_t
authum_host_cvar_get(const authum_host_t *host, const authum_char_t *name, authum_char_t *buf,
                     authum_usize_t buf_size)
{
    return authum_engine_cvar_get(authum_host_engine(host), name, buf, buf_size);
}

authum_bool_t
authum_host_cvar_set(const authum_host_t *host, const authum_char_t *name,
                     const authum_char_t *value)
{
    return authum_engine_cvar_set(authum_host_engine(host), name, value);
}

authum_bool_t
authum_host_client_is_connected(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_is_connected(authum_host_engine(host), index);
}

authum_i32_t
authum_host_client_userid(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_userid(authum_host_engine(host), index);
}

authum_i32_t
authum_host_client_index_by_userid(const authum_host_t *host, authum_i32_t userid)
{
    return authum_engine_client_index_by_userid(authum_host_engine(host), userid);
}

authum_bool_t
authum_host_client_get_name(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                            authum_usize_t buf_size)
{
    return authum_engine_client_get_name(authum_host_engine(host), index, buf, buf_size);
}

authum_bool_t
authum_host_client_get_auth_id(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                               authum_usize_t buf_size)
{
    return authum_engine_client_get_auth_id(authum_host_engine(host), index, buf, buf_size);
}

authum_bool_t
authum_host_client_get_address(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                               authum_usize_t buf_size)
{
    return authum_engine_client_get_address(authum_host_engine(host), index, buf, buf_size);
}

authum_bool_t
authum_host_client_get_info(const authum_host_t *host, authum_i32_t index, const authum_char_t *key,
                            authum_char_t *buf, authum_usize_t buf_size)
{
    return authum_engine_client_get_info(authum_host_engine(host), index, key, buf, buf_size);
}

authum_bool_t
authum_host_client_print(const authum_host_t *host, authum_i32_t index, authum_i32_t kind,
                         const authum_char_t *text)
{
    return authum_engine_client_print(authum_host_engine(host), index, kind, text);
}

authum_bool_t
authum_host_client_command(const authum_host_t *host, authum_i32_t index, const authum_char_t *cmd)
{
    return authum_engine_client_command(authum_host_engine(host), index, cmd);
}

authum_bool_t
authum_host_client_get_stats(const authum_host_t *host, authum_i32_t index, authum_i32_t *ping,
                             authum_i32_t *loss)
{
    return authum_engine_client_get_stats(authum_host_engine(host), index, ping, loss);
}

authum_bool_t
authum_host_client_set_info(const authum_host_t *host, authum_i32_t index, const authum_char_t *key,
                            const authum_char_t *value)
{
    return authum_engine_client_set_info(authum_host_engine(host), index, key, value);
}

authum_bool_t
authum_host_client_chat(const authum_host_t *host, authum_i32_t index, authum_i32_t sender,
                        const authum_char_t *text)
{
    return authum_engine_client_chat(authum_host_engine(host), index, sender, text);
}

authum_bool_t
authum_host_cvar_register(const authum_host_t *host, const authum_char_t *name,
                          const authum_char_t *value, authum_i32_t flags)
{
    return authum_engine_cvar_register(authum_host_engine(host), name, value, flags);
}

authum_i32_t
authum_host_cvar_get_i32(const authum_host_t *host, const authum_char_t *name,
                         authum_i32_t fallback)
{
    return authum_engine_cvar_get_i32(authum_host_engine(host), name, fallback);
}

authum_float_t
authum_host_cvar_get_float(const authum_host_t *host, const authum_char_t *name,
                           authum_float_t fallback)
{
    return authum_engine_cvar_get_float(authum_host_engine(host), name, fallback);
}

authum_i32_t
authum_host_message_id(const authum_host_t *host, const authum_char_t *name)
{
    return authum_engine_message_id(authum_host_engine(host), name);
}

authum_bool_t
authum_host_message_begin(const authum_host_t *host, authum_i32_t dest, authum_i32_t msg_id,
                          authum_i32_t index)
{
    return authum_engine_message_begin(authum_host_engine(host), dest, msg_id, index);
}

void
authum_host_message_write_byte(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_byte(authum_host_engine(host), value);
}

void
authum_host_message_write_char(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_char(authum_host_engine(host), value);
}

void
authum_host_message_write_short(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_short(authum_host_engine(host), value);
}

void
authum_host_message_write_long(const authum_host_t *host, authum_i32_t value)
{
    authum_engine_message_write_long(authum_host_engine(host), value);
}

void
authum_host_message_write_entity(const authum_host_t *host, authum_i32_t index)
{
    authum_engine_message_write_entity(authum_host_engine(host), index);
}

void
authum_host_message_write_angle(const authum_host_t *host, authum_float_t value)
{
    authum_engine_message_write_angle(authum_host_engine(host), value);
}

void
authum_host_message_write_coord(const authum_host_t *host, authum_float_t value)
{
    authum_engine_message_write_coord(authum_host_engine(host), value);
}

void
authum_host_message_write_string(const authum_host_t *host, const authum_char_t *text)
{
    authum_engine_message_write_string(authum_host_engine(host), text);
}

void
authum_host_message_end(const authum_host_t *host)
{
    authum_engine_message_end(authum_host_engine(host));
}

authum_i32_t
authum_host_precache_model(const authum_host_t *host, const authum_char_t *path)
{
    return authum_engine_precache_model(authum_host_engine(host), path);
}

authum_i32_t
authum_host_precache_sound(const authum_host_t *host, const authum_char_t *path)
{
    return authum_engine_precache_sound(authum_host_engine(host), path);
}

authum_i32_t
authum_host_precache_generic(const authum_host_t *host, const authum_char_t *path)
{
    return authum_engine_precache_generic(authum_host_engine(host), path);
}

authum_i32_t
authum_host_entity_max(const authum_host_t *host)
{
    return authum_engine_entity_max(authum_host_engine(host));
}

authum_bool_t
authum_host_entity_is_valid(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_entity_is_valid(authum_host_engine(host), index);
}

authum_bool_t
authum_host_entity_get_classname(const authum_host_t *host, authum_i32_t index, authum_char_t *buf,
                                 authum_usize_t buf_size)
{
    return authum_engine_entity_get_classname(authum_host_engine(host), index, buf, buf_size);
}

authum_i32_t
authum_host_entity_find_by_classname(const authum_host_t *host, authum_i32_t start,
                                     const authum_char_t *classname)
{
    return authum_engine_entity_find_by_classname(authum_host_engine(host), start, classname);
}

authum_bool_t
authum_host_entity_get_float(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                             authum_float_t *out)
{
    return authum_engine_entity_get_float(authum_host_engine(host), index, field, out);
}

authum_bool_t
authum_host_entity_set_float(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                             authum_float_t value)
{
    return authum_engine_entity_set_float(authum_host_engine(host), index, field, value);
}

authum_bool_t
authum_host_entity_get_i32(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                           authum_i32_t *out)
{
    return authum_engine_entity_get_i32(authum_host_engine(host), index, field, out);
}

authum_bool_t
authum_host_entity_set_i32(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                           authum_i32_t value)
{
    return authum_engine_entity_set_i32(authum_host_engine(host), index, field, value);
}

authum_bool_t
authum_host_entity_get_vector(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                              authum_float_t *out)
{
    return authum_engine_entity_get_vector(authum_host_engine(host), index, field, out);
}

authum_bool_t
authum_host_entity_set_vector(const authum_host_t *host, authum_i32_t index, authum_i32_t field,
                              const authum_float_t *value)
{
    return authum_engine_entity_set_vector(authum_host_engine(host), index, field, value);
}

authum_bool_t
authum_host_client_is_alive(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_is_alive(authum_host_engine(host), index);
}

authum_bool_t
authum_host_client_is_bot(const authum_host_t *host, authum_i32_t index)
{
    return authum_engine_client_is_bot(authum_host_engine(host), index);
}

authum_bool_t
authum_host_query_cvar(const authum_host_t *host, authum_i32_t index, const authum_char_t *cvar,
                       authum_i32_t request_id)
{
    return authum_engine_query_cvar(authum_host_engine(host), index, cvar, request_id);
}

authum_bool_t
authum_host_subscribe(const authum_host_t *host, const authum_char_t *name,
                      authum_event_handler_fn fn, authum_ptr user)
{
    return authum_events_subscribe(authum_host_events(host), name, fn, user);
}

authum_event_result_t
authum_host_publish(const authum_host_t *host, const authum_char_t *name, authum_ptr payload)
{
    return authum_events_publish(authum_host_events(host), name, payload);
}

void
authum_host_get_game_dir(const authum_host_t *host, authum_char_t *buf, authum_usize_t buf_size)
{
    authum_engine_get_game_dir(authum_host_engine(host), buf, buf_size);
}

void
authum_host_server_print(const authum_host_t *host, const authum_char_t *text)
{
    authum_engine_server_print(authum_host_engine(host), text);
}

void
authum_host_server_command(const authum_host_t *host, const authum_char_t *cmd)
{
    authum_engine_server_command(authum_host_engine(host), cmd);
}

void
authum_host_log_line(const authum_host_t *host, int is_error, const authum_char_t *line)
{
    authum_logger_channel_write(authum_host_logger(host),
                                is_error ? authum_logger_level_error : authum_logger_level_info,
                                line);
}
