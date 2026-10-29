/**
 * @file commands.c
 * @brief Command registry: (scope, name) → handler, owner-tagged.
 *
 * Same lifetime rules as the event bus: a handler may add / remove commands;
 * removal while a handler runs only marks the entry, it is unlinked once the
 * outermost run returns.
 */

#include <authum/host/commands.h>
#include <authum/memory.h>
#include <lh/char/letter.h>
#include <lh/char/digit.h>
#include <lh/memory/std.h>

#include <authum/cstr.h>

#include <lh/runtime/allocator.h>

#include <string.h>

typedef struct authum_commands_entry authum_commands_entry_t;

struct authum_commands_entry
{
    authum_commands_entry_t *next;
    authum_ptr owner;
    authum_command_fn fn;
    authum_ptr user;
    authum_bool_t dead;
    authum_char_t scope[authum_command_word_max];
    authum_char_t name[authum_command_word_max];
    authum_char_t usage[authum_command_usage_max];
    authum_char_t help[authum_command_help_max];
};

authum_commands_entry_t *authum_commands_head;
authum_usize_t authum_commands_depth;

authum_bool_t
authum_commands_host_is_word(const authum_char_t *text)
{
    authum_usize_t len = 0u;
    authum_char_t c;

    if (authum_null_eq(text))
    {
        return authum_bool_false;
    }
    for (; lh_math_ne(text[len], lh_char_map_nul); len = lh_math_add_one(len))
    {
        c = text[len];
        if (!(lh_char_is_letter(c) || lh_char_is_digit(c) ||
              lh_math_eq(c, lh_char_map_underscore) || lh_math_eq(c, lh_char_map_minus) ||
              lh_math_eq(c, lh_char_map_period)))
        {
            return authum_bool_false;
        }
    }
    return (lh_math_gt(len, 0u) &&
            lh_math_lt(len, lh_cast_static(authum_usize_t, authum_command_word_max)))
               ? authum_bool_true
               : authum_bool_false;
}

authum_commands_entry_t *
authum_commands_find(const authum_char_t *scope, const authum_char_t *name)
{
    authum_commands_entry_t *e;

    for (e = authum_commands_head; !authum_null_eq(e); e = e->next)
    {
        if (!e->dead && authum_cstr_eq(e->scope, scope, authum_bool_true) &&
            authum_cstr_eq(e->name, name, authum_bool_true))
        {
            return e;
        }
    }
    return authum_null;
}

void
authum_commands_sweep(void)
{
    authum_commands_entry_t **link = lh_addr_of(authum_commands_head);

    if (lh_math_ne(authum_commands_depth, 0u))
    {
        return;
    }
    while (!authum_null_eq(lh_ptr_deref(link)))
    {
        authum_commands_entry_t *e = lh_ptr_deref(link);
        if (e->dead)
        {
            lh_ptr_deref(link) = e->next;
            authum_memory_free(e);
        }
        else
        {
            link = lh_addr_of(e->next);
        }
    }
}

authum_bool_t
authum_commands_host_add(authum_ptr owner, const authum_char_t *scope,
                         const authum_command_def_t *def)
{
    authum_commands_entry_t **link = lh_addr_of(authum_commands_head);
    authum_commands_entry_t *e;

    if (authum_null_eq(def) || lh_math_eq(def->fn, 0) || !authum_commands_host_is_word(scope) ||
        !authum_commands_host_is_word(def->name))
    {
        return authum_bool_false;
    }
    e = authum_commands_find(scope, def->name);
    if (!authum_null_eq(e))
    {
        return (lh_math_eq(e->owner, owner) && lh_math_eq(e->fn, def->fn) &&
                lh_math_eq(e->user, def->user))
                   ? authum_bool_true
                   : authum_bool_false;
    }
    while (!authum_null_eq(lh_ptr_deref(link)))
    {
        link = lh_addr_of((lh_ptr_deref(link))->next);
    }
    e = lh_cast_reinterpret(authum_commands_entry_t *, authum_memory_alloc(sizeof lh_ptr_deref(e)));
    if (authum_null_eq(e))
    {
        return authum_bool_false;
    }
    lh_memory_std_set(e, 0, sizeof lh_ptr_deref(e));
    e->owner = owner;
    e->fn = def->fn;
    e->user = def->user;
    authum_cstr_copy(e->scope, sizeof e->scope, scope);
    authum_cstr_copy(e->name, sizeof e->name, def->name);
    authum_cstr_copy(e->usage, sizeof e->usage, def->usage);
    authum_cstr_copy(e->help, sizeof e->help, def->help);
    lh_ptr_deref(link) = e;
    return authum_bool_true;
}

