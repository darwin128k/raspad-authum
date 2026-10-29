#include <authum/host/events/binding.h>
#include <authum/memory.h>

authum_event_handler_fn
authum_events_binding_get_fn(const authum_events_binding_t *self)
{
    return self->fn;
}

void
authum_events_binding_set_fn(authum_events_binding_t *self, authum_event_handler_fn fn)
{
    self->fn = fn;
}

authum_ptr
authum_events_binding_get_user(const authum_events_binding_t *self)
{
    return self->user;
}

void
authum_events_binding_set_user(authum_events_binding_t *self, authum_ptr user)
{
    self->user = user;
}

lh_void
authum_events_binding_destroy(lh_ptr context)
{
    authum_memory_free(context);
}

lh_os_module_event_result_t
authum_events_binding_call(const lh_char_t *name, lh_ptr payload, lh_ptr context)
{
    const authum_events_binding_t *self = lh_cast_static(const authum_events_binding_t *, context);
    return lh_cast_static(
        lh_os_module_event_result_t,
        authum_events_binding_get_fn(self)(name, payload, authum_events_binding_get_user(self)));
}

lh_bool_t
authum_events_binding_matches(lh_ptr context, lh_ptr key)
{
    const authum_events_binding_t *left = lh_cast_static(const authum_events_binding_t *, context);
    const authum_events_binding_t *right = lh_cast_static(const authum_events_binding_t *, key);
    return lh_math_eq(authum_events_binding_get_fn(left), authum_events_binding_get_fn(right)) &&
                   lh_math_eq(authum_events_binding_get_user(left),
                              authum_events_binding_get_user(right))
               ? lh_bool_true
               : lh_bool_false;
}
