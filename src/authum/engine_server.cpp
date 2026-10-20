/**
 * @file engine_server.cpp
 * @brief ::authum_engine_t ops — thin wrappers over host `g_engfuncs`.
 */

#include <authum/char.h>
#include <authum/engine.h>
#include <authum/null.h>
#include <authum/size.h>

#include <extdll.h>

#include <cstdio>
#include <cstring>

extern enginefuncs_t g_engfuncs;

namespace
{

void
authum_engine_get_game_dir_impl(authum_char_t *buf, authum_usize_t buf_size)
{
    char tmp[256];

    if (authum_null_eq(buf) || buf_size == 0u)
    {
        return;
    }
    buf[0] = '\0';
    tmp[0] = '\0';
    if (g_engfuncs.pfnGetGameDir != 0)
    {
        g_engfuncs.pfnGetGameDir(tmp);
    }
    std::snprintf(buf, (size_t)buf_size, "%s", tmp);
}

void
authum_engine_server_print_impl(const authum_char_t *text)
{
    if (authum_null_eq(text) || g_engfuncs.pfnServerPrint == 0)
    {
        return;
    }
    g_engfuncs.pfnServerPrint(text);
}

void
authum_engine_server_command_impl(const authum_char_t *cmd)
{
    if (authum_null_eq(cmd) || g_engfuncs.pfnServerCommand == 0)
    {
        return;
    }
    /* HLSDK typedef takes char*, not const char*. */
    g_engfuncs.pfnServerCommand(const_cast<char *>(cmd));
}

const authum_engine_t authum_engine_ops = {
    authum_engine_get_game_dir_impl,
    authum_engine_server_print_impl,
    authum_engine_server_command_impl,
};

} // namespace

extern "C" const authum_engine_t *
authum_engine(void)
{
    return &authum_engine_ops;
}
