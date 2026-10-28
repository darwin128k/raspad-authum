# Memory ownership

`<authum/memory.h>` exposes the explicit-lifetime heap of the current image:
`authum_memory_alloc`, `authum_memory_alloc_zero`, `authum_memory_realloc` and
`authum_memory_free`. These functions use the existing LH runtime allocator;
there is no separate libc heap implementation in Authum. The host and each
statically linked client image retain their own LH runtime configuration.

Allocation failure returns `authum_null`. Zero dimensions and multiplication
overflow are rejected before allocating. Reallocation takes the old byte size;
on failure the caller still owns the original block. A zero new size releases
it. Allocation and release must use compatible allocator callbacks and context;
do not change the runtime heap while blocks exist, or exchange blocks between
images. Worker threads exchanging blocks must have compatible runtime heaps.

Use the sandbox for module-owned objects that must be reclaimed at unload.
Its ownership tree remains in LH, and its backing heap goes through the Authum
memory facade too. Heap blocks and sandbox blocks have different ownership and
must be released by their matching API.

Authum implementation code uses LH byte kernels (`lh_memory_std_copy`,
`lh_memory_std_move`, `lh_memory_std_set`, `lh_memory_std_compare`). Pure module
headers expose Authum types without depending on LH headers. Native APIs and
third-party libraries keep their own allocation contracts.

To investigate allocations, put a breakpoint on the Authum heap functions or
instrument the LH runtime allocator callbacks. Tests inject an allocator,
force allocation/reallocation failures, and verify zero initialization,
byte preservation, overflow rejection and release through those callbacks.

Run `python tools/check-conventions.py` to check the Authum foundation boundaries.
