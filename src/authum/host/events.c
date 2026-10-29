/**
 * @file events.c
 * @brief Host-owned module subscriptions over the LH indexed event bus.
 *
 * Names and IDs live for the host lifetime, across module unload and clear.
 * All calls run on the server thread; LH owns dispatch and deferred removal.
 */

#include <authum/host/events.h>
#include <authum/event/name/limits.h>
#include <authum/host/events/internal.h>
#include <authum/host/events/binding.h>
#include <authum/memory.h>
#include <authum/null.h>
#include <authum/unused.h>
#include <lh/util/str/ptr.h>

static lh_os_module_event_bus_t authum_events_bus;
static authum_bool_t authum_events_initialized;

lh_os_module_event_bus_t *
authum_events_host_get_bus(void)
{
    if (!authum_events_initialized)
    {
        lh_os_module_event_bus_init(lh_addr_of(authum_events_bus));
        authum_events_initialized = authum_bool_true;
    }
    return lh_addr_of(authum_events_bus);
}

authum_event_id_t
authum_events_host_resolve(authum_ptr owner, const authum_char_t *name)
{
    authum_usize_t length;
    authum_unused(owner);
    if (authum_null_eq(name))
    {
        return 0u;
    }
    length = lh_str_ptr_len(name);
    if (lh_math_eq(length, 0u) || lh_math_gt(length, AUTHUM_EVENT_NAME_LENGTH_MAX))
    {
        return 0u;
    }
    return lh_os_module_event_bus_try_get_id(authum_events_host_get_bus(), name);
}

lh_os_module_event_subscription_t *
authum_events_host_find(authum_ptr owner, authum_event_id_t id, authum_event_handler_fn fn,
                        authum_ptr user)
{
    authum_events_binding_t key;
    authum_events_binding_set_fn(lh_addr_of(key), fn);
    authum_events_binding_set_user(lh_addr_of(key), user);
    return lh_os_module_event_bus_find_subscription(authum_events_host_get_bus(), id, owner,
                                                    authum_events_binding_matches, lh_addr_of(key));
}

authum_event_result_t
authum_events_host_publish_id_as(authum_ptr owner, authum_event_id_t id, authum_ptr payload)
{
    authum_unused(owner);
    return lh_cast_static(authum_event_result_t, lh_os_module_event_bus_publish(
                                                     authum_events_host_get_bus(), id, payload));
}

authum_event_id_t
authum_events_host_get_cached_id(authum_event_id_t *cache, const authum_char_t *name)
{
    lh_os_module_event_id_t native_id =
        lh_cast_static(lh_os_module_event_id_t, lh_ptr_deref(cache));

    lh_ptr_deref(cache) = lh_cast_static(
        authum_event_id_t, lh_os_module_event_bus_try_get_cached_id(authum_events_host_get_bus(),
                                                                    lh_addr_of(native_id), name));
    return lh_ptr_deref(cache);
}

void
authum_events_host_drop_owner(authum_ptr owner)
{
    lh_os_module_event_bus_drop_owner(authum_events_host_get_bus(), owner);
}

authum_bool_t
authum_events_host_has_id(authum_event_id_t id)
{
    return !authum_null_eq(lh_os_module_event_bus_get_first(authum_events_host_get_bus(), id))
               ? authum_bool_true
               : authum_bool_false;
}

authum_usize_t
authum_events_host_count(void)
{
    return lh_os_module_event_bus_get_count(authum_events_host_get_bus());
}

void
authum_events_host_clear(void)
{
    lh_os_module_event_bus_clear(authum_events_host_get_bus());
}

authum_bool_t
authum_events_host_subscribe_id(authum_ptr owner, authum_event_id_t id, authum_event_handler_fn fn,
                                authum_ptr user)
{
    authum_events_binding_t *binding;
    lh_os_module_event_bus_t *bus = authum_events_host_get_bus();
    if (authum_null_eq(fn) || authum_null_eq(lh_os_module_event_bus_get_entry(bus, id)))
    {
        return authum_bool_false;
    }
    if (!authum_null_eq(authum_events_host_find(owner, id, fn, user)))
    {
        return authum_bool_true;
    }
    binding = lh_cast_static(authum_events_binding_t *,
                             authum_memory_alloc(sizeof lh_ptr_deref(binding)));
    if (authum_null_eq(binding))
    {
        return authum_bool_false;
    }
    authum_events_binding_set_fn(binding, fn);
    authum_events_binding_set_user(binding, user);
    if (authum_null_eq(lh_os_module_event_bus_try_subscribe(
            bus, id, owner, authum_events_binding_call, binding, authum_events_binding_destroy)))
    {
        authum_memory_free(binding);
        return authum_bool_false;
    }
    return authum_bool_true;
}

authum_bool_t
authum_events_host_unsubscribe_id(authum_ptr owner, authum_event_id_t id,
                                  authum_event_handler_fn fn, authum_ptr user)
{
    lh_os_module_event_subscription_t *sub = authum_events_host_find(owner, id, fn, user);
    if (authum_null_eq(sub))
    {
        return authum_bool_false;
    }
    lh_os_module_event_bus_remove(authum_events_host_get_bus(), sub);
    return authum_bool_true;
}

authum_event_result_t
authum_events_host_publish_as(authum_ptr owner, const authum_char_t *name, authum_ptr payload)
{
    return authum_events_host_publish_id_as(
        owner, lh_os_module_event_bus_find_id(authum_events_host_get_bus(), name), payload);
}

authum_event_result_t
authum_events_host_publish_id(authum_event_id_t id, authum_ptr payload)
{
    return authum_events_host_publish_id_as(authum_null, id, payload);
}

authum_event_id_t
authum_events_host_get_id(const authum_char_t *name)
{
    return authum_events_host_resolve(authum_null, name);
}

authum_bool_t
authum_events_host_has(const authum_char_t *name)
{
    return authum_events_host_has_id(
        lh_os_module_event_bus_find_id(authum_events_host_get_bus(), name));
}

authum_bool_t
authum_events_host_subscribe(authum_ptr owner, const authum_char_t *name,
                             authum_event_handler_fn fn, authum_ptr user)
{
    if (authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    return authum_events_host_subscribe_id(owner, authum_events_host_resolve(owner, name), fn,
                                           user);
}

authum_bool_t
authum_events_host_unsubscribe(authum_ptr owner, const authum_char_t *name,
                               authum_event_handler_fn fn, authum_ptr user)
{
    return authum_events_host_unsubscribe_id(
        owner, lh_os_module_event_bus_find_id(authum_events_host_get_bus(), name), fn, user);
}

authum_event_result_t
authum_events_host_publish(const authum_char_t *name, authum_ptr payload)
{
    return authum_events_host_publish_as(authum_null, name, payload);
}
