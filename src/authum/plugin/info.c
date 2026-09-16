#include <authum/plugin/info.h>
#include <lh/version/initializer.h>

authum_plugin_info_t authum_plugin_info = {
    .name = "Authum",
    .version = lh_version_initializer(0, 1, 0),
    .date = "2026/09/16",
    .author = "Authum",
    .url = "",
    .logtag = "AUTHUM",
};
