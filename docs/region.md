# Region memory and arrays

Every loaded module gets a **region**: a memory root owned by Authum. Anything
the module allocates from it is freed by the host right after the module's
`stop` — whether the module freed it or not. A module never has to think "did I
release that object before unload?".

Headers: `<authum/region.h>`, `<authum/array.h>`. Implementation on the host
side: lh's ownership tree (`lh_memory_tree`, talloc model) — freeing a block
frees everything it owns, destructors run first.

What it is not: isolation. The module runs in the server process like any
game library; a module that crashes takes the server down. The region
guarantees one thing — no memory of the module survives its unload.

Cost: each block carries a 32-byte header, and an allocate + free pair costs
about twice a bare `malloc` + `free` (some 40–55 ns more on small blocks,
i386, glibc). Nothing else: engine calls do not go through the region.

---

## 1. Allocate from the module region

```c
const authum_region_t *region = authum_host_get_region(host);

mymod_state_t *state = authum_region_alloc(region, sizeof *state); /* zeroed */
mymod_conn_t *conn = authum_region_alloc_under(region, state, sizeof *conn);
```

| Call | Meaning |
|------|---------|
| `authum_region_alloc(region, size)` | Zeroed block owned by the module root |
| `authum_region_alloc_under(region, parent, size)` | Zeroed block owned by `parent` — freed with it |
| `authum_region_realloc(region, ptr, size)` | Resize (null `ptr` → alloc); the tail is not zeroed |
| `authum_region_free(region, ptr)` | Free now, with everything it owns |
| `authum_region_set_destructor(region, ptr, fn)` | `fn(ptr)` runs right before the block is freed — close files, sockets |

Freeing early is fine and often right (a disconnected client's data). Not
freeing is also fine — unload takes care of it.

A size of half the address space or more gets null back. Running out of memory
is not reported: the host ends the server process, so there is no null to
check after an ordinary allocation.

---

## 2. Your own region (nested)

For shorter lifetimes — one map, one connection, one request — open a nested
region and drop it in one call:

```c
authum_region_t map_region;

authum_region_open(&map_region, authum_host_get_region(host));  /* on map start */
/* … allocate map data from &map_region … */
authum_region_close(&map_region);                            /* on map end */
```

The nested region is owned by its parent: forgetting `close` is not a leak, the
module root takes it down at unload.

`authum_region_t` is a plain value (ops + root). Copying it is cheap and safe
while its root is alive.

---

## 3. Arrays

`authum_array_t` is an opaque, growable array of fixed-size items. The array
is a block of a region and its storage hangs under that block (inside it is
lh's `lh_array_t` with the region as its allocator):

```c
authum_array_t *names =
    authum_array_create(authum_host_get_region(host), sizeof(mymod_name_t));
mymod_name_t *n = authum_array_push(names, authum_null);   /* zeroed item */
mymod_name_t *first = authum_array_get_at(names, 0u);
authum_array_remove_at(names, 0u);                         /* keeps order */
```

| Call | Meaning |
|------|---------|
| `create(region, item_size)` | Empty array, or null; storage not allocated yet |
| `push(arr, item)` | Copy `item` in (null → zeroed); returns the slot |
| `get_at(arr, i)` / `get_size(arr)` / `get_capacity(arr)` / `is_empty(arr)` | Access |
| `remove_at(arr, i)` / `clear(arr)` | Shrink (clear keeps capacity) |
| `reserve(arr, n)` | Pre-grow |
| `destroy(arr)` | Free the array and its storage now (optional) |

Pointers from `at` / `push` stay valid until the next call that grows or
shrinks the array.

---

## 4. Rules

- Use the region for anything that lives longer than one function call.
- Use the stack for short scratch buffers.
- Don't `free()` region blocks and don't `authum_region_free()` libc blocks.
- Anything that is *not* memory (a `FILE *`, a socket, an engine hook) needs a
  destructor or an explicit release in `stop` — the region only knows memory
  and destructors.
