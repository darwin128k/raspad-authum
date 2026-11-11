/**
 * @file ops.h
 * @brief Host ops table behind ::authum_region_t (filled by `authum`).
 *
 * Modules never call these directly — use the `authum_region_*` functions in
 * <authum/region.h>. The host implements them on lh's ownership tree
 * (`lh_memory_tree`): every block has a parent, freeing a block frees what it
 * owns.
 */

#ifndef AUTHUM_REGION_OPS_H
#define AUTHUM_REGION_OPS_H

#include <authum/size.h>
#include <authum/extern.h>
#include <authum/ptr.h>

AUTHUM_EXTERN_C_BEGIN

/** Runs on a block just before it is freed (close a file, drop a handle, …). */
typedef void (*authum_region_destructor_fn)(authum_ptr ptr);

AUTHUM_EXTERN_C_END

struct authum_region_ops
{
    /** Zeroed block of @p size bytes owned by @p parent (never null on success). */
    authum_ptr (*alloc)(authum_ptr parent, authum_usize_t size);
    /** Resize keeping ownership; tail beyond the old size is not zeroed. */
    authum_ptr (*realloc)(authum_ptr ptr, authum_usize_t size);
    /** Free @p ptr and everything it owns. Null is ignored. */
    void (*free)(authum_ptr ptr);
    void (*set_destructor)(authum_ptr ptr, authum_region_destructor_fn fn);
};
typedef struct authum_region_ops authum_region_ops_t;

#endif /* AUTHUM_REGION_OPS_H */
