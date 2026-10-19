/**
 * @file module.c
 * @brief qproto Authum client module (ABI 0.2) — no lh ops table.
 */

#include "bridge.h"

#include <authum/client.h>

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = qproto_authum_start,
    .stop = qproto_authum_stop,
};
