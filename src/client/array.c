#include <authum/array.h>

#include <authum/null.h>

#include <string.h>

authum_bool_t
authum_array_init(authum_array_t *self, const authum_sandbox_t *sandbox, authum_usize_t item_size)
{
    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    memset(self, 0, sizeof *self);
    if (!authum_sandbox_is_valid(sandbox) || item_size == 0u)
    {
        return authum_bool_false;
    }
    self->sandbox = *sandbox;
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
    authum_sandbox_free(&self->sandbox, self->data);
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
    return (authum_array_size(self) == 0u) ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_array_reserve(authum_array_t *self, authum_usize_t capacity)
{
    authum_ptr grown;

    if (authum_null_eq(self) || self->item_size == 0u)
    {
        return authum_bool_false;
    }
    if (capacity <= self->capacity)
    {
        return authum_bool_true;
    }
    if (capacity > ((authum_usize_t)-1) / self->item_size)
    {
        return authum_bool_false;
    }
    grown = authum_sandbox_realloc(&self->sandbox, self->data, capacity * self->item_size);
    if (authum_null_eq(grown))
    {
        return authum_bool_false;
    }
    self->data = grown;
    self->capacity = capacity;
    return authum_bool_true;
}

authum_ptr
authum_array_push(authum_array_t *self, const void *item)
{
    authum_byte_t *slot;
    authum_usize_t want;

    if (authum_null_eq(self))
    {
        return authum_null;
    }
    if (self->size == self->capacity)
    {
        want = (self->capacity == 0u) ? 4u : self->capacity * 2u;
        if (!authum_array_reserve(self, want))
        {
            return authum_null;
        }
    }
    slot = (authum_byte_t *)self->data + self->size * self->item_size;
    if (authum_null_eq(item))
    {
        memset(slot, 0, self->item_size);
    }
    else
    {
        memcpy(slot, item, self->item_size);
    }
    ++self->size;
    return slot;
}

authum_ptr
authum_array_at(const authum_array_t *self, authum_usize_t index)
{
    if (authum_null_eq(self) || index >= self->size)
    {
        return authum_null;
    }
    return (authum_byte_t *)self->data + index * self->item_size;
}

authum_bool_t
authum_array_remove_at(authum_array_t *self, authum_usize_t index)
{
    authum_byte_t *base;

    if (authum_null_eq(self) || index >= self->size)
    {
        return authum_bool_false;
    }
    base = (authum_byte_t *)self->data;
    memmove(base + index * self->item_size, base + (index + 1u) * self->item_size,
            (self->size - index - 1u) * self->item_size);
    --self->size;
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
