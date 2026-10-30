# The script layer — how a script reaches the game

Decided on 2026-10-30. This is the design; the code follows it.

## 1. Where the line runs

- **The core knows nothing about scripts.** No cnor in `lib/lh`, no script
  types in `include/authum/`, no script branch in the host. Authum stays a
  thin facade over lh.
- **The bridge is a module.** It is an ordinary Authum module — the same ABI,
  the same loader — that happens to embed the cnor VM and hand it a table of
  natives. It uses the public API like any other module: `authum_host_subscribe`,
  `authum_hooks_add`, `authum_logger_*`, `authum_file_*`, `authum_host_client_print`.
- **The standard library is scripts**, not C: a set of cnor modules that a
  server ships and a script imports.

So there is no second road: a script reaches the game through the same API a C
module uses. The bridge only translates values.

## 2. What a script sees

The native table is the whole surface. It is small on purpose — a script has
no pointers, so everything is a value, a name or a handle.

| native | what it does |
|---|---|
| `log.info(text)` / `log.warning(text)` / `log.error(text)` | the module's own log channel, with the module's mark |
| `player.name(index)` / `player.userid(index)` | who is that slot |
| `player.print(index, text)` / `player.chat(index, sender, text)` | say something to a player |
| `player.kick(index, reason)` | remove a player |
| `time.unix()` / `time.local()` | the clock |
| `config.get(key)` / `config.set(key, value)` | the module's own config |
| `file.read(path)` / `file.write(path, text)` | the module's own files, through `authum_file_*` |
| `http.get(url)` — later | the shared HTTP service |

Values cross the boundary as numbers, strings, booleans, lists and maps. A
player is an **index**, never a pointer — the same rule the C API follows.

## 3. How a script changes the game

A script cannot hand the core a C function, so it cannot hang a handler on a
hook by itself. The bridge does it for the script:

1. The bridge hangs **one** C handler on the point (client connect, client
   command, …).
2. When the point comes, the handler asks the script: "what do you decide?"
3. The script returns an answer — allow or refuse, with a reason.
4. The handler turns that answer into the chain's `proceed` and `reject`.

That is the whole "living rule" path: a script decides, the bridge carries the
decision, the core applies it, and the log says whose decision it was.

The budget is the core's: the chain counts the time a handler spends, and the
script's time is inside it. The bridge therefore keeps its own instruction
budget for one answer and must not let a script loop in a game frame.

## 4. The standard library

The standard library is scripts that ship with the server:

- `authum.cn` — the everyday helpers (the mark, the player list, the config);
- `rules.cn` — the shape a "rule module" takes: the events it listens to and
  the decisions it may make.

A script imports them the way any cnor script does — `import authum`. The
modules are plain cnor modules, so they are read, checked and versioned like
any other script.

## 5. The order of the work

1. The bridge module: embed the VM, take a script, run it on demand.
2. The native table: the table above, one function per row.
3. The decision path: the bridge's handler on the points, the script's answer.
4. The standard library: `authum.cn` and `rules.cn`.
5. The server's own rules: the ones the RASPAD server actually runs, written in
   cnor instead of C.

## 6. What this does not change

- The module ABI (3.0.0) and the host tables.
- The event bus, the hooks and the log channels: a script uses them through
  the bridge, not beside them.
- The core's promise: it names no platform, and it knows nothing about the
  language a script is written in.
