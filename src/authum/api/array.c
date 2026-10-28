#include <authum/array.h>
#include <lh/cast/reinterpret.h>
#include <lh/math/ops.h>
#include <lh/util/ptr.h>
#include <lh/memory/std.h>
#include <lh/numeric/fixed/limits.h>

#include <authum/null.h>

#include <string.h>

authum_bool_t
authum_array_init(authum_array_t *self, const authum_sandbox_t *sandbox, authum_usize_t item_size)
{
    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    lh_memory_std_set(self, 0, sizeof lh_ptr_deref(self));
    if (!authum_sandbox_is_valid(sandbox) || lh_math_eq(item_size, 0u))
    {
        return authum_bool_false;
    }
    self->sandbox = lh_ptr_deref(sandbox);
    self->item_size = item_size;
    return authum_bool_true;
}

void
authum_array_deinit(authum_array_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    authum_sandbox_free(lh_addr_of(self->sandbox), self->data);
    self->data = authum_null;
    self->size = 0u;
    self->capacity = 0u;
}

authum_usize_t
authum_array_size(const authum_array_t *self)
{
    return authum_null_eq(self) ? 0u : self->size;
}

authum_bool_t
authum_array_is_empty(const authum_array_t *self)
{
    return (lh_math_eq(authum_array_size(self), 0u)) ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_array_reserve(authum_array_t *self, authum_usize_t capacity)
{
    authum_ptr grown;

    if (authum_null_eq(self) || lh_math_eq(self->item_size, 0u))
    {
        return authum_bool_false;
    }
    if (lh_math_le(capacity, self->capacity))
    {
        return authum_bool_true;
    }
    if (lh_math_gt(capacity, lh_math_div((LH_U32_T_MAX), self->item_size)))
    {
        return authum_bool_false;
    }
    grown = authum_sandbox_realloc(lh_addr_of(self->sandbox), self->data,
                                   lh_math_mul(capacity, self->item_size));
    if (authum_null_eq(grown))
    {
        return authum_bool_false;
    }
    self->data = grown;
    self->capacity = capacity;
    return authum_bool_true;
}

authum_ptr
authum_array_push(authum_array_t *self, authum_cptr item)
{
    authum_byte_t *slot;
    authum_usize_t want;

    if (authum_null_eq(self))
    {
        return authum_null;
    }
    if (lh_math_eq(self->size, self->capacity))
    {
        want = (lh_math_eq(self->capacity, 0u)) ? 4u : lh_math_mul(self->capacity, 2u);
        if (!authum_array_reserve(self, want))
        {
            return authum_null;
        }
    }
    slot = lh_math_add(lh_cast_reinterpret(authum_byte_t *, self->data),
                       lh_math_mul(self->size, self->item_size));
    if (authum_null_eq(item))
    {
        lh_memory_std_set(slot, 0, self->item_size);
    }
    else
    {
        lh_memory_std_copy(slot, item, self->item_size);
    }
    self->size = lh_math_add_one(self->size);
    return slot;
}

authum_ptr
authum_array_at(const authum_array_t *self, authum_usize_t index)
{
    if (authum_null_eq(self) || lh_math_ge(index, self->size))
    {
        return authum_null;
    }
    return lh_math_add(lh_cast_reinterpret(authum_byte_t *, self->data),
                       lh_math_mul(index, self->item_size));
}

authum_bool_t
authum_array_remove_at(authum_array_t *self, authum_usize_t index)
{
    authum_byte_t *base;

    if (authum_null_eq(self) || lh_math_ge(index, self->size))
    {
        return authum_bool_false;
    }
    base = lh_cast_reinterpret(authum_byte_t *, self->data);
    lh_memory_std_move(
        lh_math_add(base, lh_math_mul(index, self->item_size)),
        lh_math_add(base, lh_math_mul((lh_math_add(index, 1u)), self->item_size)),
        lh_math_mul((lh_math_sub(lh_math_sub(self->size, index), 1u)), self->item_size));
    self->size = lh_math_sub_one(self->size);
    return authum_bool_true;
}

void
authum_array_clear(authum_array_t *self)
{
    if (!authum_null_eq(self))
    {
        self->size = 0u;
    }
}
