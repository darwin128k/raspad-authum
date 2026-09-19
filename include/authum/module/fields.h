/**
 * @file fields.h
 * @brief Member fields of ::authum_module_t.
 */

#ifndef AUTHUM_MODULE_FIELDS_H
#define AUTHUM_MODULE_FIELDS_H

/**
 * @def authum_module_fields(handle_type, info_type)
 * @brief Injects the OS handle and identity pointer.
 *
 * @param handle_type Type of the OS handle (::lh_os_shared_handle_t).
 * @param info_type   Type of the identity pointer (`const authum_module_info_t *`).
 */
#define authum_module_fields(handle_type, info_type)                                               \
    handle_type handle;                                                                            \
    info_type info

#endif /* AUTHUM_MODULE_FIELDS_H */
