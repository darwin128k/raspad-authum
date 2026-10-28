#include <authum/detour.h>
#include <lh/cast/reinterpret.h>
#include <lh/cast/static.h>
#include <lh/math/ops.h>
#include <lh/util/ptr.h>
#include <lh/memory/std.h>

#include <lh/os/memory.h>

#include <string.h>

/* rel32 reaches anywhere only in a 32-bit address space. */
#if defined(__i386__) || defined(_M_IX86)
#    define AUTHUM_DETOUR_X86 1
#else
#    define AUTHUM_DETOUR_X86 0
#endif

authum_bool_t
authum_detour_unprotect(authum_ptr address, authum_usize_t size)
{
    return lh_os_memory_protect(
               address, lh_cast_static(lh_usize_t, size),
               lh_math_bit_or(lh_math_bit_or(lh_os_memory_access_read, lh_os_memory_access_write),
                              lh_os_memory_access_execute))
               ? authum_bool_true
               : authum_bool_false;
}

void
authum_detour_write_jump(authum_ptr target, authum_ptr hook)
{
    authum_byte_t *t = lh_cast_reinterpret(authum_byte_t *, target);
    authum_i32_t rel =
        lh_cast_static(authum_i32_t, lh_math_sub(lh_ptr_udiff(hook, target), AUTHUM_DETOUR_SIZE));

    t[0] = 0xE9u;
    lh_memory_std_copy(lh_math_add(t, 1), lh_addr_of(rel), sizeof rel);
}

authum_bool_t
authum_detour_attach(authum_detour_t *self, authum_ptr target, authum_ptr hook)
{
    if (!AUTHUM_DETOUR_X86 || authum_null_eq(self) || authum_null_eq(target) ||
        authum_null_eq(hook) || self->attached)
    {
        return authum_bool_false;
    }
    if (!authum_detour_unprotect(target, AUTHUM_DETOUR_SIZE))
    {
        return authum_bool_false;
    }
    self->target = target;
    self->hook = hook;
    lh_memory_std_copy(self->saved, target, AUTHUM_DETOUR_SIZE);
    authum_detour_write_jump(target, hook);
    self->attached = authum_bool_true;
    self->patched = authum_bool_true;
    return authum_bool_true;
}

void
authum_detour_detach(authum_detour_t *self)
{
    if (authum_null_eq(self) || !self->attached)
    {
        return;
    }
    authum_detour_suspend(self);
    self->attached = authum_bool_false;
}

void
authum_detour_suspend(authum_detour_t *self)
{
    if (authum_null_eq(self) || !self->attached || !self->patched)
    {
        return;
    }
    lh_memory_std_copy(self->target, self->saved, AUTHUM_DETOUR_SIZE);
    self->patched = authum_bool_false;
}

void
authum_detour_resume(authum_detour_t *self)
{
    if (authum_null_eq(self) || !self->attached || self->patched)
    {
        return;
    }
    authum_detour_write_jump(self->target, self->hook);
    self->patched = authum_bool_true;
}
