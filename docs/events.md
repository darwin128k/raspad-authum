# Events: how modules talk

Authum modules never call each other directly: no `dlsym`, no shared function
tables, no pointers into another module. Everything goes through the **event
bus** in the host. A module that unloads simply stops answering — Authum drops
its subscriptions after `stop`.

Headers: `<authum/events.h>` (bus), `<authum/event/client.h>` (player events
the host publishes), `<authum/event/server.h>` (map and frame events). Shortcuts on the host facade: `authum_host_subscribe`,
`authum_host_publish`.

The rule behind it: **one module = one job.** qproto knows how a client
authenticated; it does not keep bans or history. A ban module refuses
connections; a stats module writes MySQL. They meet on the bus.

---

## 1. Two patterns, one mechanism

**Forward** — "this happened". The publisher fills a payload, every subscriber
sees it in subscription order.

```c
authum_event_result_t
mymod_on_connect(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_client_t *ev = payload;
    /* … */
    return authum_event_result_continue;
}

authum_host_subscribe(host, AUTHUM_EVENT_CLIENT_CONNECT, mymod_on_connect, authum_null);
```

**Query** — "who knows X?". The caller fills the in-fields of a payload and
publishes; the module that owns the answer fills the out-fields and returns
`authum_event_result_handled`.

```c
qproto_client_query_t q = { .index = 1 };
if (authum_host_publish(host, QPROTO_EVENT_CLIENT_QUERY, &q) != authum_event_result_continue
    && q.found) {
    /* q.info.auth_kind — protocol the player connected with */
}
```

`authum_event_result_continue` from publish means nobody answered (the owner
is not loaded, or nothing matched).

---

## 2. Handler results

| Result | Meaning |
|--------|---------|
| `authum_event_result_continue` | Not mine / nothing to say — keep going |
| `authum_event_result_handled` | I acted or answered — still call the rest |
| `authum_event_result_stop` | I acted; nobody after me sees this event |

`authum_events_publish` returns the strongest result any handler gave.

---

## 3. Names and payloads

- Names are dotted, owner first: `authum.*` for the host, `<module>.*` for a
  module (`qproto.client.query`). Max 63 characters.
