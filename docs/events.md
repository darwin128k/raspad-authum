# Events: how modules talk

Authum modules never call each other directly: no `dlsym`, no shared function
tables, no pointers into another module. Everything goes through the **event
bus** in the host. A module that unloads simply stops answering — Authum drops
its subscriptions after `stop`.

Headers: `<authum/events.h>` (bus), `<authum/event/client.h>` (player events
the host publishes). Shortcuts on the host facade: `authum_host_subscribe`,
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
    if (ev->size < sizeof *ev)
        return authum_event_result_continue;   /* older host, unknown layout */
    /* … */
    return authum_event_result_continue;
}

authum_host_subscribe(host, AUTHUM_EVENT_CLIENT_CONNECT, mymod_on_connect, authum_null);
```

**Query** — "who knows X?". The caller fills the in-fields of a payload and
publishes; the module that owns the answer fills the out-fields and returns
`authum_event_result_handled`.

```c
qproto_client_query_t q = { .size = sizeof q, .index = 1 };
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
- Every payload starts with `authum_usize_t size` = `sizeof` the struct the
  publisher built. Subscribers check `size >= sizeof(their struct)` before
  reading; new fields are only ever appended. That is how a payload grows
  without breaking older modules.
- Publish a **copy** of your state, not a pointer into it, if subscribers must
  not change it.

---

## 4. Player events from the host

Authum hooks the game DLL (via Metamod) once and forwards to modules, so
modules need no Metamod hooks of their own. Payload: `authum_event_client_t`.

| Event | When | Notes |
|-------|------|-------|
| `authum.client.connect` | `ClientConnect` | Set `allow = authum_bool_false` + `reject_reason` to refuse; return `stop` if no one else should see it |
| `authum.client.put_in_server` | `ClientPutInServer` | |
| `authum.client.info_changed` | `ClientUserInfoChanged` | `name` is the new name — rename history lives here |
| `authum.client.disconnect` | `ClientDisconnect` | |

Fields: `index` (entity index 1…maxClients — AMXX `id`), `userid`, `name`,
`address` (connect), `auth_id`, `game_time`, `unix_time`, `allow`,
`reject_reason`.

A ban module, for example:

```c
authum_event_result_t
bans_on_authorized(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const qproto_client_info_t *info = payload;
    if (info->size >= sizeof *info && bans_contains(info->long_authid))
        authum_host_kick(&g_bans_host, info->userid, "You are banned.");
    return authum_event_result_continue;
}
```

---

## 5. Lifetime

- Subscribe in `start`. Unsubscribing in `stop` is optional — the host drops
  every subscription a module made after its `stop`.
- Handlers may subscribe, unsubscribe and publish from inside a handler.
  Removal during a publish takes effect for the next publish.
- The same `(name, fn, user)` is registered once; subscribing again is a no-op.

---

## 6. Checklist

- [ ] Payload struct + event name in a public types-only header of the owner
- [ ] `size` first; check it in every handler
- [ ] Queries: in-fields, out-fields, `found`; answer with `handled`
- [ ] No `dlsym` / function tables between modules
