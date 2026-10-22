#include <authum/events.h>

#include <authum/null.h>

authum_bool_t
authum_events_subscribe(const authum_events_t *self, const authum_char_t *name,
                        authum_event_handler_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) || authum_null_eq(self->ops) ||
        !AUTHUM_ABI_HAS(self->ops, subscribe) || authum_null_eq(name) || fn == 0)
    {
        return authum_bool_false;
    }
    return self->ops->subscribe(self->owner, name, fn, user);
}

authum_bool_t
authum_events_unsubscribe(const authum_events_t *self, const authum_char_t *name,
                          authum_event_handler_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) || authum_null_eq(self->ops) ||
        !AUTHUM_ABI_HAS(self->ops, unsubscribe) || authum_null_eq(name) || fn == 0)
    {
        return authum_bool_false;
    }
    return self->ops->unsubscribe(self->owner, name, fn, user);
}

authum_event_result_t
authum_events_publish(const authum_events_t *self, const authum_char_t *name, authum_ptr payload)
{
    if (authum_null_eq(self) || authum_null_eq(self->ops) || !AUTHUM_ABI_HAS(self->ops, publish) ||
        authum_null_eq(name))
    {
        return authum_event_result_continue;
    }
    return self->ops->publish(self->owner, name, payload);
}
