/**
 * @file extern.h
 * @brief C linkage for Authum's headers, so C++ modules can include them.
 *
 * Wrap a header's declarations in ::AUTHUM_EXTERN_C_BEGIN / ::AUTHUM_EXTERN_C_END
 * instead of the `#ifdef __cplusplus` / `extern "C" {` pair; in C both expand
 * to nothing.
 */

#ifndef AUTHUM_EXTERN_H
#define AUTHUM_EXTERN_H

#ifdef __cplusplus
/** `extern "C"` in C++, nothing in C — for a single declaration. */
#    define AUTHUM_EXTERN_C extern "C"
/** Opens a block of C-linkage declarations (`extern "C" {` in C++). */
#    define AUTHUM_EXTERN_C_BEGIN                                                                 \
        extern "C"                                                                                 \
        {
/** Closes the block ::AUTHUM_EXTERN_C_BEGIN opened. */
#    define AUTHUM_EXTERN_C_END }
#else
#    define AUTHUM_EXTERN_C
#    define AUTHUM_EXTERN_C_BEGIN
#    define AUTHUM_EXTERN_C_END
#endif

#endif /* AUTHUM_EXTERN_H */
