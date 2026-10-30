# Module hooks — the design

Decided on 2026-10-30. This is the design; the code follows it.

## 1. An event is not a hook

An **event** tells a module that something happened. The module reacts; the
game goes on. Today that is the whole story: `src/authum/gamedll/hooks.cpp`
publishes `authum.client.*` and `authum.server.*` and then calls the game's own
function, unless a handler set `allow = false` or `block = true`.

A **hook** is a module that stands *in* the call. It sees the arguments, may
change them, may change the result, and — with an explicit right — may stop the
call from reaching the game at all. That is what a "living rule" needs: a
kuprot that decides whether a player may spawn, a module that rewrites a
command before the game reads it.

So the two must not share a mechanism. An event is a message; a hook is a
place in the call chain.

## 2. What gets hooked

The points where Authum already stands in the chain — the wrappers in
`src/authum/gamedll/hooks.cpp`:

| point | game call | today |
|---|---|---|
| client connect | `ClientConnect` | event, `allow` may refuse |
| client put in server | `ClientPutInServer` | event |
| client info changed | `ClientUserInfoChanged` | event |
| client disconnect | `ClientDisconnect` | event |
| client kill | `ClientKill` | event, `allow` may refuse |
| client command | `ClientCommand` | event, `block` may swallow |
| cvar value | `CvarValue2` | event (answer) |
| server activate | `ServerActivate` | event |
| server deactivate | `ServerDeactivate` | event |
| server frame | `ServerFrame` | event |
| server precache | `PrecacheModel` | event |
| server shutdown | `ServerShutdown` | event |

The first hooked point is **client connect**: it already carries the decision
(`allow`, `reject_reason`) and it is where a module has to be right, not
merely fast.

Points that are not in the table yet (`PlayerSpawn`, `RunCmd`, `GiveFnptrsToDll`)
come later, one at a time, each with its own event first.

## 3. What a hook may do

| power | limit |
|---|---|
| read the arguments | free |
| change an argument (a name, a command, a value) | the game sees the change |
| change the result | the game sees the change |
| stop the call | only with an explicit right, and the reason is logged |
| run long | **budget in the core** |

The core counts the time a hook spends and drops the hook that overran it, so
one module cannot stall the game loop. The same for a module that crashes:
nothing can catch a crash in the same process, so the limit is not "catch it"
but "the hook does not hold the frame" — the budget is what protects the game.

## 4. The chain

- The core owns the chain: a slot per point, the handlers in it, and the
  game's own function at the end.
- **Order is LIFO**: the module that hooked last is called first (as in
  Metamod). The host's own wrapper is always last.
- A handler that changed an argument passes the changed value on; the next
  handler sees the change.
- A handler that stops the call ends the chain, the game function is not
  called, and the core logs who stopped it and why.
- Unloading a module removes its handlers from every slot — the same rule the
  event bus already follows for subscriptions.

## 5. The module API

```c
/* The points a module may hook. */
typedef enum authum_hook_point
{
    authum_hook_point_client_connect = 0,
    authum_hook_point_client_kill,
    authum_hook_point_client_command,
    authum_hook_point_count
} authum_hook_point_t;

/* A handler: it may change what it is given and say whether to continue. */
typedef authum_bool_t (*authum_hook_fn)(authum_ptr user, authum_hook_call_t *call);

/* Add a handler to a point; the module's own owner is taken from the host. */
authum_bool_t
authum_host_hook_add(const authum_host_t *host, authum_hook_point_t point, authum_hook_fn fn,
                     authum_ptr user);

authum_bool_t
authum_host_hook_remove(const authum_host_t *host, authum_hook_point_t point, authum_hook_fn fn,
                        authum_ptr user);
```

`authum_hook_call_t` carries the arguments and the result of the point, so a
handler reads and writes them in one place; it names no HL type and no game
type.

## 6. How it sits next to the events

The event stays what it is: a message about what happened. The hook is the
place in the call. A module may use both: the event to know *that* a player
connected, the hook to decide *what happens next*. The event bus is not
extended with hook powers, and the hook chain is not fed by the event bus.

The `allow` / `reject_reason` / `block` fields of the client events stay as they
are for now: they are the event's answer to the host, and the host reads them
after the chain has run. When the hook chain for client connect is in place,
the refusal moves into the chain (with its own author), and the event fields
go away with it.

## 7. The order of the work

1. The chain in the core: slots, order, budget, removal on unload — with tests.
2. The module API: `authum_host_hook_add` / `_remove` — with tests.
3. The first point: client connect. The event's `allow` moves into the chain.
4. The rest of the points, one at a time, each with its event first.

Nothing lands half-done: a chain with no hooked point is dead code, so step 1
and step 2 are only committed together with step 3.
