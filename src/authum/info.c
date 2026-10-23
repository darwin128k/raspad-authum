/*
 * The host's own identity — the object the loader binds the host image
 * through (::authum_module_info). Both builds export it: the Metamod plugin
 * (authum_mm.so) and the standalone game library (authum.so). The image must
 * export the symbol: the loader reads it from the host image, and an
 * undefined one keeps the .so from loading at all.
 */
#include <authum/module/info.h>

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "Authum",
    .version = AUTHUM_VERSION(3, 0, 0),
    .date = "2026-10-23",
    .author = "Authum",
    .url = "",
    .logtag = "AUTHUM",
};
