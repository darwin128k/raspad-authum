#include <authum/host.h>

authum_host_log_fn
authum_host_get_log(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->log : 0;
}

authum_host_get_ptr_fn
authum_host_engine_funcs(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->engine_funcs : 0;
}

authum_host_get_ptr_fn
authum_host_globals(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->globals : 0;
}

const authum_env_t *
authum_host_env(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->env : authum_null;
}

const authum_crypto_t *
authum_host_crypto(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->crypto : authum_null;
}

const authum_engine_t *
authum_host_engine(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->engine : authum_null;
}

const authum_char_t *
authum_host_module_dir(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->module_dir : authum_null;
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

void
authum_host_set_log(authum_host_t *host, authum_host_log_fn log)
{
    if (!authum_null_eq(host))
    {
        host->log = log;
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
authum_host_set_module_dir(authum_host_t *host, const authum_char_t *module_dir)
{
    if (!authum_null_eq(host))
    {
        host->module_dir = module_dir;
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
    if (authum_null_eq(host))
    {
        return;
    }
    host->log = 0;
    host->engine_funcs = 0;
    host->globals = 0;
    host->env = authum_null;
    host->module_dir = authum_null;
    host->crypto = authum_null;
    host->engine = authum_null;
    host->sandbox = authum_null;
    host->events = authum_null;
    host->commands = authum_null;
}

const authum_commands_t *
authum_host_commands(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->commands : authum_null;
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
    return (!authum_null_eq(host)) ? host->sandbox : authum_null;
}

const authum_events_t *
authum_host_events(const authum_host_t *host)
{
    return (!authum_null_eq(host)) ? host->events : authum_null;
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
    authum_engine_kick(authum_host_engine(host), userid, reason);
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
    authum_host_log_fn log = authum_host_get_log(host);
    if (log != 0 && !authum_null_eq(line))
    {
        log(is_error, line);
    }
}
