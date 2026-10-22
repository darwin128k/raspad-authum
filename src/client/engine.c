#include <authum/engine.h>

#include <authum/cstr.h>

#include <stdio.h>
#include <stdlib.h>

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

authum_bool_t
authum_engine_client_get_stats(const authum_engine_t *engine, authum_i32_t index,
                               authum_i32_t *ping, authum_i32_t *loss)
{
    authum_i32_t dummy_ping;
    authum_i32_t dummy_loss;

    ping = authum_null_eq(ping) ? &dummy_ping : ping;
    loss = authum_null_eq(loss) ? &dummy_loss : loss;
    *ping = 0;
    *loss = 0;
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, client_get_stats))
    {
        return authum_bool_false;
    }
    return engine->client_get_stats(index, ping, loss);
}

authum_bool_t
authum_engine_client_set_info(const authum_engine_t *engine, authum_i32_t index,
                              const authum_char_t *key, const authum_char_t *value)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, client_set_info) || authum_null_eq(key) ||
        key[0] == '\0' || authum_null_eq(value))
    {
        return authum_bool_false;
    }
    return engine->client_set_info(index, key, value);
}

authum_bool_t
authum_engine_client_chat(const authum_engine_t *engine, authum_i32_t index, authum_i32_t sender,
                          const authum_char_t *text)
{
    authum_i32_t say_text = authum_engine_message_id(engine, "SayText");
    authum_i32_t dest = index == 0 ? authum_message_dest_all : authum_message_dest_one;

    if (say_text == 0 || authum_null_eq(text))
    {
        return authum_bool_false;
    }
    /* SayText carries the sender's index first; its team picks the \x03 color. */
    if (!authum_engine_message_begin(engine, dest, say_text, index))
    {
        return authum_bool_false;
    }
    authum_engine_message_write_byte(engine, sender);
    authum_engine_message_write_string(engine, text);
    authum_engine_message_end(engine);
    return authum_bool_true;
}

/* --- cvars ------------------------------------------------------------------ */

authum_bool_t
authum_engine_cvar_register(const authum_engine_t *engine, const authum_char_t *name,
                            const authum_char_t *value, authum_i32_t flags)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, cvar_register) || authum_null_eq(name) ||
        name[0] == '\0')
    {
        return authum_bool_false;
    }
    return engine->cvar_register(name, authum_null_eq(value) ? "" : value, flags);
}

authum_i32_t
authum_engine_cvar_get_i32(const authum_engine_t *engine, const authum_char_t *name,
                           authum_i32_t fallback)
{
    authum_char_t text[64];
    authum_i32_t value;

    return (authum_engine_cvar_get(engine, name, text, sizeof text) &&
            authum_cstr_parse_i32(text, &value))
               ? value
               : fallback;
}

authum_float_t
authum_engine_cvar_get_float(const authum_engine_t *engine, const authum_char_t *name,
                             authum_float_t fallback)
{
    authum_char_t text[64];
    char *end;
    authum_float_t value;

    if (!authum_engine_cvar_get(engine, name, text, sizeof text))
    {
        return fallback;
    }
    value = strtof(text, &end);
    return (end != text) ? value : fallback;
}

/* --- user messages ---------------------------------------------------------- */

authum_i32_t
authum_engine_message_id(const authum_engine_t *engine, const authum_char_t *name)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, message_id) || authum_null_eq(name) ||
        name[0] == '\0')
    {
        return 0;
    }
    return engine->message_id(name);
}

/* Set by a successful begin, cleared by end: writes after a refused begin are dropped. */
authum_bool_t authum_engine_message_open;

authum_bool_t
authum_engine_message_begin(const authum_engine_t *engine, authum_i32_t dest, authum_i32_t msg_id,
                            authum_i32_t index)
{
    authum_engine_message_open = authum_bool_false;
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, message_begin) ||
        !AUTHUM_ABI_HAS(engine, message_write) || !AUTHUM_ABI_HAS(engine, message_end) ||
        msg_id <= 0)
    {
        return authum_bool_false;
    }
    authum_engine_message_open = engine->message_begin(dest, msg_id, index);
    return authum_engine_message_open;
}

void
authum_engine_message_write(const authum_engine_t *engine, authum_i32_t kind, authum_i32_t value,
                            authum_float_t real, const authum_char_t *text)
{
    if (authum_engine_message_open && !authum_null_eq(engine))
    {
        engine->message_write(kind, value, real, text);
    }
}

void
authum_engine_message_write_byte(const authum_engine_t *engine, authum_i32_t value)
{
    authum_engine_message_write(engine, authum_message_arg_byte, value, 0.0f, authum_null);
}

void
authum_engine_message_write_char(const authum_engine_t *engine, authum_i32_t value)
{
    authum_engine_message_write(engine, authum_message_arg_char, value, 0.0f, authum_null);
}

void
authum_engine_message_write_short(const authum_engine_t *engine, authum_i32_t value)
{
    authum_engine_message_write(engine, authum_message_arg_short, value, 0.0f, authum_null);
}

void
authum_engine_message_write_long(const authum_engine_t *engine, authum_i32_t value)
{
    authum_engine_message_write(engine, authum_message_arg_long, value, 0.0f, authum_null);
}

void
authum_engine_message_write_entity(const authum_engine_t *engine, authum_i32_t index)
{
    authum_engine_message_write(engine, authum_message_arg_entity, index, 0.0f, authum_null);
}

void
authum_engine_message_write_angle(const authum_engine_t *engine, authum_float_t value)
{
    authum_engine_message_write(engine, authum_message_arg_angle, 0, value, authum_null);
}

