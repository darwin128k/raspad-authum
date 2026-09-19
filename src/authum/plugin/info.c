#include <authum/module/info.h>
#include <lh/date/initializer.h>
#include <lh/datetime/initializer.h>
#include <lh/time/initializer.h>
#include <lh/version/initializer.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#    define AUTHUM_MODULE_EXPORT __declspec(dllexport)
#else
#    define AUTHUM_MODULE_EXPORT
#endif

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "Authum",
    .version = lh_version_initializer(0, 1, 0),
    .datetime = lh_datetime_initializer(lh_date_initializer(2026, 9, 16),
                                        lh_time_initializer(0, 0, 0)),
    .author = "Authum",
    .url = "",
    .logtag = "AUTHUM",
};
