#include <authum/events.h>
#include <authum/null.h>

const authum_events_ops_t *
authum_events_get_ops(const authum_events_t *self)
{
    return self->ops;
}

void
authum_events_set_ops(authum_events_t *self, const authum_events_ops_t *value)
{
    self->ops = value;
}

authum_ptr
authum_events_get_owner(const authum_events_t *self)
{
    return self->owner;
}

void
authum_events_set_owner(authum_events_t *self, authum_ptr value)
{
    self->owner = value;
}

authum_bool_t
authum_events_subscribe(const authum_events_t *self, const authum_char_t *name,
                        authum_event_handler_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) || !authum_events_ops_has_subscribe(authum_events_get_ops(self)) ||
        authum_null_eq(name) || authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    return authum_events_ops_get_subscribe(authum_events_get_ops(self))(
        authum_events_get_owner(self), name, fn, user);
}

authum_bool_t
authum_events_unsubscribe(const authum_events_t *self, const authum_char_t *name,
                          authum_event_handler_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) || !authum_events_ops_has_unsubscribe(authum_events_get_ops(self)) ||
        authum_null_eq(name) || authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    return authum_events_ops_get_unsubscribe(authum_events_get_ops(self))(
        authum_events_get_owner(self), name, fn, user);
}

authum_event_result_t
authum_events_publish(const authum_events_t *self, const authum_char_t *name, authum_ptr payload)
{
    if (authum_null_eq(self) || !authum_events_ops_has_publish(authum_events_get_ops(self)) ||
        authum_null_eq(name))
    {
        return authum_event_result_continue;
    }
    return authum_events_ops_get_publish(authum_events_get_ops(self))(authum_events_get_owner(self),
                                                                      name, payload);
}

authum_event_id_t
authum_events_get_id(const authum_events_t *self, const authum_char_t *name)
{
    if (authum_null_eq(self) || !authum_events_ops_has_resolve(authum_events_get_ops(self)) ||
        authum_null_eq(name))
    {
        return 0u;
    }
    return authum_events_ops_get_resolve(authum_events_get_ops(self))(authum_events_get_owner(self),
                                                                      name);
}

authum_bool_t
authum_events_subscribe_id(const authum_events_t *self, authum_event_id_t id,
                           authum_event_handler_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) || !authum_events_ops_has_subscribe_id(authum_events_get_ops(self)) ||
        authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    return authum_events_ops_get_subscribe_id(authum_events_get_ops(self))(
        authum_events_get_owner(self), id, fn, user);
}

authum_bool_t
authum_events_unsubscribe_id(const authum_events_t *self, authum_event_id_t id,
                             authum_event_handler_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) ||
        !authum_events_ops_has_unsubscribe_id(authum_events_get_ops(self)) || authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    return authum_events_ops_get_unsubscribe_id(authum_events_get_ops(self))(
        authum_events_get_owner(self), id, fn, user);
}

authum_event_result_t
authum_events_publish_id(const authum_events_t *self, authum_event_id_t id, authum_ptr payload)
{
    if (authum_null_eq(self) || !authum_events_ops_has_publish_id(authum_events_get_ops(self)))
    {
        return authum_event_result_continue;
    }
    return authum_events_ops_get_publish_id(authum_events_get_ops(self))(
        authum_events_get_owner(self), id, payload);
}