void
authum_engine_message_write_coord(const authum_engine_t *engine, authum_float_t value)
{
    authum_engine_message_write(engine, authum_message_arg_coord, 0, value, authum_null);
}

void
authum_engine_message_write_string(const authum_engine_t *engine, const authum_char_t *text)
{
    authum_engine_message_write(engine, authum_message_arg_string, 0, 0.0f,
                                authum_null_eq(text) ? "" : text);
}

void
authum_engine_message_end(const authum_engine_t *engine)
{
    if (authum_engine_message_open && !authum_null_eq(engine))
    {
        engine->message_end();
    }
    authum_engine_message_open = authum_bool_false;
}

/* --- precache --------------------------------------------------------------- */

authum_i32_t
authum_engine_precache_model(const authum_engine_t *engine, const authum_char_t *path)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, precache_model) || authum_null_eq(path) ||
        path[0] == 0)
    {
        return 0;
    }
    return engine->precache_model(path);
}

authum_i32_t
authum_engine_precache_sound(const authum_engine_t *engine, const authum_char_t *path)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, precache_sound) || authum_null_eq(path) ||
        path[0] == 0)
    {
        return 0;
    }
    return engine->precache_sound(path);
}

authum_i32_t
authum_engine_precache_generic(const authum_engine_t *engine, const authum_char_t *path)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, precache_generic) ||
        authum_null_eq(path) || path[0] == 0)
    {
        return 0;
    }
    return engine->precache_generic(path);
}

/* --- entities ---------------------------------------------------------------- */

authum_i32_t
authum_engine_entity_max(const authum_engine_t *engine)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, entity_max)) ? engine->entity_max()
                                                                           : 0;
}

authum_bool_t
authum_engine_entity_is_valid(const authum_engine_t *engine, authum_i32_t index)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, entity_is_valid))
               ? engine->entity_is_valid(index)
               : authum_bool_false;
}

authum_bool_t
authum_engine_entity_get_classname(const authum_engine_t *engine, authum_i32_t index,
                                   authum_char_t *buf, authum_usize_t buf_size)
{
    if (!authum_engine_buf_reset(buf, buf_size) || authum_null_eq(engine) ||
        !AUTHUM_ABI_HAS(engine, entity_get_classname))
    {
        return authum_bool_false;
    }
    return engine->entity_get_classname(index, buf, buf_size);
}

authum_i32_t
authum_engine_entity_find_by_classname(const authum_engine_t *engine, authum_i32_t start,
                                       const authum_char_t *classname)
{
    if (authum_null_eq(engine) || !AUTHUM_ABI_HAS(engine, entity_find_by_classname) ||
        authum_null_eq(classname) || classname[0] == 0)
    {
        return 0;
    }
    return engine->entity_find_by_classname(start, classname);
}

authum_bool_t
authum_engine_entity_get_float(const authum_engine_t *engine, authum_i32_t index,
                               authum_i32_t field, authum_float_t *out)
{
    authum_float_t dummy;

    out = authum_null_eq(out) ? &dummy : out;
    *out = 0.0f;
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, entity_get_float))
               ? engine->entity_get_float(index, field, out)
               : authum_bool_false;
}

authum_bool_t
authum_engine_entity_set_float(const authum_engine_t *engine, authum_i32_t index,
                               authum_i32_t field, authum_float_t value)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, entity_set_float))
               ? engine->entity_set_float(index, field, value)
               : authum_bool_false;
}

authum_bool_t
authum_engine_entity_get_i32(const authum_engine_t *engine, authum_i32_t index, authum_i32_t field,
                             authum_i32_t *out)
{
    authum_i32_t dummy;

    out = authum_null_eq(out) ? &dummy : out;
    *out = 0;
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, entity_get_i32))
               ? engine->entity_get_i32(index, field, out)
               : authum_bool_false;
}

authum_bool_t
authum_engine_entity_set_i32(const authum_engine_t *engine, authum_i32_t index, authum_i32_t field,
                             authum_i32_t value)
{
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, entity_set_i32))
               ? engine->entity_set_i32(index, field, value)
               : authum_bool_false;
}

authum_bool_t
authum_engine_entity_get_vector(const authum_engine_t *engine, authum_i32_t index,
                                authum_i32_t field, authum_float_t out[3])
{
    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    return (!authum_null_eq(engine) && AUTHUM_ABI_HAS(engine, entity_get_vector))
               ? engine->entity_get_vector(index, field, out)
               : authum_bool_false;
}

authum_bool_t
authum_engine_entity_set_vector(const authum_engine_t *engine, authum_i32_t index,
                                authum_i32_t field, const authum_float_t value[3])
{
    return (!authum_null_eq(engine) && !authum_null_eq(value) &&
            AUTHUM_ABI_HAS(engine, entity_set_vector))
               ? engine->entity_set_vector(index, field, value)
               : authum_bool_false;
}

authum_bool_t
authum_engine_client_is_alive(const authum_engine_t *engine, authum_i32_t index)
{
    authum_i32_t dead;
    authum_float_t health;

    return (authum_engine_client_is_connected(engine, index) &&
            authum_engine_entity_get_i32(engine, index, authum_entity_i32_dead, &dead) &&
            dead == authum_entity_dead_no &&
            authum_engine_entity_get_float(engine, index, authum_entity_float_health, &health) &&
            health > 0.0f)
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_engine_client_is_bot(const authum_engine_t *engine, authum_i32_t index)
{
    authum_i32_t flags;

    return (authum_engine_client_is_connected(engine, index) &&
            authum_engine_entity_get_i32(engine, index, authum_entity_i32_flags, &flags) &&
            (flags & authum_entity_flag_fake_client) != 0)
               ? authum_bool_true
               : authum_bool_false;
}
