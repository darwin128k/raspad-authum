#include <authum/client/ref.h>
#include <lh/util/str/ptr.h>
#include <authum/memory.h>
#include <lh/memory/std.h>
#include <authum/event/client.h>
#include <authum/event/server.h>
#include <stdlib.h>

typedef struct authum_client_identity
{
    authum_u64_t generation;
    authum_i32_t userid;
    authum_bool_t connected;
} authum_client_identity_t;

typedef struct authum_clients_impl
{
    authum_host_t host;
    authum_usize_t max;
    authum_u64_t id;
    authum_client_identity_t slots[];
} authum_clients_impl_t;

static authum_u64_t m_next_tracker;
static const authum_char_t *const m_client_events[] = {
    AUTHUM_EVENT_CLIENT_CONNECT, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, AUTHUM_EVENT_CLIENT_DISCONNECT,
    AUTHUM_EVENT_SERVER_DEACTIVATE, AUTHUM_EVENT_SERVER_ACTIVATE};

authum_event_result_t
authum_clients_event(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_clients_impl_t *impl = user;
    const authum_event_client_t *ev = payload;
    authum_client_identity_t *slot;
    authum_usize_t i;
    if (lh_str_ptr_equals(name, AUTHUM_EVENT_SERVER_DEACTIVATE, lh_bool_false) ||
        lh_str_ptr_equals(name, AUTHUM_EVENT_SERVER_ACTIVATE, lh_bool_false))
    {
        for (i = 1; lh_math_le(i, impl->max); i = lh_math_add_one(i))
        {
            ++impl->slots[i].generation;
            impl->slots[i].connected = authum_bool_false;
        }
        return authum_event_result_continue;
    }
    if (!ev || lh_math_lt(ev->index, 1) ||
        lh_math_gt(lh_cast_static(authum_usize_t, ev->index), impl->max))
    {
        return authum_event_result_continue;
    }
    slot = lh_addr_of(impl->slots[ev->index]);
    if (!lh_str_ptr_equals(name, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, lh_bool_false))
    {
        slot->generation = lh_math_add_one(slot->generation);
    }
    else if (!slot->connected)
    {
        slot->generation = lh_math_add_one(slot->generation);
    }
    slot->connected = !lh_str_ptr_equals(name, AUTHUM_EVENT_CLIENT_DISCONNECT, lh_bool_false);
    slot->userid = ev->userid;
    return authum_event_result_continue;
}

void
authum_clients_destroy(authum_clients_t *self)
{
    authum_clients_impl_t *impl;
    authum_u32_t i;
    if (!self || !(impl = self->impl))
    {
        return;
    }
    for (i = 0; lh_math_lt(i, lh_math_div(sizeof m_client_events, sizeof m_client_events[0]));
         i = lh_math_add_one(i))
    {
        authum_events_unsubscribe(authum_host_get_events(lh_addr_of(impl->host)),
                                  m_client_events[i], authum_clients_event, impl);
    }
    authum_memory_free(impl);
    self->impl = authum_null;
}

authum_bool_t
authum_clients_init(authum_clients_t *self, const authum_host_t *host, authum_usize_t max)
{
    authum_clients_impl_t *impl;
    authum_u32_t i;
    if (!self || self->impl || !host || !authum_host_get_events(host) || !max ||
        lh_math_gt(max, 4096))
    {
        return authum_bool_false;
    }
    impl = authum_memory_alloc_zero(
        1, lh_math_add(sizeof lh_ptr_deref(impl),
                       lh_math_mul((lh_math_add(max, 1)), sizeof impl->slots[0])));
    if (!impl)
    {
        return authum_bool_false;
    }
    authum_host_copy(lh_addr_of(impl->host), host);
    impl->max = max;
    impl->id = ++m_next_tracker;
    for (i = 1; lh_math_le(i, max); i = lh_math_add_one(i))
    {
        impl->slots[i].userid =
            authum_host_client_get_userid(host, lh_cast_static(authum_i32_t, i));
        impl->slots[i].connected = lh_math_gt(impl->slots[i].userid, 0);
        impl->slots[i].generation = 1;
    }
    self->impl = impl;
    for (i = 0; lh_math_lt(i, lh_math_div(sizeof m_client_events, sizeof m_client_events[0]));
         i = lh_math_add_one(i))
    {
        if (!authum_host_subscribe(host, m_client_events[i], authum_clients_event, impl))
        {
            authum_clients_destroy(self);
            return authum_bool_false;
        }
    }
    return authum_bool_true;
}

authum_bool_t
authum_clients_capture(const authum_clients_t *self, authum_i32_t index, authum_client_ref_t *out)
{
    const authum_clients_impl_t *impl = self ? self->impl : authum_null;
    const authum_client_identity_t *slot;
    if (out)
    {
        lh_memory_std_set(out, 0, sizeof lh_ptr_deref(out));
    }
    if (!impl || !out || lh_math_lt(index, 1) ||
        lh_math_gt(lh_cast_static(authum_usize_t, index), impl->max))
    {
        return authum_bool_false;
    }
    slot = lh_addr_of(impl->slots[index]);
    if (!slot->connected || lh_math_le(slot->userid, 0) ||
        lh_math_ne(authum_host_client_get_userid(lh_addr_of(impl->host), index), slot->userid))
    {
        return authum_bool_false;
    }
    out->index = index;
    out->userid = slot->userid;
    out->generation = slot->generation;
    out->tracker = lh_cast_static(authum_ptr, impl);
    out->lifetime = impl->id;
    return authum_bool_true;
}

authum_bool_t
authum_client_ref_is_current(const authum_clients_t *self, const authum_client_ref_t *ref)
{
    authum_client_ref_t current;
    return ref && authum_clients_capture(self, ref->index, lh_addr_of(current)) &&
           lh_math_eq(ref->tracker, current.tracker) &&
           lh_math_eq(ref->lifetime, current.lifetime) &&
           lh_math_eq(ref->generation, current.generation) &&
           lh_math_eq(ref->userid, current.userid);
}
