#include <authum/client/ref.h>
#include <authum/event/client.h>
#include <authum/event/server.h>
#include <stdlib.h>
#include <string.h>

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
static const char *const m_client_events[] = {
    AUTHUM_EVENT_CLIENT_CONNECT, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, AUTHUM_EVENT_CLIENT_DISCONNECT,
    AUTHUM_EVENT_SERVER_DEACTIVATE, AUTHUM_EVENT_SERVER_ACTIVATE};

authum_event_result_t
authum_clients_event(const char *name, authum_ptr payload, authum_ptr user)
{
    authum_clients_impl_t *impl = user;
    const authum_event_client_t *ev = payload;
    authum_client_identity_t *slot;
    authum_usize_t i;
    if (!strcmp(name, AUTHUM_EVENT_SERVER_DEACTIVATE) ||
        !strcmp(name, AUTHUM_EVENT_SERVER_ACTIVATE))
    {
        for (i = 1; i <= impl->max; ++i)
        {
            ++impl->slots[i].generation;
            impl->slots[i].connected = authum_bool_false;
        }
        return authum_event_result_continue;
    }
    if (!ev || ev->size < sizeof *ev || ev->index < 1 || (authum_usize_t)ev->index > impl->max)
    {
        return authum_event_result_continue;
    }
    slot = &impl->slots[ev->index];
    if (strcmp(name, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER))
    {
        ++slot->generation;
    }
    else if (!slot->connected)
    {
        ++slot->generation;
    }
    slot->connected = strcmp(name, AUTHUM_EVENT_CLIENT_DISCONNECT) != 0;
    slot->userid = ev->userid;
    return authum_event_result_continue;
}
void
authum_clients_destroy(authum_clients_t *self)
{
    authum_clients_impl_t *impl;
    unsigned i;
    if (!self || !(impl = self->impl))
    {
        return;
    }
    for (i = 0; i < sizeof m_client_events / sizeof m_client_events[0]; ++i)
    {
        authum_events_unsubscribe(authum_host_events(&impl->host), m_client_events[i],
                                  authum_clients_event, impl);
    }
    free(impl);
    self->impl = NULL;
}
authum_bool_t
authum_clients_init(authum_clients_t *self, const authum_host_t *host, authum_usize_t max)
{
    authum_clients_impl_t *impl;
    unsigned i;
    if (!self || self->impl || !host || !authum_host_events(host) || !max || max > 4096)
    {
        return authum_bool_false;
    }
    impl = calloc(1, sizeof *impl + (max + 1) * sizeof impl->slots[0]);
    if (!impl)
    {
        return authum_bool_false;
    }
    authum_host_copy(&impl->host, host);
    impl->max = max;
    impl->id = ++m_next_tracker;
    for (i = 1; i <= max; ++i)
    {
        impl->slots[i].userid = authum_host_client_userid(host, (authum_i32_t)i);
        impl->slots[i].connected = impl->slots[i].userid > 0;
        impl->slots[i].generation = 1;
    }
    self->impl = impl;
    for (i = 0; i < sizeof m_client_events / sizeof m_client_events[0]; ++i)
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
    const authum_clients_impl_t *impl = self ? self->impl : NULL;
    const authum_client_identity_t *slot;
    if (out)
    {
        memset(out, 0, sizeof *out);
    }
    if (!impl || !out || index < 1 || (authum_usize_t)index > impl->max)
    {
        return authum_bool_false;
    }
    slot = &impl->slots[index];
    if (!slot->connected || slot->userid <= 0 ||
        authum_host_client_userid(&impl->host, index) != slot->userid)
    {
        return authum_bool_false;
    }
    out->index = index;
    out->userid = slot->userid;
    out->generation = slot->generation;
    out->tracker = (authum_ptr)impl;
    out->lifetime = impl->id;
    return authum_bool_true;
}
authum_bool_t
authum_client_ref_is_current(const authum_clients_t *self, const authum_client_ref_t *ref)
{
    authum_client_ref_t current;
    return ref && authum_clients_capture(self, ref->index, &current) &&
           ref->tracker == current.tracker && ref->lifetime == current.lifetime &&
           ref->generation == current.generation && ref->userid == current.userid;
}
