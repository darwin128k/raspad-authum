#include <authum/events/ops.h>
#include <authum/null.h>
#include <lh/math/ops.h>

authum_events_subscribe_cb
authum_events_ops_get_subscribe(const authum_events_ops_t *self)
{
    return self->subscribe;
}

authum_events_unsubscribe_cb
authum_events_ops_get_unsubscribe(const authum_events_ops_t *self)
{
    return self->unsubscribe;
}

authum_events_publish_cb
authum_events_ops_get_publish(const authum_events_ops_t *self)
{
    return self->publish;
}

authum_events_resolve_cb
authum_events_ops_get_resolve(const authum_events_ops_t *self)
{
    return self->resolve;
}

authum_events_subscribe_id_cb
authum_events_ops_get_subscribe_id(const authum_events_ops_t *self)
{
    return self->subscribe_id;
}

authum_events_unsubscribe_id_cb
authum_events_ops_get_unsubscribe_id(const authum_events_ops_t *self)
{
    return self->unsubscribe_id;
}

authum_events_publish_id_cb
authum_events_ops_get_publish_id(const authum_events_ops_t *self)
{
    return self->publish_id;
}

authum_bool_t
authum_events_ops_has_subscribe(const authum_events_ops_t *self)
{
    return !authum_null_eq(self) && !authum_null_eq(authum_events_ops_get_subscribe(self))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_events_ops_has_unsubscribe(const authum_events_ops_t *self)
{
    return !authum_null_eq(self) && !authum_null_eq(authum_events_ops_get_unsubscribe(self))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_events_ops_has_publish(const authum_events_ops_t *self)
{
    return !authum_null_eq(self) && !authum_null_eq(authum_events_ops_get_publish(self))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_events_ops_has_resolve(const authum_events_ops_t *self)
{
    return !authum_null_eq(self) && !authum_null_eq(authum_events_ops_get_resolve(self))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_events_ops_has_subscribe_id(const authum_events_ops_t *self)
{
    return !authum_null_eq(self) && !authum_null_eq(authum_events_ops_get_subscribe_id(self))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_events_ops_has_unsubscribe_id(const authum_events_ops_t *self)
{
    return !authum_null_eq(self) && !authum_null_eq(authum_events_ops_get_unsubscribe_id(self))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_events_ops_has_publish_id(const authum_events_ops_t *self)
{
    return !authum_null_eq(self) && !authum_null_eq(authum_events_ops_get_publish_id(self))
               ? authum_bool_true
               : authum_bool_false;
}
