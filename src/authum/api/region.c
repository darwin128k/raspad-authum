#include <authum/region.h>

#include <authum/null.h>

authum_bool_t
authum_region_is_valid(const authum_region_t *self)
{
    return (!authum_null_eq(self) && !authum_null_eq(self->ops) && !authum_null_eq(self->root))
               ? authum_bool_true
               : authum_bool_false;
}

authum_ptr
authum_region_alloc(const authum_region_t *self, authum_usize_t size)
{
    return authum_region_alloc_under(self, authum_null, size);
}

authum_ptr
authum_region_alloc_under(const authum_region_t *self, authum_ptr parent, authum_usize_t size)
{
    if (!authum_region_is_valid(self) || authum_null_eq((self->ops)->alloc))
    {
        return authum_null;
    }
    return self->ops->alloc(authum_null_eq(parent) ? self->root : parent, size);
}

authum_ptr
authum_region_realloc(const authum_region_t *self, authum_ptr ptr, authum_usize_t size)
{
    if (authum_null_eq(ptr))
    {
        return authum_region_alloc(self, size);
    }
    if (!authum_region_is_valid(self) || authum_null_eq((self->ops)->realloc))
    {
        return authum_null;
    }
    return self->ops->realloc(ptr, size);
}

void
authum_region_free(const authum_region_t *self, authum_ptr ptr)
{
    if (authum_null_eq(ptr) || !authum_region_is_valid(self) || authum_null_eq((self->ops)->free))
    {
        return;
    }
    self->ops->free(ptr);
}

void
authum_region_set_destructor(const authum_region_t *self, authum_ptr ptr,
                             authum_region_destructor_fn fn)
{
    if (authum_null_eq(ptr) || !authum_region_is_valid(self) ||
        authum_null_eq((self->ops)->set_destructor))
    {
        return;
    }
    self->ops->set_destructor(ptr, fn);
}

authum_bool_t
authum_region_open(authum_region_t *self, const authum_region_t *parent)
{
    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    self->ops = authum_null;
    self->root = authum_null;
    if (!authum_region_is_valid(parent))
    {
        return authum_bool_false;
    }
    self->root = authum_region_alloc(parent, 0u);
    if (authum_null_eq(self->root))
    {
        return authum_bool_false;
    }
    self->ops = parent->ops;
    return authum_bool_true;
}

void
authum_region_close(authum_region_t *self)
{
    if (!authum_region_is_valid(self))
    {
        return;
    }
    authum_region_free(self, self->root);
    self->ops = authum_null;
    self->root = authum_null;
}
