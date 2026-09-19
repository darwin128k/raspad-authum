/**
 * @file fields.h
 * @brief Member fields of ::authum_module_t.
 *
 * Prefix is ::lh_os_shared_module_t (handle, owned, children). `info` is
 * Authum identity on top of that image.
 */

#ifndef AUTHUM_MODULE_FIELDS_H
#define AUTHUM_MODULE_FIELDS_H

#include <lh/os/shared/module/fields.h>

/**
 * @def authum_module_fields(handle_type, owned_type, info_type, vector_type)
 * @brief Injects the shared-image prefix plus identity.
 *
 * @param handle_type Type of the OS handle (::lh_os_shared_handle_t).
 * @param owned_type  Type of the close-ownership flag (::lh_bool_t).
 * @param info_type   Type of the identity pointer (`const authum_module_info_t *`).
 * @param vector_type Type of the child table (::lh_vector_t).
 */
#define authum_module_fields(handle_type, owned_type, info_type, vector_type)                      \
    lh_os_shared_module_fields(handle_type, owned_type, vector_type);                              \
    info_type info

#endif /* AUTHUM_MODULE_FIELDS_H */
