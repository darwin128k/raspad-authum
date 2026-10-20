/**
 * @file engine.h
 * @brief HLDS engfuncs shared with Authum child modules (they do not get GiveFnptrsToDll).
 *
 * Returned as void* so this header stays free of HLSDK; cast to enginefuncs_t /
 * globalvars_t in the module.
 */

#ifndef AUTHUM_ENGINE_H
#define AUTHUM_ENGINE_H

#include <lh/compiler/extern/c.h>

#ifndef AUTHUM_MODULE_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#        define AUTHUM_MODULE_EXPORT __declspec(dllexport)
#    elif defined(__GNUC__)
#        define AUTHUM_MODULE_EXPORT __attribute__((visibility("default")))
#    else
#        define AUTHUM_MODULE_EXPORT
#    endif
#endif

LH_COMPILER_EXTERN_C_BEGIN

AUTHUM_MODULE_EXPORT void *
authum_engine_funcs(void);

AUTHUM_MODULE_EXPORT void *
authum_globals(void);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_ENGINE_H */
