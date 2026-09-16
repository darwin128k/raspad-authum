/**
 * @file info.h
 * @brief Plugin identity (::authum_plugin_info_t).
 *
 * Authum's own description of the plugin — name, SemVer, date, author, URL,
 * logtag. This is not Metamod's `plugin_info_t`; that table is filled later
 * from this one (`version` formatted via ::lh_version_format).
 */

#ifndef AUTHUM_PLUGIN_INFO_H
#define AUTHUM_PLUGIN_INFO_H

#include <authum/plugin/info/view/fields.h>
#include <lh/str/ptr.h>
#include <lh/version.h>

/**
 * @struct authum_plugin_info
 * @brief Identity the plugin reports about itself.
 *
 * Fields are injected via ::authum_plugin_info_fields.
 */
struct authum_plugin_info
{
    authum_plugin_info_fields(lh_str_cptr, lh_version_t);
};

/**
 * @typedef authum_plugin_info_t
 * @brief Alias for `struct authum_plugin_info`.
 */
typedef struct authum_plugin_info authum_plugin_info_t;

#endif /* AUTHUM_PLUGIN_INFO_H */
