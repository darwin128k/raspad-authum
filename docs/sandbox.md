# Sandbox memory and arrays

Every loaded module gets a **sandbox**: a memory root owned by Authum. Anything
the module allocates from it is freed by the host right after the module's
`stop` — whether the module freed it or not. A module never has to think "did I
release that object before unload?".

Headers: `<authum/sandbox.h>`, `<authum/array.h>`. Implementation on the host
side: lh's ownership tree (`lh_memory_tree`, talloc model) — freeing a block
frees everything it owns, destructors run first.

What it is not: isolation. The module runs in the server process like any
game library; a module that crashes takes the server down. The sandbox
guarantees one thing — no memory of the module survives its unload.

Cost: each block carries a 32-byte header, and an allocate + free pair costs
about twice a bare `malloc` + `free` (some 40–55 ns more on small blocks,
i386, glibc). Nothing else: engine calls do not go through the sandbox.

---

## 1. Allocate from the module sandbox

```c
const authum_sandbox_t *sb = authum_host_sandbox(host);

mymod_state_t *state = authum_sandbox_alloc(sb, sizeof *state); /* zeroed */
mymod_conn_t *conn = authum_sandbox_alloc_under(sb, state, sizeof *conn);
```

| Call | Meaning |
|------|---------|
| `authum_sandbox_alloc(sb, size)` | Zeroed block owned by the module root |
| `authum_sandbox_alloc_under(sb, parent, size)` | Zeroed block owned by `parent` — freed with it |
| `authum_sandbox_realloc(sb, ptr, size)` | Resize (null `ptr` → alloc); the tail is not zeroed |
| `authum_sandbox_free(sb, ptr)` | Free now, with everything it owns |
| `authum_sandbox_set_destructor(sb, ptr, fn)` | `fn(ptr)` runs right before the block is freed — close files, sockets |

Freeing early is fine and often right (a disconnected client's data). Not
freeing is also fine — unload takes care of it.

A size of half the address space or more gets null back. Running out of memory
is not reported: the host ends the server process, so there is no null to
check after an ordinary allocation.

---

## 2. Your own sandbox (nested)

For shorter lifetimes — one map, one connection, one request — open a nested
sandbox and drop it in one call:

```c
authum_sandbox_t map_sb;

authum_sandbox_open(&map_sb, authum_host_sandbox(host));  /* on map start */
/* … allocate map data from &map_sb … */
authum_sandbox_close(&map_sb);                            /* on map end */
```

The nested sandbox is owned by its parent: forgetting `close` is not a leak, the
module root takes it down at unload.

`authum_sandbox_t` is a plain value (ops + root). Copying it is cheap and safe
while its root is alive.

---

## 3. Arrays

`authum_array_t` is a growable array of fixed-size items whose storage lives in
a sandbox:

```c
authum_array_t names;

authum_array_init(&names, authum_host_sandbox(host), sizeof(mymod_name_t));
mymod_name_t *n = authum_array_push(&names, authum_null);   /* zeroed item */
mymod_name_t *first = authum_array_at(&names, 0u);
authum_array_remove_at(&names, 0u);                         /* keeps order */
```

| Call | Meaning |
|------|---------|
| `init(arr, sandbox, item_size)` | Empty; allocates nothing yet |
| `push(arr, item)` | Copy `item` in (null → zeroed); returns the slot |
| `at(arr, i)` / `size(arr)` / `is_empty(arr)` | Access |
| `remove_at(arr, i)` / `clear(arr)` | Shrink (clear keeps capacity) |
| `reserve(arr, n)` | Pre-grow |
| `deinit(arr)` | Free now (optional) |

Pointers from `at` / `push` stay valid until the next call that grows or
shrinks the array.

---

## 4. Rules

- Use the sandbox for anything that lives longer than one function call.
- Use the stack for short scratch buffers.
- Don't `free()` sandbox blocks and don't `authum_sandbox_free()` libc blocks.
- Anything that is *not* memory (a `FILE *`, a socket, an engine hook) needs a
  destructor or an explicit release in `stop` — the sandbox only knows memory
  and destructors.
