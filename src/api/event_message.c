#include <authum/event/message.h>

#include <authum/null.h>

#include <string.h>

const authum_event_message_arg_t *
authum_event_message_arg(const authum_event_message_t *ev, authum_i32_t i)
{
    if (authum_null_eq(ev) || ev->size < sizeof *ev || i < 0 || i >= ev->argc ||
        i >= authum_event_message_args_max)
    {
        return authum_null;
    }
    return &ev->args[i];
}

authum_i32_t
authum_event_message_get_i32(const authum_event_message_t *ev, authum_i32_t i)
{
    const authum_event_message_arg_t *arg = authum_event_message_arg(ev, i);

    return !authum_null_eq(arg) ? arg->value : 0;
}

authum_float_t
authum_event_message_get_float(const authum_event_message_t *ev, authum_i32_t i)
{
    const authum_event_message_arg_t *arg = authum_event_message_arg(ev, i);

    return !authum_null_eq(arg) ? arg->real : 0.0f;
}

const authum_char_t *
authum_event_message_get_string(const authum_event_message_t *ev, authum_i32_t i)
{
    const authum_event_message_arg_t *arg = authum_event_message_arg(ev, i);

    if (authum_null_eq(arg) || arg->kind != authum_message_arg_string ||
        arg->text >= authum_event_message_text_max)
    {
        return "";
    }
    return ev->text + arg->text;
}

authum_bool_t
authum_event_message_set_i32(authum_event_message_t *ev, authum_i32_t i, authum_i32_t value)
{
    authum_event_message_arg_t *arg = (authum_event_message_arg_t *)authum_event_message_arg(ev, i);

    if (authum_null_eq(arg) || arg->kind == authum_message_arg_string)
    {
        return authum_bool_false;
    }
    arg->value = value;
    return authum_bool_true;
}

authum_bool_t
authum_event_message_set_float(authum_event_message_t *ev, authum_i32_t i, authum_float_t value)
{
    authum_event_message_arg_t *arg = (authum_event_message_arg_t *)authum_event_message_arg(ev, i);

    if (authum_null_eq(arg) || arg->kind == authum_message_arg_string)
    {
        return authum_bool_false;
    }
    arg->real = value;
    return authum_bool_true;
}

authum_bool_t
authum_event_message_set_string(authum_event_message_t *ev, authum_i32_t i,
                                const authum_char_t *text)
{
    authum_event_message_arg_t *arg = (authum_event_message_arg_t *)authum_event_message_arg(ev, i);
    authum_usize_t n;

    if (authum_null_eq(arg) || arg->kind != authum_message_arg_string || authum_null_eq(text))
    {
        return authum_bool_false;
    }
    /* The new text goes after the others; the old one just stops being used. */
    n = strlen(text) + 1u;
    if (ev->text_used + n > authum_event_message_text_max)
    {
        return authum_bool_false;
    }
    memcpy(ev->text + ev->text_used, text, n);
    arg->text = ev->text_used;
    ev->text_used += (authum_u32_t)n;
    return authum_bool_true;
}
