# Standalone: Authum as the game library

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

1. Build it: `cmake --build build-linux32 --target authum_standalone`
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
| entity class names (`info_player_start`, …) | the game | see below |

Metamod-P serves `GameDLLInit` and `DispatchSpawn` itself; standing in for
Metamod, Authum does the same.

## The game library is a dependency

The engine resolves every map entity's spawn function by its **class name**
(`info_player_start`, `func_door`, …) *in the game module*. Those names live
in the real game library, so it has to be part of Authum's dependency tree:
`dlsym` on Authum's handle then finds them, and the entities are spawned.

The build records that dependency against a stub carrying the game's file
name — the real library is found at run time in the game directory's `dlls/`
(`$ORIGIN/../../dlls`, Authum sits in `<game dir>/addons/authum/`):

```sh
cmake -DAUTHUM_GAME_DLL_NAME=cs.so ...      # the default
```

The name must match the file in `<game dir>/dlls/` (`cs.so` for
Counter-Strike). `gamedll.txt` names the same library by path; the file name
is what the loader matches, so keep the two in step.

## Limits

- Linux (i386) only: the image is a game library for the original HLDS.
- The engine's map loading, entity spawning and game rules are the real
  game's — Authum only overlays the callbacks it publishes events for.
- `authum` (the console command) is registered by Authum, the rest of the
  console is the engine's.
