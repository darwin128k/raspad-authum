#include <authum/module.h>
#include <lh/date/initializer.h>
#include <lh/datetime/initializer.h>
#include <lh/time/initializer.h>
#include <lh/version/initializer.h>

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "Authum",
    .version = lh_version_initializer(1, 0, 0),
    .datetime = lh_datetime_initializer(lh_date_initializer(2026, 10, 19),
                                        lh_time_initializer(0, 0, 0)),
    .author = "Authum",
    .url = "",
    .logtag = "AUTHUM",
};
