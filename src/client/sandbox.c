#include <authum/sandbox.h>

#include <authum/null.h>

authum_bool_t
authum_sandbox_is_valid(const authum_sandbox_t *self)
{
    return (!authum_null_eq(self) && !authum_null_eq(self->ops) && !authum_null_eq(self->root))
               ? authum_bool_true
               : authum_bool_false;
}

authum_ptr
authum_sandbox_alloc(const authum_sandbox_t *self, authum_usize_t size)
{
    return authum_sandbox_alloc_under(self, authum_null, size);
}

authum_ptr
authum_sandbox_alloc_under(const authum_sandbox_t *self, authum_ptr parent, authum_usize_t size)
{
    if (!authum_sandbox_is_valid(self) || self->ops->alloc == 0)
    {
        return authum_null;
    }
    return self->ops->alloc(authum_null_eq(parent) ? self->root : parent, size);
}

authum_ptr
authum_sandbox_realloc(const authum_sandbox_t *self, authum_ptr ptr, authum_usize_t size)
{
    if (authum_null_eq(ptr))
    {
        return authum_sandbox_alloc(self, size);
    }
    if (!authum_sandbox_is_valid(self) || self->ops->realloc == 0)
    {
        return authum_null;
    }
    return self->ops->realloc(ptr, size);
}

void
authum_sandbox_free(const authum_sandbox_t *self, authum_ptr ptr)
{
    if (authum_null_eq(ptr) || !authum_sandbox_is_valid(self) || self->ops->free == 0)
    {
        return;
    }
    self->ops->free(ptr);
}

void
authum_sandbox_set_destructor(const authum_sandbox_t *self, authum_ptr ptr,
                              authum_sandbox_destructor_fn fn)
{
    if (authum_null_eq(ptr) || !authum_sandbox_is_valid(self) || self->ops->set_destructor == 0)
    {
        return;
    }
    self->ops->set_destructor(ptr, fn);
}

authum_bool_t
authum_sandbox_open(authum_sandbox_t *self, const authum_sandbox_t *parent)
{
    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    self->ops = authum_null;
    self->root = authum_null;
    if (!authum_sandbox_is_valid(parent))
    {
        return authum_bool_false;
    }
    self->root = authum_sandbox_alloc(parent, 0u);
    if (authum_null_eq(self->root))
    {
        return authum_bool_false;
    }
    self->ops = parent->ops;
    return authum_bool_true;
}

void
authum_sandbox_close(authum_sandbox_t *self)
{
    if (!authum_sandbox_is_valid(self))
    {
        return;
    }
    authum_sandbox_free(self, self->root);
    self->ops = authum_null;
    self->root = authum_null;
}
