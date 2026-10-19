/**
 * Minimal Authum module — host facade only (architecture B + escape hatch).
 *
 * Shared with the process: authum_host_log / engfuncs via authum_host_bind.
 * Not shared: embedding full lh in this .so (optional; this example does not).
 */

#include <authum/host.h>
#include <authum/module/info.h>

#include <lh/attribute/unused.h>
#include <lh/date/initializer.h>
#include <lh/datetime/initializer.h>
#include <lh/null.h>
#include <lh/time/initializer.h>
#include <lh/version/initializer.h>

static authum_host_t g_host;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "hello",
    .version = lh_version_initializer(0, 1, 0),
    .datetime = lh_datetime_initializer(lh_date_initializer(2026, 10, 19),
                                        lh_time_initializer(0, 0, 0)),
    .author = "Authum examples",
    .url = "",
    .logtag = "HELLO",
};

static lh_bool_t
hello_start(authum_module_t *self)
{
    if (!authum_host_bind(self, &g_host)) {
        return lh_bool_false;
    }
    authum_host_log_line(&g_host, 0, "[hello] start");
    return lh_bool_true;
}

static void
hello_stop(authum_module_t *self)
{
    authum_host_log_line(&g_host, 0, "[hello] stop");
    g_host.log = lh_null;
    g_host.engine_funcs = lh_null;
    g_host.globals = lh_null;
    LH_ATTRIBUTE_UNUSED(self);
}

AUTHUM_MODULE_EXPORT const lh_os_module_ops_t authum_module_ops = {
    .start = hello_start,
    .stop = hello_stop,
};
