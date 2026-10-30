/**
 * skeleton — the smallest Authum module. Copy this folder, rename `skeleton`
 * everywhere, and start writing.
 *
 * Every module exports two things:
 *   - authum_module_info: who it is and which loader ABI it was built for;
 *   - authum_module_api:  start (return false to refuse loading) and stop.
 *
 * In start, keep the host with authum_host_copy and open the logger. Lines
 * go to addons/authum/logs/skeleton/YYYY-MM-DD.log (levels: authum.cfg).
 *
 * Install: addons/authum/modules/skeleton/skeleton.so + a line "skeleton"
 * in addons/authum/modules.list. Check with `authum status`.
 */

#include <authum/client.h>

authum_host_t g_skeleton_host;
authum_logger_t g_skeleton_log;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "skeleton",
    .version = AUTHUM_VERSION(1, 0, 0),
    .date = "2026-10-22",
    .author = "Authum examples",
    .url = "",
    .tag = "SKELETON",
};

authum_bool_t
skeleton_start(authum_host_t *host)
{
    if (authum_null_eq(host))
    {
        return authum_bool_false;
    }
    authum_host_copy(&g_skeleton_host, host);
    authum_logger_init(&g_skeleton_log, &g_skeleton_host);
    authum_logger_info(&g_skeleton_log, "started");
    return authum_bool_true;
}

void
skeleton_stop(void)
{
    /* Authum drops subscriptions, commands and sandbox memory by itself. */
    authum_logger_info(&g_skeleton_log, "stopped");
    authum_logger_close(&g_skeleton_log);
    authum_host_clear(&g_skeleton_host);
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = skeleton_start,
    .stop = skeleton_stop,
};
