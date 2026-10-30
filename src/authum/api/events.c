#include <authum/events.h>
#include <authum/null.h>

#include <lh/str.h>

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

const authum_char_t *
authum_events_get_prefix(const authum_events_t *self)
{
    return authum_null_eq(self) ? authum_null : self->prefix;
}

void
authum_events_set_prefix(authum_events_t *self, const authum_char_t *prefix)
{
    if (!authum_null_eq(self))
    {
        self->prefix = prefix;
    }
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

/** Whether @p name is one of the module's own names (`<prefix>.<what>`). */
static authum_bool_t
authum_events_name_is_own(const authum_events_t *self, const authum_char_t *name)
{
    const authum_char_t *prefix = authum_events_get_prefix(self);
    authum_usize_t length;

    if (authum_null_eq(prefix) || authum_null_eq(name))
    {
        return authum_bool_false;
    }
    length = lh_str_ptr_len(prefix);
    if (lh_math_eq(length, 0u) || !lh_str_ptr_starts_with(name, prefix, lh_bool_false))
    {
        return authum_bool_false;
    }
    return lh_math_eq(name[length], '.');
}

authum_event_result_t
authum_events_forward(const authum_events_t *self, const authum_char_t *name, authum_ptr payload)
{
    if (authum_null_eq(self) || !authum_events_ops_has_publish(authum_events_get_ops(self)) ||
        authum_null_eq(name) || !authum_events_name_is_own(self, name))
    {
        return authum_event_result_continue;
    }
    return authum_events_ops_get_publish(authum_events_get_ops(self))(authum_events_get_owner(self),
                                                                      name, payload);
}

authum_event_result_t
authum_events_query(const authum_events_t *self, const authum_char_t *name, authum_ptr payload)
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
