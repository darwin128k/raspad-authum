/**
 * @file def.c
 * @brief Accessors of ::authum_command_def_t.
 */

#include <authum/command/def.h>
#include <lh/assert/runtime.h>

const authum_char_t *
authum_command_def_get_name(const authum_command_def_t *self)
{
    lh_assert_runtime_ref(self);
    return self->name;
}

void
authum_command_def_set_name(authum_command_def_t *self, const authum_char_t *name)
{
    lh_assert_runtime_ref(self);
    self->name = name;
}

const authum_char_t *
authum_command_def_get_usage(const authum_command_def_t *self)
{
    lh_assert_runtime_ref(self);
    return self->usage;
}

void
authum_command_def_set_usage(authum_command_def_t *self, const authum_char_t *usage)
{
    lh_assert_runtime_ref(self);
    self->usage = usage;
}

const authum_char_t *
authum_command_def_get_help(const authum_command_def_t *self)
{
    lh_assert_runtime_ref(self);
    return self->help;
}

void
authum_command_def_set_help(authum_command_def_t *self, const authum_char_t *help)
{
    lh_assert_runtime_ref(self);
    self->help = help;
}

authum_command_fn
authum_command_def_get_fn(const authum_command_def_t *self)
{
    lh_assert_runtime_ref(self);
    return self->fn;
}

void
authum_command_def_set_fn(authum_command_def_t *self, authum_command_fn fn)
{
    lh_assert_runtime_ref(self);
    self->fn = fn;
}

authum_ptr
authum_command_def_get_user(const authum_command_def_t *self)
{
    lh_assert_runtime_ref(self);
    return self->user;
}

void
authum_command_def_set_user(authum_command_def_t *self, authum_ptr user)
{
    lh_assert_runtime_ref(self);
    self->user = user;
}
