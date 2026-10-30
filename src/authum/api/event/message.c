#include <authum/event/message.h>
#include <lh/util/ptr.h>
#include <lh/util/str/ptr.h>
#include <lh/memory/std.h>

#include <authum/null.h>

const authum_event_message_arg_t *
authum_event_message_arg(const authum_event_message_t *ev, authum_i32_t i)
{
    if (authum_null_eq(ev) || lh_math_lt(i, 0) || lh_math_ge(i, ev->argc) ||
        lh_math_ge(i, authum_event_message_args_max))
    {
        return authum_null;
    }
    return lh_addr_of(ev->args[i]);
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

    if (authum_null_eq(arg) || lh_math_ne(arg->kind, authum_message_arg_string) ||
        lh_math_ge(arg->text, authum_event_message_text_max))
    {
        return "";
    }
    return lh_math_add(ev->text, arg->text);
}

authum_bool_t
authum_event_message_set_i32(authum_event_message_t *ev, authum_i32_t i, authum_i32_t value)
{
    authum_event_message_arg_t *arg =
        lh_cast_reinterpret(authum_event_message_arg_t *, authum_event_message_arg(ev, i));

    if (authum_null_eq(arg) || lh_math_eq(arg->kind, authum_message_arg_string))
    {
        return authum_bool_false;
    }
    arg->value = value;
    return authum_bool_true;
}

authum_bool_t
authum_event_message_set_float(authum_event_message_t *ev, authum_i32_t i, authum_float_t value)
{
    authum_event_message_arg_t *arg =
        lh_cast_reinterpret(authum_event_message_arg_t *, authum_event_message_arg(ev, i));

    if (authum_null_eq(arg) || lh_math_eq(arg->kind, authum_message_arg_string))
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
    authum_event_message_arg_t *arg =
        lh_cast_reinterpret(authum_event_message_arg_t *, authum_event_message_arg(ev, i));
    authum_usize_t n;

    if (authum_null_eq(arg) || lh_math_ne(arg->kind, authum_message_arg_string) ||
        authum_null_eq(text))
    {
        return authum_bool_false;
    }
    /* The new text goes after the others; the old one just stops being used. */
    n = lh_math_add(lh_str_ptr_len(text), 1u);
    if (lh_math_gt(lh_math_add(ev->text_used, n), authum_event_message_text_max))
    {
        return authum_bool_false;
    }
    lh_memory_std_copy(lh_math_add(ev->text, ev->text_used), text, n);
    arg->text = ev->text_used;
    ev->text_used += lh_cast_static(authum_u32_t, n);
    return authum_bool_true;
}
