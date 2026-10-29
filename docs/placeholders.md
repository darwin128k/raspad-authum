# Placeholder service

The host owns the placeholder vocabulary. A module never parses `{name}`
itself: it asks the host to resolve a template and gets a path back.

    const authum_placeholder_service_t *placeholders = authum_host_placeholders(host);
    authum_char_t path[512];
    if (!authum_placeholder_expand(placeholders, "{game}/maps", path, sizeof path))
    {
        /* unknown placeholder, or the result does not fit */
    }

The service is owner-bound like the HTTP one: `authum_host_placeholders()`
returns the scope of the calling module, or null when the host has none — the
module should say so plainly rather than guess.

## Standard names

The host fills these from the module's own environment
(<authum/env.h>). Every module sees its own folder under the same name:

| Name | Path |
|------|------|
| `{game}` | game directory (`cstrike`, …) |
| `{module}` | `modules/<module>` — this module's folder |
| `{data}` | `modules/<module>/data` — what the module writes |
| `{config}` | `modules/<module>/configs` — the admin's |
| `{lang}` | `modules/<module>/lang` |
| `{models}`, `{sprites}` | `game/models`, `game/sprites` |
| `{addons}`, `{authum}`, `{modules}` | `game/addons`, `addons/authum`, `authum/modules` |

A name the host does not know (no game directory, say) is simply absent:
expanding it fails instead of producing a wrong path.

Standard names are reserved. A module cannot register one, and cannot take a
name another module already published.

## Registering a name

A module may publish a name of its own:

    authum_placeholder_register(placeholders, "mymaps", "/srv/games/mymaps");

The name becomes available to every module through
`authum_placeholder_expand()`. This is the only channel between modules
here: one publishes a path, the other uses the name — neither learns what the
other does inside its own folder.

Names are letters, digits and `_`. Values are absolute paths; a relative one
is refused. Names are unique host-wide: a taken name is refused rather than
stolen, so two modules cannot fight over one.

Registration is per owner. The entry lives until the registering module is
unloaded, then it is gone — a later module cannot find a name whose owner is
gone. `authum_placeholder_unregister()` removes it earlier.

## Expansion rules

A template is plain text with `{name}` tokens. Expansion substitutes the
value of each token and returns an **absolute** path: a template that stays
relative is taken against the calling module's folder (`{module}`).

- Unknown name — failure.
- Unpaired `{` or `}` — failure.
- Empty template — failure.
- Result larger than the caller's buffer — failure.

Nothing is guessed: a typo in a template stops startup instead of publishing
the wrong tree.

Values are canonicalized (symlinks resolved), so a folder may be published as
a source root: the HTTP server opens a source root without following a
symlink at its end.

## Threading

Owner-thread only, like the HTTP service: register and expand from the game
thread, not from a worker. The registry is not synchronized.

## Implementation map

- `include/authum/placeholders.h` — the module-facing API.
- `src/authum/api/placeholders.c` — SDK dispatchers.
- `src/authum/host/placeholders.c` — the registry and expansion.
