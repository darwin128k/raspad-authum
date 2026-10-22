#include <authum/engine.h>

#include <stdio.h>

/** Empty @p buf so a refused call still leaves a valid string. */
authum_bool_t
authum_engine_buf_reset(authum_char_t *buf, authum_usize_t buf_size)
{
    if (authum_null_eq(buf) || buf_size == 0u)
    {
        return authum_bool_false;
    }
    buf[0] = '\0';
    return authum_bool_true;
}

/* --- server --------------------------------------------------------------- */

void
authum_engine_get_game_dir(const authum_engine_t *engine, authum_char_t *buf,
                           authum_usize_t buf_size)
{
    if (authum_engine_buf_reset(buf, buf_size) && !authum_null_eq(engine) &&
        AUTHUM_ABI_HAS(engine, get_game_dir))
    {
        engine->get_game_dir(buf, buf_size);
    }
}

void
authum_engine_server_print(const authum_engine_t *engine, const authum_char_t *text)
{
    if (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, server_print) && !authum_null_eq(text))
    {
        engine->server_print(text);
    }
}

void
authum_engine_server_command(const authum_engine_t *engine, const authum_char_t *cmd)
{
    if (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, server_command) && !authum_null_eq(cmd))
    {
        engine->server_command(cmd);
    }
}

authum_i32_t
authum_engine_max_clients(const authum_engine_t *engine)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, max_clients)) ? engine->max_clients()
                                                                            : 0;
}

authum_float_t
authum_engine_game_time(const authum_engine_t *engine)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, game_time)) ? engine->game_time()
                                                                          : 0.0f;
}

void
authum_engine_get_map_name(const authum_engine_t *engine, authum_char_t *buf,
                           authum_usize_t buf_size)
{
    if (authum_engine_buf_reset(buf, buf_size) && !authum_null_eq(engine) &&
        AUTHUM_ABI_HAS(engine, get_map_name))
    {
        engine->get_map_name(buf, buf_size);
    }
}

authum_bool_t
authum_engine_cvar_get(const authum_engine_t *engine, const authum_char_t *name, authum_char_t *buf,
                       authum_usize_t buf_size)
{
    if (!authum_engine_buf_reset(buf, buf_size) || authum_null_eq(engine) ||
        !AUTHUM_ABI_HAS(engine, cvar_get) || authum_null_eq(name) || name[0] == '\0')
    {
        return authum_bool_false;
    }
    return engine->cvar_get(name, buf, buf_size);
}

authum_bool_t
authum_engine_cvar_set(const authum_engine_t *engine, const authum_char_t *name,
                       const authum_char_t *value)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, cvar_set) || authum_null_eq(name) ||
        name[0] == '\0' || authum_null_eq(value))
    {
        return authum_bool_false;
    }
    return engine->cvar_set(name, value);
}

void
authum_engine_kick(const authum_engine_t *engine, authum_i32_t userid, const authum_char_t *reason)
{
    authum_char_t clean[128];
    authum_char_t cmd[192];
    authum_usize_t n = 0u;

    if (!authum_null_eq(reason))
    {
        for (; reason[n] != '\0' && n < sizeof clean - 1u; ++n)
        {
            authum_char_t c = reason[n];
            clean[n] = (c == '"' || c == ';' || c == '\n' || c == '\r') ? ' ' : c;
        }
    }
    clean[n] = '\0';
    if (n > 0u)
    {
        snprintf(cmd, sizeof cmd, "kick #%d \"%s\"\n", (int)userid, clean);
    }
    else
    {
        snprintf(cmd, sizeof cmd, "kick #%d\n", (int)userid);
    }
    authum_engine_server_command(engine, cmd);
}

/* --- players -------------------------------------------------------------- */

authum_bool_t
authum_engine_client_is_connected(const authum_engine_t *engine, authum_i32_t index)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, client_is_connected))
               ? engine->client_is_connected(index)
               : authum_bool_false;
}

authum_i32_t
authum_engine_client_userid(const authum_engine_t *engine, authum_i32_t index)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, client_userid))
               ? engine->client_userid(index)
               : 0;
}

authum_i32_t
authum_engine_client_index_by_userid(const authum_engine_t *engine, authum_i32_t userid)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, client_index_by_userid))
               ? engine->client_index_by_userid(userid)
               : 0;
}

authum_bool_t
authum_engine_client_get_name(const authum_engine_t *engine, authum_i32_t index, authum_char_t *buf,
                              authum_usize_t buf_size)
{
    if (!authum_engine_buf_reset(buf, buf_size) || authum_null_eq(engine) ||
        !AUTHUM_ABI_HAS(engine, client_get_name))
    {
        return authum_bool_false;
    }
    return engine->client_get_name(index, buf, buf_size);
}

authum_bool_t
authum_engine_client_get_auth_id(const authum_engine_t *engine, authum_i32_t index,
                                 authum_char_t *buf, authum_usize_t buf_size)
{
    if (!authum_engine_buf_reset(buf, buf_size) || authum_null_eq(engine) ||
        !AUTHUM_ABI_HAS(engine, client_get_auth_id))
    {
        return authum_bool_false;
    }
    return engine->client_get_auth_id(index, buf, buf_size);
}

authum_bool_t
authum_engine_client_get_address(const authum_engine_t *engine, authum_i32_t index,
                                 authum_char_t *buf, authum_usize_t buf_size)
{
    if (!authum_engine_buf_reset(buf, buf_size) || authum_null_eq(engine) ||
        !AUTHUM_ABI_HAS(engine, client_get_address))
    {
        return authum_bool_false;
    }
    return engine->client_get_address(index, buf, buf_size);
}

authum_bool_t
authum_engine_client_get_info(const authum_engine_t *engine, authum_i32_t index,
                              const authum_char_t *key, authum_char_t *buf, authum_usize_t buf_size)
{
    if (!authum_engine_buf_reset(buf, buf_size) || authum_null_eq(engine) ||
        !AUTHUM_ABI_HAS(engine, client_get_info) || authum_null_eq(key) || key[0] == '\0')
    {
        return authum_bool_false;
    }
    return engine->client_get_info(index, key, buf, buf_size);
}

authum_bool_t
authum_engine_client_print(const authum_engine_t *engine, authum_i32_t index, authum_i32_t kind,
                           const authum_char_t *text)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, client_print) || authum_null_eq(text) ||
        kind < authum_engine_print_console || kind > authum_engine_print_chat)
    {
        return authum_bool_false;
    }
    return engine->client_print(index, kind, text);
}

authum_bool_t
authum_engine_client_command(const authum_engine_t *engine, authum_i32_t index,
                             const authum_char_t *cmd)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, client_command) || authum_null_eq(cmd) ||
        cmd[0] == '\0')
    {
        return authum_bool_false;
    }
    return engine->client_command(index, cmd);
}

authum_bool_t
authum_engine_query_cvar(const authum_engine_t *engine, authum_i32_t index,
                         const authum_char_t *cvar, authum_i32_t request_id)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, query_client_cvar) ||
        authum_null_eq(cvar) || cvar[0] == '\0')
    {
        return authum_bool_false;
    }
    return engine->query_client_cvar(index, cvar, request_id);
}
