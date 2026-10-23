/**
 * @file ops.h
 * @brief Host ops table behind ::authum_sandbox_t (filled by `authum`).
 *
 * Modules never call these directly — use the `authum_sandbox_*` functions in
 * <authum/sandbox.h>. The host implements them on lh's ownership tree
 * (`lh_memory_tree`): every block has a parent, freeing a block frees what it
 * owns.
 */

#ifndef AUTHUM_SANDBOX_OPS_H
#define AUTHUM_SANDBOX_OPS_H

#include <authum/abi.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Runs on a block just before it is freed (close a file, drop a handle, …). */
    typedef void (*authum_sandbox_destructor_fn)(authum_ptr ptr);

    struct authum_sandbox_ops
    {
        /** sizeof this table as built by the host — see <authum/abi.h>. */
        authum_usize_t size;
        /** Zeroed block of @p size bytes owned by @p parent (never null on success). */
        authum_ptr (*alloc)(authum_ptr parent, authum_usize_t size);
        /** Resize keeping ownership; tail beyond the old size is not zeroed. */
        authum_ptr (*realloc)(authum_ptr ptr, authum_usize_t size);
        /** Free @p ptr and everything it owns. Null is ignored. */
        void (*free)(authum_ptr ptr);
        void (*set_destructor)(authum_ptr ptr, authum_sandbox_destructor_fn fn);
    };
    typedef struct authum_sandbox_ops authum_sandbox_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_SANDBOX_OPS_H */
