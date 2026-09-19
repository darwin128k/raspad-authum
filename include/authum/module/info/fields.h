/**
 * @file fields.h
 * @brief Member fields of ::authum_module_info_t.
 */

#ifndef AUTHUM_MODULE_INFO_FIELDS_H
#define AUTHUM_MODULE_INFO_FIELDS_H

/**
 * @def authum_module_info_fields(str_type, version_type, datetime_type)
 * @brief Injects the module ABI identity members.
 *
 * @param str_type      Type of name / author / url / logtag (::lh_str_cptr).
 * @param version_type  Type of the ABI and SemVer fields (::lh_version_t).
 * @param datetime_type Type of the timestamp field (::lh_datetime_t).
 */
#define authum_module_info_fields(str_type, version_type, datetime_type)                           \
    version_type abi;                                                                              \
    str_type name;                                                                                 \
    version_type version;                                                                          \
    datetime_type datetime;                                                                        \
    str_type author;                                                                               \
    str_type url;                                                                                  \
    str_type logtag

#endif /* AUTHUM_MODULE_INFO_FIELDS_H */
