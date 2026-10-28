/**
 * @file events.c
 * @brief Event bus: name → ordered subscriber list, owner-tagged.
 *
 * Handlers may subscribe / unsubscribe / publish from inside a handler.
 * Removal during a publish only marks the entry; it is unlinked once the
 * outermost publish returns.
 */

#include <authum/host/events.h>
#include <authum/memory.h>
#include <lh/memory/std.h>

#include <authum/null.h>
#include <authum/unused.h>

#include <lh/runtime/allocator.h>

#include <string.h>

enum
{
    authum_events_name_max = 64
};

typedef struct authum_events_sub authum_events_sub_t;

struct authum_events_sub
{
    authum_events_sub_t *next;
    authum_ptr owner;
    authum_event_handler_fn fn;
    authum_ptr user;
    authum_bool_t dead;
    authum_char_t name[authum_events_name_max];
};

authum_events_sub_t *authum_events_head;
authum_usize_t authum_events_depth;

authum_bool_t
authum_events_sub_matches(const authum_events_sub_t *sub, authum_ptr owner,
                          const authum_char_t *name, authum_event_handler_fn fn, authum_ptr user)
{
    return (!sub->dead && lh_math_eq(sub->owner, owner) && lh_math_eq(sub->fn, fn) &&
            lh_math_eq(sub->user, user) && lh_math_eq(strcmp(sub->name, name), 0))
               ? authum_bool_true
               : authum_bool_false;
}

void
authum_events_sweep(void)
{
    authum_events_sub_t **link = lh_addr_of(authum_events_head);

    if (lh_math_ne(authum_events_depth, 0u))
    {
        return;
    }
    while (!authum_null_eq(lh_ptr_deref(link)))
    {
        authum_events_sub_t *sub = lh_ptr_deref(link);
        if (sub->dead)
        {
            lh_ptr_deref(link) = sub->next;
            authum_memory_free(sub);
        }
        else
        {
            link = lh_addr_of(sub->next);
        }
    }
}

authum_bool_t
authum_events_host_subscribe(authum_ptr owner, const authum_char_t *name,
                             authum_event_handler_fn fn, authum_ptr user)
{
    authum_events_sub_t **link = lh_addr_of(authum_events_head);
    authum_events_sub_t *sub;
    authum_usize_t len;

    if (authum_null_eq(name) || lh_math_eq(fn, 0))
    {
        return authum_bool_false;
    }
    len = lh_cast_static(authum_usize_t, strlen(name));
    if (lh_math_eq(len, 0u) ||
        lh_math_ge(len, lh_cast_static(authum_usize_t, authum_events_name_max)))
    {
        return authum_bool_false;
    }
    while (!authum_null_eq(lh_ptr_deref(link)))
    {
        if (authum_events_sub_matches(lh_ptr_deref(link), owner, name, fn, user))
        {
            return authum_bool_true;
        }
        link = lh_addr_of((lh_ptr_deref(link))->next);
    }
    sub = lh_cast_reinterpret(authum_events_sub_t *, authum_memory_alloc(sizeof lh_ptr_deref(sub)));
    if (authum_null_eq(sub))
    {
        return authum_bool_false;
    }
    lh_memory_std_set(sub, 0, sizeof lh_ptr_deref(sub));
    sub->owner = owner;
    sub->fn = fn;
    sub->user = user;
    lh_memory_std_copy(sub->name, name, lh_math_add(lh_cast_static(authum_usize_t, len), 1u));
    lh_ptr_deref(link) = sub;
    return authum_bool_true;
}

authum_bool_t
authum_events_host_unsubscribe(authum_ptr owner, const authum_char_t *name,
                               authum_event_handler_fn fn, authum_ptr user)
{
    authum_events_sub_t *sub;

    if (authum_null_eq(name) || lh_math_eq(fn, 0))
    {
        return authum_bool_false;
    }
    for (sub = authum_events_head; !authum_null_eq(sub); sub = sub->next)
    {
        if (authum_events_sub_matches(sub, owner, name, fn, user))
        {
            sub->dead = authum_bool_true;
            authum_events_sweep();
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

authum_event_result_t
authum_events_host_publish_as(authum_ptr owner, const authum_char_t *name, authum_ptr payload)
{
    authum_events_sub_t *sub;
    authum_event_result_t best = authum_event_result_continue;
    authum_event_result_t r;

    authum_unused(owner);
    if (authum_null_eq(name))
    {
        return best;
    }
    authum_events_depth = lh_math_add_one(authum_events_depth);
    for (sub = authum_events_head; !authum_null_eq(sub); sub = sub->next)
    {
        if (sub->dead || lh_math_ne(strcmp(sub->name, name), 0))
        {
            continue;
        }
        r = sub->fn(name, payload, sub->user);
        if (lh_math_gt(r, best))
        {
            best = r;
        }
        if (lh_math_eq(r, authum_event_result_stop))
        {
            break;
        }
    }
    authum_events_depth = lh_math_sub_one(authum_events_depth);
    authum_events_sweep();
    return best;
}

const authum_events_ops_t authum_events_host_ops_table = {
    .size = sizeof(authum_events_ops_t),
    .subscribe = authum_events_host_subscribe,
    .unsubscribe = authum_events_host_unsubscribe,
    .publish = authum_events_host_publish_as,
};

const authum_events_ops_t *
authum_events_host_ops(void)
{
    return lh_addr_of(authum_events_host_ops_table);
}

authum_event_result_t
authum_events_host_publish(const authum_char_t *name, authum_ptr payload)
{
    return authum_events_host_publish_as(authum_null, name, payload);
}

void
authum_events_host_drop_owner(authum_ptr owner)
{
    authum_events_sub_t *sub;

    for (sub = authum_events_head; !authum_null_eq(sub); sub = sub->next)
    {
        if (lh_math_eq(sub->owner, owner))
        {
            sub->dead = authum_bool_true;
        }
    }
    authum_events_sweep();
}

authum_bool_t
authum_events_host_has(const authum_char_t *name)
{
    authum_events_sub_t *sub;

    if (authum_null_eq(name))
    {
        return authum_bool_false;
    }
    for (sub = authum_events_head; !authum_null_eq(sub); sub = sub->next)
    {
        if (!sub->dead && lh_math_eq(strcmp(sub->name, name), 0))
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

authum_usize_t
authum_events_host_count(void)
{
    authum_events_sub_t *sub;
    authum_usize_t n = 0u;

    for (sub = authum_events_head; !authum_null_eq(sub); sub = sub->next)
    {
        if (!sub->dead)
        {
            n = lh_math_add_one(n);
        }
    }
    return n;
}

void
authum_events_host_clear(void)
{
    authum_events_sub_t *sub;

    for (sub = authum_events_head; !authum_null_eq(sub); sub = sub->next)
    {
        sub->dead = authum_bool_true;
    }
    authum_events_sweep();
}
