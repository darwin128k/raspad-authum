#include <authum/commands.h>
#include <lh/util/ptr.h>

#include <authum/null.h>

authum_bool_t
authum_commands_add(const authum_commands_t *self, const authum_command_def_t *def)
{
    if (authum_null_eq(self) || authum_null_eq(self->ops) || authum_null_eq((self->ops)->add) ||
        authum_null_eq(self->scope) || authum_null_eq(def))
    {
        return authum_bool_false;
    }
    return self->ops->add(self->owner, self->scope, def);
}

authum_bool_t
authum_commands_add_all(const authum_commands_t *self, const authum_command_def_t *defs,
                        authum_usize_t count)
{
    authum_usize_t i;

    if (authum_null_eq(defs))
    {
        return authum_bool_false;
    }
    for (i = 0u; lh_math_lt(i, count); i = lh_math_add_one(i))
    {
        if (!authum_commands_add(self, lh_addr_of(defs[i])))
        {
            return authum_bool_false;
        }
    }
    return authum_bool_true;
}

authum_bool_t
authum_commands_remove(const authum_commands_t *self, const authum_char_t *name)
{
    if (authum_null_eq(self) || authum_null_eq(self->ops) || authum_null_eq((self->ops)->remove) ||
        authum_null_eq(self->scope) || authum_null_eq(name))
    {
        return authum_bool_false;
    }
    return self->ops->remove(self->owner, self->scope, name);
}

const authum_char_t *
authum_commands_scope(const authum_commands_t *self)
{
    return authum_null_eq(self) ? authum_null : self->scope;
}
