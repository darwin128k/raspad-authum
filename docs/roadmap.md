# Authum roadmap

What the owner wants next for Authum itself. Every item says what exists today
and what the target is. LH's own targets are in
[lib/lh/docs/roadmap.md](../lib/lh/docs/roadmap.md); a rule that changes how the
code is written goes into [AGENTS.md](../AGENTS.md). This file is the target, not a
debt list.

## 1. The script layer (cnor)

**Today.** The design is decided ([scripting.md](scripting.md)): the bridge is an
ordinary module that embeds the cnor VM and hands it a table of natives; the core
knows nothing about scripts.

**Target.** The bridge module (the VM and the natives), the decision path (the
bridge hangs a C hook, the script answers), and the script standard library
(`authum.cn`, `rules.cn`).

## 2. The connect gate decides

**Today.** `host/gate.c` is scaffolding ([gate.md](gate.md)).

**Target.** One owner of the "let in / keep out" decision, named by the config;
the other modules observe it.

## 3. A service registry

**Today.** The only channel between modules is the event bus by name: a module
cannot get another module's API. This is on purpose for now.

**Target.** Services after the source mark: a module publishes a table, another
asks for it by name and version, and the host knows who holds what.

## 4. Modules see the kind of an SQL failure

**Today.** lh classifies every backend failure (`lh_sql_error_kind_t`,
`lh_sql_db_get_error_kind` / `_get_error_code`), but `authum_sql_thread_result_t`
and `authum_sql_t` are frozen, so a module sees only the text.

**Target.** A getter on `authum_sql_t` and a new result structure: a minor ABI
step, when the owner schedules it.

## 5. A module's files carry their owner

**Today.** A module reads and writes its files through `authum_file_t` (a facade over
`lh/os/fs/file.h`), but the host does not know which module a file belongs to.

**Target.** Files opened under a module's region, so the host can name the owner in
its logs and close what a module forgot at unload.

## 6. The RASPAD modules move to ABI 4.0

**Today.** fastdl, gatekeeper, monitoring and qproto (outside this repository) are
built for 3.x.

**Target.** `sandbox` → `region`, `fopen` → `authum_file_t`, in one pass per module.

## 7. The engine's own log is visible on time

**Today.** Original HLDS writes its primary log (`logs/L*.log`) through a buffer it
does not flush: a running server's log looks cut at 4096 bytes until the map changes.
This is the engine, not Authum (reproduced without Authum), and the SDK gives no handle
to that file. A preload prototype that queues and flushes the writes on its own thread
works ([the investigation](reports/hlds-log-buffering/follow-up.md)).

**Target.** An opt-in adapter with process lifetime that owns the primary log's
writes, verified during an active map, across 4096 bytes, at map change and shutdown,
under queue overflow, with and without Authum. No engine offsets, no per-frame flushing.

## Decisions kept here

- **`authum_detour_attach` stays a separate tool.** Nobody in Authum calls it, but
  `raspad-qproto` patches engine code with it (`Steam_NotifyClientConnect`: a 5-byte
  `jmp rel32`). The hook chain does not replace it: hooks swap the
  `DLL_FUNCTIONS` tables, a detour patches code. Look again when qproto moves to
  ABI 4.0: does it still belong in the public API?
- **`include/authum/symbol.h` names the engine library per platform.** A public
  Authum header may not include LH, so the platform is named there on purpose;
  `tools/check-native.py` exempts it and `include/authum/export.h`.
