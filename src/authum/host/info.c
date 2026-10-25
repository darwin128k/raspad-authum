/*
 * The host's own identity — the object the loader binds the host image
 * through (::authum_module_info). The host image exports it — it is the
 * game library the engine loads. The image must
 * export the symbol: the loader reads it from the host image, and an
 * undefined one keeps the .so from loading at all.
 */
#include <authum/module/info.h>

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "Authum",
    .version = AUTHUM_VERSION(4, 2, 1),
    .date = "2026-10-25",
    .author = "Authum",
    .url = "",
    .logtag = "AUTHUM",
};
