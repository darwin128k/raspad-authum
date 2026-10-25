#ifndef AUTHUM_THREAD_H
#define AUTHUM_THREAD_H
#include <authum/abi.h>
#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/ptr.h>
#include <authum/size.h>
AUTHUM_EXTERN_C_BEGIN
typedef struct authum_thread authum_thread_t;
typedef void (*authum_thread_fn)(authum_ptr user);
enum { AUTHUM_THREAD_BLOCK_SIGPIPE = 1u };
typedef struct authum_thread_ops {
    authum_usize_t size;
    authum_thread_t *(*start)(authum_ptr owner, authum_thread_fn run,
                             authum_thread_fn wake, authum_ptr user, int cpu,
                             unsigned flags, authum_char_t *error, authum_usize_t error_size);
    authum_bool_t (*stop)(authum_ptr owner, authum_thread_t *thread);
} authum_thread_ops_t;
typedef struct authum_threads {
    const authum_thread_ops_t *ops;
    authum_ptr owner;
} authum_threads_t;
/** Owner/game thread only. run executes on a private OS thread and must not
 * call game-thread-only APIs. wake executes ON THE OWNER THREAD, must be
 * nonblocking/idempotent, must not call thread-management APIs, and must cause run to return (e.g. write a wake pipe).
 * The host wakes ALL owner threads, then joins ALL, BEFORE module stop.
 * Module stop may then release shared state and stop/free its handles.
 * Remaining handles are reclaimed after stop, including failed module starts.
 * No forced cancellation: run must use bounded I/O. max 64 handles/owner.
 * cpu=-1 inherits affinity; flags=0 preserves the inherited signal mask.
 * On failure, no callback runs and ownership remains with the caller. */
authum_thread_t *authum_thread_start(const authum_threads_t *threads,
    authum_thread_fn run, authum_thread_fn wake, authum_ptr user, int cpu,
    unsigned flags, authum_char_t *error, authum_usize_t error_size);
/** wake + join + release; called on the owner thread. Invalidates the handle.
 * Safe after the host already joined this owner's threads before module stop. */
authum_bool_t authum_thread_stop(const authum_threads_t *threads, authum_thread_t *thread);
AUTHUM_EXTERN_C_END
#endif
