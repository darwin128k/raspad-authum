/**
 * @file fields.h
 * @brief Member fields of ::authum_plugin_info_t.
 */

#ifndef AUTHUM_PLUGIN_INFO_FIELDS_H
#define AUTHUM_PLUGIN_INFO_FIELDS_H

/**
 * @def authum_plugin_info_fields(str_type, version_type)
 * @brief Injects the plugin identity members.
 *
 * @param str_type     Type of name / date / author / url / logtag (::lh_str_cptr).
 * @param version_type Type of the SemVer field (::lh_version_t).
 */
#define authum_plugin_info_fields(str_type, version_type)                                          \
    str_type name;                                                                                 \
    version_type version;                                                                          \
    str_type date;                                                                                 \
    str_type author;                                                                               \
    str_type url;                                                                                  \
    str_type logtag

#endif /* AUTHUM_PLUGIN_INFO_FIELDS_H */
