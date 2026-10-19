/**
 * @file code.h
 * @brief Authum codes on top of ::lh_error_code_t, stored in the OS last-error slot.
 *
 * Numbers start after ::lh_os_error_code_* so a caller can tell an Authum
 * check from an lh-side OS check.
 */

#ifndef AUTHUM_ERROR_CODE_H
#define AUTHUM_ERROR_CODE_H

#include <lh/error/code.h>

/**
 * @def authum_error_code_ok
 * @brief Alias for the non-error sentinel (::lh_error_code_ok).
 */
#define authum_error_code_ok lh_error_code_ok

/**
 * @def authum_error_code_missing_info
 * @brief The image does not export ::AUTHUM_MODULE_INFO_SYM.
 */
#define authum_error_code_missing_info 0x20

/**
 * @def authum_error_code_invalid_info
 * @brief ::authum_module_info_t is null or has no name.
 */
#define authum_error_code_invalid_info 0x21

/**
 * @def authum_error_code_incompatible_abi
 * @brief Module ABI is not compatible with ::AUTHUM_MODULE_ABI.
 */
#define authum_error_code_incompatible_abi 0x22

/**
 * @def authum_error_code_attach_failed
 * @brief Optional ::AUTHUM_MODULE_ATTACH_SYM returned false.
 */
#define authum_error_code_attach_failed 0x23

/**
 * @def authum_error_code_bad_modules_list
 * @brief ::AUTHUM_MODULES_LIST could not be parsed (Authum list dialect).
 */
#define authum_error_code_bad_modules_list 0x24

/**
 * @typedef authum_error_code_t
 * @brief Same representation as ::lh_error_code_t for Authum last-error codes.
 */
typedef lh_error_code_t authum_error_code_t;

#endif /* AUTHUM_ERROR_CODE_H */
