# Authum as the game library

`authum.so` is Authum **as the HLDS game library** — no Metamod in between.
The engine loads it where the game library used to be, Authum loads the real
game behind it and overlays its event hooks on the game's callbacks.

The layout is the same as the Metamod build, so the admin's files move over
unchanged:

```text
cstrike/addons/authum/
  authum.so                          ← the host image: the game library now
  gamedll.txt                        ← the real game library (one line)
  configs/                           ← Authum's: authum.cfg, modules.list
  logs/YYYY-MM-DD.log                ← Authum's log + every module's warnings
  modules/                           ← one sandbox per module (as in Metamod)
    qproto/qproto.so
```

## Install

1. Build it: `cmake --build build-linux32 --target authum_gamedll`
   (the image is `build-linux32/authum.so`).
2. Put it in `<game dir>/addons/authum/authum.so`.
3. Name the real game library in `<game dir>/addons/authum/gamedll.txt` —
   one line, the path relative to the game directory:

   ```text
   dlls/cs.so
   ```

4. Point the engine at Authum, in `<game dir>/liblist.gam`:

   ```text
   gamedll_linux "addons/authum/authum.so"
   ```

5. Modules and `configs/` come over as they are — the loader finds them
   beside the image, exactly as in the Metamod build.

## What the engine asks the game module for

The engine resolves more than the four entry points in the game module by
name. Authum answers them all, forwarding to the real game:

| Symbol | Who serves it | Note |
|--------|---------------|------|
| `GiveFnptrsToDll` | Authum | forwards to the game's |
| `GetEntityAPI2` / `GetEntityAPI` | Authum | the **old** `GetEntityAPI`: besides the table it sizes the entity hash table — without it the game divides by zero on the first entity |
| `GetNewDLLFunctions` | Authum | forwards to the game's |
| `worldspawn` | Authum | forwards to the game's |
| `GameDLLInit` | Authum | the game's own start-up (the game exports it as `_Z11GameDLLInitv`) |
| `DispatchSpawn` | Authum | the game's entity spawner (`_Z13DispatchSpawnP7edict_s`) |
| `Server_GetBlendingInterface` | Authum | forwards to the game's |
| entity class names (`info_player_start`, `func_breakable`, …) | Authum | one trampoline per class, see below |

Metamod-P serves `GameDLLInit` and `DispatchSpawn` itself; standing in for
Metamod, Authum does the same.

## The entity class names

For every map entity the engine looks the spawn function up **by its class
name** in the game module — the library it loaded, which is us. The names
live in the real game (`cs.so` exports `info_player_start`, `func_breakable`,
`weapon_ak47`, …), so Authum exports them too: one trampoline per class, each
forwarding to the game. With a class missing, the engine silently skips that
entity — and the game then dies on an empty entity hash (this is the same
failure Metamod-R hit on ReHLDS, see
[rehlds/Metamod-R#34](https://github.com/rehlds/Metamod-R/issues/34)).

The list is generated from the game library:

```sh
python3 tools/gen-game-classes.py /path/to/cs.so > src/authum/gamedll/game_classes.cpp
```

`src/authum/gamedll/game_classes.cpp` is checked in (the classes of Counter-Strike
1.6 — 195 of them); regenerate it for another game.

## Limits

- The entity class list is the game's: regenerate it for another game (or
  another mod) — see above.
- The engine's map loading, entity spawning and game rules are the real
  game's — Authum only overlays the callbacks it publishes events for.
- `authum` (the console command) is registered by Authum, the rest of the
  console is the engine's.

## Windows

The same sources, plus two things the Windows engine expects of a game DLL:

- **`GiveFnptrsToDll` is `__stdcall`** (HLSDK's `DLLEXPORT`), and it alone:
  the engine calls it so, and Authum calls the game's the same way. Its export
  name stays undecorated (`GiveFnptrsToDll`, not `GiveFnptrsToDll@8`): MinGW
  links with `--kill-at`. The other entry points are cdecl.
- **The export table sits in `.rdata`.** For save/restore the engine reads the
  game DLL's export names itself, from the section *named* `.rdata` — where
  MSVC puts the export table. GNU ld keeps it in `.edata`, and the engine then
  reads garbage and faults before the map. So the build renames the sections
  after linking (`.rdata` → `.rodata`, `.edata` → `.rdata`): only the names
  change, the loader ignores them.

Verified on Windows with engine builds 8684 and 10210: the map loads, the
modules run, `changelevel` works, `quit` exits cleanly.
