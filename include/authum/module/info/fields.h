/**
 * @file fields.h
 * @brief Member fields of ::authum_module_info_t.
 */

#ifndef AUTHUM_MODULE_INFO_FIELDS_H
#define AUTHUM_MODULE_INFO_FIELDS_H

/**
 * @def authum_module_info_fields(str_type, version_type)
 * @brief Injects the module ABI identity members.
 */
#define authum_module_info_fields(str_type, version_type)                                          \
    version_type abi;                                                                              \
    str_type name;                                                                                 \
    version_type version;                                                                          \
    str_type date;                                                                                 \
    str_type author;                                                                               \
    str_type url;                                                                                  \
    str_type logtag

#endif /* AUTHUM_MODULE_INFO_FIELDS_H */
