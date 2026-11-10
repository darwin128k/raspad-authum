/**
 * @file private.h
 * @brief Private to `runtime.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_RUNTIME_PRIVATE_H
#define AUTHUM_API_RUNTIME_PRIVATE_H

#include <authum/runtime.h>
#include <lh/core/char/map.h>
#include <lh/core/compiler/constructor.h>
#include <lh/core/runtime/allocator.h>
#include <lh/core/runtime/logger.h>
#include <lh/core/runtime/terminate.h>
#include <lh/core/str/format/text.h>
#include <lh/core/util/str/ptr.h>
#include <lh/os/rail/base.h>
#include <lh/rail/runtime/base.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The C functions take no context, so they are wrapped rather than cast to the callback
 * types: calling a function through a pointer of another type is undefined behaviour.
 */
lh_ptr
authum_runtime_alloc(lh_self_ptr self, lh_usize_t size);

/**
 * @brief Free @p ptr with libc `free`: the runtime allocator's release callback.
 */
lh_void
authum_runtime_dealloc(lh_self_ptr self, lh_ptr ptr);

/**
 * @brief Resize @p ptr with libc `realloc`: the runtime allocator's resize callback.
 */
lh_ptr
authum_runtime_realloc(lh_self_ptr self, lh_ptr ptr, lh_usize_t old_size, lh_usize_t new_size);

/**
 * @brief End the process with libc `abort`: the runtime terminate handler.
 */
lh_void
authum_runtime_abort(lh_self_ptr self);

/**
 * @brief One record, one line on stderr; a record too long for the line is written as its format
 * mask, so it still says something.
 */
lh_ssize_t
authum_runtime_log_stderr(lh_self_ptr self, lh_logger_level_t level, lh_str_cptr fmt, va_list args);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_RUNTIME_PRIVATE_H */
