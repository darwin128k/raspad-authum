# Console / RCON commands

Authum registers **one** engine command, `authum`. Every module command lives
under it:

```
authum / list / status       loaded modules (RUNNING)
authum info                  Authum version + how many modules
authum info <module>         one module: version, path, commands yes/no
authum load <module>         load modules/<module>.so
authum unload <module>       stop + unload (temporary off — like meta unload)
authum reload <module>       unload then load again
authum help                  built-ins + every module command
authum version               Authum + module ABI
authum <module> <name> ...   that module's own command
```

Examples over RCON:

```
rcon authum status
rcon authum info qproto
rcon authum unload qproto          # off — authum qproto info disappears
rcon authum load qproto            # on again — commands come back
rcon authum reload qproto
rcon authum qproto info
rcon authum qproto info #12
```

`load` / `unload` / `reload` are why Authum is not “just Metamod with another name”:
you can turn a module off without restarting the server or fighting AMXX for a
command word. After `unload`, the module's event subscriptions and
`authum <module> …` commands are gone; after `load` / `reload` they register
again (Qproto included).

The reply goes through the engine console (`pfnServerPrint`), which is what an
RCON client receives.

## Why modules never call `pfnAddServerCommand`

- **Name clashes.** AMX Mod X, other Metamod plugins and the game DLL register
  commands too. The engine keeps the first registration of a name, so a second
  plugin silently loses its command or takes another plugin's. Under `authum`
  there is one namespace per module, and the module part is the module's own
  `authum_module_info.name`, so a module cannot claim another module's words.
- **Unload safety.** The engine has no way to remove a server command. A
  command whose code lived in an unloaded `.so` crashes the server the next
  time someone types it. Authum's `authum` handler lives in `authum_mm` and
  only dispatches into modules that are loaded right now; a module's commands
  are dropped after its `stop`.

## Adding commands

```c
#include <authum/client.h>

authum_bool_t
mymod_cmd_status(const authum_command_t *cmd, authum_ptr user)
{
    authum_i32_t slot;

    (void)user;
    if (authum_command_argc(cmd) != 1u || !authum_command_arg_i32(cmd, 0u, &slot)) {
        return authum_bool_false; /* Authum prints the usage line */
    }
    authum_command_printf(cmd, "slot %d: ok", (int)slot);
    return authum_bool_true;
}

const authum_command_def_t g_mymod_commands[] = {
    { "status", "<slot>", "show one slot", mymod_cmd_status, authum_null },
};

/* in start(): */
authum_host_command_add_all(host, g_mymod_commands,
                            sizeof g_mymod_commands / sizeof g_mymod_commands[0]);
```

That gives `authum mymod status 3`.

| Piece | Notes |
|-------|-------|
| `name` | Letters, digits, `_`, `-`, `.`; up to 31 chars; case-insensitive |
| `usage` | Argument synopsis, shown in help and on bad usage (may be null) |
| `help` | One line for `authum help` (may be null) |
| `fn` | Returns `authum_bool_false` for bad usage. Authum then prints `usage: authum <module> <name> <usage>` |
| `user` | Passed back to `fn` |

Strings are copied, so the def table can be `const` data or live on the stack.
Adding the same def twice is fine. Adding a different handler under a taken
name fails.

## Inside a handler

| Call | Gives |
|------|-------|
| `authum_command_argc(cmd)` | Arguments after the command name |
| `authum_command_argv(cmd, i)` | Argument `i` (`""` when out of range) |
| `authum_command_arg_i32(cmd, i, &out)` | Argument `i` as a number |
| `authum_command_scope(cmd)` / `_name(cmd)` | `qproto` / `info` |
| `authum_command_print(cmd, line)` | One line (newline added) |
| `authum_command_printf(cmd, fmt, ...)` | Formatted line, up to 512 chars |

The engine has already split the line: quoted arguments (`"Some Player"`)
arrive as one argument.

## Lifetime

- Commands are owned by the module that added them and dropped by the host
  after its `stop` (or a failed `start`); nothing to remove by hand.
- `authum_commands_remove(authum_host_commands(host), "name")` removes one
  early; a handler may remove its own command.
- `authum` built-ins (`list`, `help`, `version`) win over a module named the
  same.

## Host side

`src/authum/commands.c` (registry, tests in `tests/commands.c`),
`src/authum/console.c` (built-ins and routing), `src/authum/plugin/meta.cpp`
(the one `REG_SVR_COMMAND`).
