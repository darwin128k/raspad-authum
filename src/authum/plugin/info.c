#include <authum/plugin/info.h>
#include <lh/date/initializer.h>
#include <lh/datetime/initializer.h>
#include <lh/time/initializer.h>
#include <lh/version/initializer.h>

authum_plugin_info_t authum_plugin_info = {
    .name = "Authum",
    .version = lh_version_initializer(0, 1, 0),
    .datetime = lh_datetime_initializer(lh_date_initializer(2026, 9, 16),
                                        lh_time_initializer(0, 0, 0)),
    .author = "Authum",
    .url = "",
    .logtag = "AUTHUM",
};