authum_bool_t
authum_commands_host_remove(authum_ptr owner, const authum_char_t *scope, const authum_char_t *name)
{
    authum_commands_entry_t *e = authum_commands_find(scope, name);

    if (authum_null_eq(e) || lh_math_ne(e->owner, owner))
    {
        return authum_bool_false;
    }
    e->dead = authum_bool_true;
    authum_commands_sweep();
    return authum_bool_true;
}

const authum_commands_ops_t authum_commands_host_ops_table = {
    .add = authum_commands_host_add,
    .remove = authum_commands_host_remove,
};

const authum_commands_ops_t *
authum_commands_host_ops(void)
{
    return lh_addr_of(authum_commands_host_ops_table);
}

void
authum_commands_host_print_entry(const authum_commands_entry_t *e, authum_command_print_fn print,
                                 authum_ptr out, const authum_char_t *prefix)
{
    authum_command_t cmd;

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(lh_addr_of(cmd), "%sauthum %s %s%s%s%s%s", prefix, e->scope, e->name,
                          lh_math_ne(e->usage[0], lh_char_map_nul) ? " " : "", e->usage,
                          lh_math_ne(e->help[0], lh_char_map_nul) ? "  - " : "", e->help);
}

authum_bool_t
authum_commands_host_run(const authum_char_t *scope, const authum_char_t *name, authum_usize_t argc,
                         const authum_char_t *const *argv, authum_command_print_fn print,
                         authum_ptr out)
{
    authum_commands_entry_t *e = authum_commands_find(scope, name);
    authum_command_t cmd;
    authum_bool_t ok;

    if (authum_null_eq(e))
    {
        return authum_bool_false;
    }
    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.scope = e->scope;
    cmd.name = e->name;
    cmd.argc = authum_null_eq(argv) ? 0u : argc;
    cmd.argv = argv;
    cmd.print = print;
    cmd.out = out;

    authum_commands_depth = lh_math_add_one(authum_commands_depth);
    ok = e->fn(lh_addr_of(cmd), e->user);
    if (!ok)
    {
        authum_commands_host_print_entry(e, print, out, "usage: ");
    }
    authum_commands_depth = lh_math_sub_one(authum_commands_depth);
    authum_commands_sweep();
    return authum_bool_true;
}

authum_bool_t
authum_commands_host_has_scope(const authum_char_t *scope)
{
    authum_commands_entry_t *e;

    for (e = authum_commands_head; !authum_null_eq(e); e = e->next)
    {
        if (!e->dead && authum_cstr_eq(e->scope, scope, authum_bool_true))
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

authum_usize_t
authum_commands_host_print_help(const authum_char_t *scope, authum_command_print_fn print,
                                authum_ptr out)
{
    authum_commands_entry_t *e;
    authum_usize_t n = 0u;

    for (e = authum_commands_head; !authum_null_eq(e); e = e->next)
    {
        if (e->dead ||
            (!authum_null_eq(scope) && !authum_cstr_eq(e->scope, scope, authum_bool_true)))
        {
            continue;
        }
        authum_commands_host_print_entry(e, print, out, "  ");
        n = lh_math_add_one(n);
    }
    return n;
}

void
authum_commands_host_drop_owner(authum_ptr owner)
{
    authum_commands_entry_t *e;

    for (e = authum_commands_head; !authum_null_eq(e); e = e->next)
    {
        if (lh_math_eq(e->owner, owner))
        {
            e->dead = authum_bool_true;
        }
    }
    authum_commands_sweep();
}

authum_usize_t
authum_commands_host_count(void)
{
    authum_commands_entry_t *e;
    authum_usize_t n = 0u;

    for (e = authum_commands_head; !authum_null_eq(e); e = e->next)
    {
        if (!e->dead)
        {
            n = lh_math_add_one(n);
        }
    }
    return n;
}

void
authum_commands_host_clear(void)
{
    authum_commands_entry_t *e;

    for (e = authum_commands_head; !authum_null_eq(e); e = e->next)
    {
        e->dead = authum_bool_true;
    }
    authum_commands_sweep();
}
