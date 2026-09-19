#include <authum/loader.h>
#include <lh/assert.h>
#include <lh/null.h>
#include <lh/util/addr.h>

authum_loader_t authum_loader;

void
authum_loader_init(void)
{
    lh_vector_init(lh_addr_of(authum_loader.modules), sizeof(authum_module_t));
}

void
authum_loader_deinit(void)
{
    authum_loader_unload_all(&authum_loader);
    lh_vector_deinit(lh_addr_of(authum_loader.modules));
}

lh_bool_t
authum_loader_load(authum_loader_t *self, lh_str_cptr path)
{
    authum_module_t module;

    lh_assert_runtime_ref(self);
    authum_module_init(&module);
    if (!authum_module_load(&module, path))
    {
        return lh_bool_false;
    }
    lh_vector_push_back(lh_addr_of(self->modules), lh_addr_of(module));
    return lh_bool_true;
}

void
authum_loader_unload_all(authum_loader_t *self)
{
    lh_usize_t i;
    lh_usize_t n;

    lh_assert_runtime_ref(self);
    n = lh_vector_get_size(lh_addr_of(self->modules));
    for (i = 0; i < n; i++)
    {
        authum_module_t *module = (authum_module_t *)lh_vector_get_ptr(lh_addr_of(self->modules), i);
        authum_module_unload(module);
    }
    lh_vector_clear(lh_addr_of(self->modules));
}

lh_usize_t
authum_loader_get_loaded(const authum_loader_t *self)
{
    lh_assert_runtime_ref(self);
    return lh_vector_get_size(lh_addr_of(self->modules));
}

authum_module_t *
authum_loader_get(authum_loader_t *self, lh_uindex_t index)
{
    lh_assert_runtime_ref(self);
    if (!lh_vector_is_valid_index(lh_addr_of(self->modules), index))
    {
        return lh_null;
    }
    return (authum_module_t *)lh_vector_get_ptr(lh_addr_of(self->modules), index);
}
