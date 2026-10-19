/**
 * Minimal Authum client module — no lh includes, no liblh link.
 *
 * Build: cmake --build <dir> --target hello
 * Drop:  addons/authum/modules/hello/hello.so  +  modules.list line "hello"
 */

#include <authum/client.h>

static authum_host_t g_host;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "hello",
    .version = AUTHUM_VERSION(0, 1, 0),
    .date = "2026-10-19",
    .author = "Authum examples",
    .url = "",
    .logtag = "HELLO",
};

static authum_bool_t
hello_start(authum_host_t *host)
{
    if (host == 0) {
        return AUTHUM_FALSE;
    }
    g_host = *host;
    authum_host_log_line(&g_host, 0, "[hello] start");
    if (g_host.module_dir != 0) {
        authum_host_log_line(&g_host, 0, g_host.module_dir);
    }
    return AUTHUM_TRUE;
}

static void
hello_stop(void)
{
    authum_host_log_line(&g_host, 0, "[hello] stop");
    g_host.log = 0;
    g_host.engine_funcs = 0;
    g_host.globals = 0;
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = hello_start,
    .stop = hello_stop,
};
