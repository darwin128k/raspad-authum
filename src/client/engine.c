#include <authum/engine.h>

#include <stdio.h>

void
authum_engine_get_game_dir(const authum_engine_t *engine, authum_char_t *buf,
                           authum_usize_t buf_size)
{
    if (!authum_null_eq(engine) && engine->get_game_dir != 0)
    {
        engine->get_game_dir(buf, buf_size);
    }
    else if (!authum_null_eq(buf) && buf_size > 0u)
    {
        buf[0] = '\0';
    }
}

void
authum_engine_server_print(const authum_engine_t *engine, const authum_char_t *text)
{
    if (!authum_null_eq(engine) && engine->server_print != 0 && !authum_null_eq(text))
    {
        engine->server_print(text);
    }
}

void
authum_engine_server_command(const authum_engine_t *engine, const authum_char_t *cmd)
{
    if (!authum_null_eq(engine) && engine->server_command != 0 && !authum_null_eq(cmd))
    {
        engine->server_command(cmd);
    }
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