- The owner of an event declares its payload struct and name in a **public
  header with types only** (e.g. qproto's `<qproto/event.h>`). Consumers
  include that header; they never link the owner.
- A payload, like the rest of the ABI (::AUTHUM_MODULE_ABI), never changes what it
  has: new fields are only appended, so a module built against an older header
  reads the part it knows. There is no size to check — a module never runs
  against a publisher older than the header it was built with.
- Publish a **copy** of your state, not a pointer into it, if subscribers must
  not change it.

---

## 4. Player events from the host

Authum hooks the game DLL once and forwards to modules, so
modules need no hooks of their own. Payload: `authum_event_client_t`.

| Event | When | Notes |
|-------|------|-------|
| `authum.client.connect` | `ClientConnect` | Set `allow = authum_bool_false` + `reject_reason` to refuse; return `stop` if no one else should see it |
| `authum.client.put_in_server` | `ClientPutInServer` | |
| `authum.client.info_changed` | `ClientUserInfoChanged` | `name` is the new name — rename history lives here |
| `authum.client.disconnect` | `ClientDisconnect` | |

Fields: `index` (entity index 1…maxClients — AMXX `id`), `userid`, `name`,
`address` (connect), `auth_id`, `game_time`, `unix_time`, `allow`,
`reject_reason`.

| `authum.client.command` | `ClientCommand` | Payload `authum_event_client_command_t`: `client`, `argc`, `argv[16][128]`, `args` (the line after the command). Set `block = authum_bool_true` and the game DLL never sees the command |
| `authum.client.kill` | `ClientKill` (the `kill` command) | Payload `authum_event_client_t`; set `allow = authum_bool_false` to refuse the suicide |
| `authum.client.cvar_value` | `CvarValue2` | Payload `authum_event_client_cvar_t`: `client`, `request_id`, `cvar`, `value` — the answer to `authum_host_query_cvar(host, index, "cvar", request_id)` |

Map and frame events, payload `authum_event_server_t` (`<authum/event/server.h>`):
`map`, `max_clients`, `edict_count` (activate), `game_time`, `frame_time`,
`unix_time`.

| Event | When | Notes |
|-------|------|-------|
| `authum.server.activate` | after the game DLL's `ServerActivate` | The map is spawned; a good place to reset per-map state |
| `authum.server.deactivate` | `ServerDeactivate` | Map change or shutdown |
| `authum.server.frame` | `StartFrame` | Every tick — keep handlers short (for "every N seconds" use `<authum/timer.h>`) |
| `authum.server.precache` | worldspawn's `Spawn` | The map is loading: the only moment `authum_host_precache_model` / `_sound` / `_generic` work (0 otherwise) |
| `authum.server.shutdown` | `GameShutdown` | The server is quitting |

The HLDS log, payload `authum_event_server_log_t` (`text`, `game_time`,
`unix_time`):

| Event | When | Notes |
|-------|------|-------|
| `authum.server.log` | a line the game writes to the log (`log on`) | The lines stats are made of: `"Vega<2><STEAM_0:1:1><CT>" killed "Gordon<3><…><TERRORIST>" with "ak47"`, `World triggered "Round_Start"`. Read-only; Authum's own lines are not reported |

### Game messages

`authum.message.<Name>` — one event per user message the game sends:
`authum.message.DeathMsg`, `authum.message.SayText`, `authum.message.TextMsg`,
… Payload `authum_event_message_t` (`<authum/event/message.h>`): `name`,
`msg_id`, `dest`, `index` (the receiver for one-player messages), `argc` and
the arguments, read with `authum_event_message_get_i32` / `_get_float` /
`_get_string` (index from 0, as the game writes them).

A handler may change arguments (`authum_event_message_set_*`) or set
`block = authum_bool_true`: nobody receives the message.

```c
/* DeathMsg: killer, victim, headshot, weapon */
authum_event_result_t
mymod_on_death(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_message_t *ev = payload;
    int killer = authum_event_message_get_i32(ev, 0);
    int victim = authum_event_message_get_i32(ev, 1);
    const authum_char_t *weapon = authum_event_message_get_string(ev, 3);
    /* … */
    return authum_event_result_continue;
}

authum_host_subscribe(host, AUTHUM_EVENT_MESSAGE_PREFIX "DeathMsg", mymod_on_death, authum_null);
```

These are the real game's messages: Authum hands the game the engine with its
own hooks over them (`src/authum/gamedll/engine.cpp`). Only messages someone
subscribed to are held back, so the rest cost nothing; a held message goes to
the engine once, after the handlers ran. Messages Authum and its modules send
go straight to the engine and are not reported, so a `SayText` handler can
send `SayText`.

A ban module, for example:

```c
authum_event_result_t
bans_on_authorized(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const qproto_client_info_t *info = payload;
    if (bans_contains(info->long_authid))
        authum_host_kick(&g_bans_host, info->userid, "You are banned.");
    return authum_event_result_continue;
}
```

---

## 5. Lifetime

- Subscribe in `start`. Unsubscribing in `stop` is optional — the host drops
  every subscription a module made after its `stop`.
- Delivery is synchronous on the publishing thread. The bus does not copy or
  retain the payload; handlers must not retain it without a separate agreement.
- Handlers may subscribe, unsubscribe and publish from inside a handler.
  Removal skips a not-yet-called handler immediately, including in nested
  publications. Unlinking and freeing wait for the outermost publish to return.
  New subscriptions may run in the ongoing publication.
- All bus calls must be serialized on the server thread; worker threads must
  hand work back to that thread before publishing.
- The same `(name, fn, user)` is registered once; subscribing again is a no-op.

---

## 6. Checklist

- [ ] Payload struct + event name in a public types-only header of the owner
- [ ] New payload fields only at the end
- [ ] Queries: in-fields, out-fields, `found`; answer with `handled`
- [ ] No `dlsym` / function tables between modules

## 7. Numeric dispatch

The host uses the LH module event bus, `<lh/os/module/event/bus.h>`: the registry,
the per-event subscriber lists, subscribing once per module, handler and user, and
dropping a module's subscriptions when it unloads. A module handler is the bus
callback itself and its user value the subscription context, so Authum adds nothing
of its own but the module ABI table.

Resolve an event once during module startup, then retain its ID:

```c
authum_event_id_t id = authum_events_get_id(authum_host_get_events(host), "scores.changed");
/* Check id != 0 before retaining/using it. */
authum_events_subscribe_id(authum_host_get_events(host), id, on_changed, user);
authum_events_publish_id(authum_host_get_events(host), id, payload);
```

`authum_events_unsubscribe_id` removes the same subscription as the string
operation. Mixed string/numeric registration still deduplicates by module,
event, callback and user. The callback continues to receive the canonical name.

IDs are bus-local, one-based and never recycled during the host lifetime.
Zero is invalid/unavailable. IDs survive module unload and clearing subscribers;
do not save them to disk or exchange them between independent host instances.
The host registry retains names for its lifetime. In standalone LH usage,
`lh_os_module_event_bus_deinit` frees names, entries and subscriptions and invalidates IDs.

Registration and string lookup scan registered event names, without scanning
subscriptions. Numeric publication performs direct array lookup, followed only
by that event's subscribers; no hashing or name comparison occurs in dispatch.
Fixed game hooks and registered game-message events cache these IDs.
