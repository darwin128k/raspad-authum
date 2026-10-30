# Authum — how the core, the extensions and the modules fit

Decided on 2026-10-30 after a long discussion of the module mechanism. This
file records the decisions so the next session does not start them again. The
rules behind the code stay in `AGENTS.md` and `lib/lh/AGENTS.md`; this file
says what is built on top of them.

## 1. What Authum is

Authum is the **core** of a Half-Life server: the gamedll proxy, the host, the
module loader, the event bus, the log channels, the shared HTTP service. It is
a thin facade over lh — the bricks live in lh, Authum keeps the game.

- lh is a box of bricks; Authum is a thin game facade over them.
- Authum names no platform: no `_WIN32`, no libc, no native calls. `tools/check-native.py`
  keeps it that way.
- The next version breaks compatibility on purpose (0.4.0): the layout below is
  not frozen, so it is built once, properly.

## 2. One mechanism for modules

There is no separate class of "extension". A module is a shared library with
one exported entry point; lh already has the machinery
(`lh/os/module.h`, `lh/os/loader.h`, `lh/os/module/event/bus.h` — see
`lib/lh/docs/module-loader.md`).

A module may be both:

- a **consumer** of the core API (events, commands, tasks, threads, HTTP,
  players, entities), and
- a **provider** of API to other modules (a named service in a registry).

The second part does not exist yet: today the only module-to-module channel is
the event bus, by name. It appears when a module really needs it — and the
registry is keyed by name, so a consumer never learns who provides a service.

## 3. Three kinds of power, not a list of bans

Nothing in Authum is forbidden by a list. The limit is the API.

| power | who |
|---|---|
| observe | every module |
| change a value or a result | every module |
| refuse (do not let a player in, cancel an action) | the owner of the decision point |

The **owner of a decision point** is named in the configuration: the host asks
that one module, and the others observe the outcome. That is the gatekeeper
(`host/gate.c` today is a stub): it is not a privileged module, it is the module
that owns the point where a player is let in.

## 4. Events and hooks are different things

- **Events** (the bus): a subscriber reacts when something happened. It cannot
  change what happened. Two modes, not one: a *forward* (nobody may stop the
  chain) and a *query* (one owner answers).
- **Hooks**: a slot in a call chain that the engine runs in its own stack. A
  hook may change an argument or a result; refusing is a separate, explicit
  right. A hook is not an event, and it is not in the bus.

Today the host influences the game by replacing the `DLL_FUNCTIONS` /
`NEW_DLL_FUNCTIONS` tables (`gamedll/hooks.cpp`). `authum_detour_attach` is dead
code. When modules get hooks, they get a chain of slots with limits — the
budget is counted by the core, not by the module.

## 5. The game objects are handles, never pointers

A module never receives a pointer into the game. It receives a handle and a
field id: `authum_entity_try_get_float(index, field, &out)`,
`authum_client_get_userid(index)`. The escape hatches (`authum_host_t.engine_funcs`,
`.globals`) go away with the next version: they are the second, raw copy of the
same API, and they break the isolation.

## 6. Every output says who made it

The logs and the commands are the reference: owner → channel → tag, through a
single point each (`host/logs.c`, `host/commands.c` over the lh console).

The holes, to close:

- the publisher of an event is dropped (`host/events.c`);
- the engine ops table is one singleton for all modules, so `client_print`,
  chat, `cvar_set` and `kick` are anonymous (`host/engine_server.cpp`);
- the HTTP access line has no owner (`host/http/access.c`);
- `authum_module_info_t.logtag` is declared in the ABI and read by nobody.

## 7. The interpreter is a module

cnor stays a separate project, a small pocket language. Inside Authum it is an
ordinary module: the same ABI, the same loader.

- The natives the script calls are defined **once**, in the core — that is what
  keeps the API from growing per module.
- The bridge (core API → cnor natives) lives in the interpreter module, not in
  the core: the core does not know that cnor exists.
- A script sees only handles and values, never pointers; a script that runs in
  a frame has a step budget.
- cnor has no closures by design (nothing is allocated while running). A rule
  with state is a state plus functions, not a captured environment.

The four modules of the RASPAD server (gatekeeper, kuprot, monitoring, fastdl)
are the pilot: the first two rewritten on cnor, the last two after.

## 8. The plan

1. **0.4.0 into master** as it is; the new work happens on a branch.
2. **Clear the path** before building anything new — done:
   - `tools/check-native.py` to 0 (it reported 143 places);
   - owner marks on the outputs above;
   - the `_v` naming rule everywhere (`no suffix` = fast, `_v` = validating;
     `_unsafe` is gone).
3. **New bricks, then new mechanisms**: the hook chain (done — one road to a
   decision, [docs/hooks.md](hooks.md)), the service registry, the decision
   point — each a lh brick with a thin Authum facade.
4. **cnor**: the interpreter module, the natives, the standard library.

## 9. What stays as it is

- The event bus with its owner tags and its drop-on-unload.
- The log channels, the command registry, the placeholders, the sandbox, the
  module loader of lh.
- lh itself: it is the box of bricks, and it is not rewritten here.

## 10. The platform debt, measured

`tools/check-native.py` counts every place where Authum names the platform it
runs on. It started at **143** and stands at **0**:

| what | count | the brick that replaced it |
|---|---|---|
| `snprintf` / `vsnprintf` | 33 | `lh_str_ptr_format_cstr` / `..._cstr_v` (`lh/str/format/text.h`) |
| forbidden headers | 23 | `stdio.h` 14, `stdlib.h` 8, `string.h` 1 |
| file calls | 9 | `lh/os/system/fs/file.h` (`open`, `read`, `write`, `remove`) |
| libc string calls | 9 | `lh/str/ptr.h` (`lh_str_ptr_len`, `..._equals`, `..._find_of_char`) |
| clock calls | 3 | `lh/os/tick.h`, `lh/os/timestamp.h` |
| platform branches | 2 | the backend decides, not the caller |
| numeric parses | 2 | `lh/str/parse/*` |
| `getenv` | 1 | `lh/os/env.h` |

Every brick the table names already exists in lh — nothing had to be written
for it. Two differences mattered when replacing: the lh formatter leaves the
buffer **empty** when the result does not fit (libc truncates), and it knows
`%% %c %s %d %i %u %x %X` only — no `%f`, `%zu` or `%+d`.

`include/authum/export.h` and `include/authum/symbol.h` are exempt: they sit
on the ABI boundary with the host process.

## 11. The names, and the build

Three words, three meanings: `owner` is who (a subscription, a journal, a
decision), `tag` is the mark a source is shown with in a record, `name` is
what it is called. The module identity carries `tag`, not `logtag`; the module
ABI is 3.0.0 because of that rename, and no 2.x module loads.

The build runs on Windows in 32 bits (`cmake --preset windows-mingw32`). The
preset puts `C:\msys64\mingw32\bin` on the PATH — without it the i686 gcc
cannot find `cc1.exe` and fails with `0xC0000135` (a DLL it cannot locate,
not a missing package). WSL is the second path, not the first: it serves the
tree over a slow filesystem and it dropped its service once during a long
link.
