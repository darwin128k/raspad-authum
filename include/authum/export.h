/**
 * @file export.h
 * @brief DLL / .so export attribute for Authum module symbols.
 */

#ifndef AUTHUM_EXPORT_H
#define AUTHUM_EXPORT_H

#ifndef AUTHUM_MODULE_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#        define AUTHUM_MODULE_EXPORT __declspec(dllexport)
#    elif defined(__GNUC__)
#        define AUTHUM_MODULE_EXPORT __attribute__((visibility("default")))
#    else
#        define AUTHUM_MODULE_EXPORT
#    endif
#endif

/**
 * @brief Export of the host image's own entry points.
 *
 * The host image is the game library: the engine resolves its symbols
 * by name (GiveFnptrsToDll, the entity class names, …), so they must be
 * exported. On Windows that means __declspec(dllexport) — a visibility
 * attribute is not enough for a DLL.
 */
#ifndef AUTHUM_IMAGE_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#        define AUTHUM_IMAGE_EXPORT __declspec(dllexport)
#    elif defined(__GNUC__)
#        define AUTHUM_IMAGE_EXPORT __attribute__((visibility("default")))
#    else
#        define AUTHUM_IMAGE_EXPORT
#    endif
#endif

#endif /* AUTHUM_EXPORT_H */
