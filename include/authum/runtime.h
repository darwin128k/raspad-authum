#ifndef AUTHUM_RUNTIME_H
#define AUTHUM_RUNTIME_H

#include <authum/extern.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Fill LH's runtime slots over the C library.
 *
 * LH's core uses no C library and starts with every runtime slot empty. Every
 * image built with Authum carries its own copy of LH — authum.so and each
 * module (authum_client links LH statically and keeps it private) — and each
 * copy needs its slots filled: `malloc` / `free` / `realloc` for the allocator,
 * `abort` for the terminate handler, and a one-line-per-record `stderr` sink
 * for the logger (in authum.so the log, until authum_log_init routes it to the
 * channels).
 *
 * authum_client does this by itself when an image loads, before any of its
 * code runs (a load-time constructor in this object, which authum_client's
 * link options always pull in). Calling it again is harmless; the host does
 * so first thing in GiveFnptrsToDll to say where Authum takes LH's host role.
 * Do not call it while other threads use LH.
 */
void
authum_runtime_install(void);

/**
 * @brief Point LH's runtime logger back at `stderr`.
 *
 * What the host's log falls back to when its channels close
 * (authum_log_deinit). Call it only while no other thread logs through LH.
 */
void
authum_runtime_use_stderr_logger(void);

AUTHUM_EXTERN_C_END

#endif
