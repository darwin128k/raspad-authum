#include <authum/detour.h>

#include <lh/os/memory.h>

#include <stdint.h>
#include <string.h>

/* rel32 reaches anywhere only in a 32-bit address space. */
#if defined(__i386__) || defined(_M_IX86)
#define AUTHUM_DETOUR_X86 1
#else
#define AUTHUM_DETOUR_X86 0
#endif

authum_bool_t
authum_detour_unprotect(authum_ptr address, authum_usize_t size)
{
    return lh_os_memory_protect(address, (lh_usize_t)size,
                                lh_os_memory_access_read | lh_os_memory_access_write |
                                    lh_os_memory_access_execute)
               ? authum_bool_true
               : authum_bool_false;
}

void
authum_detour_write_jump(authum_ptr target, authum_ptr hook)
{
    authum_byte_t *t = (authum_byte_t *)target;
    int32_t rel = (int32_t)((intptr_t)hook - (intptr_t)target - AUTHUM_DETOUR_SIZE);

    t[0] = 0xE9u;
    memcpy(t + 1, &rel, sizeof rel);
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
    memcpy(self->saved, target, AUTHUM_DETOUR_SIZE);
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
    memcpy(self->target, self->saved, AUTHUM_DETOUR_SIZE);
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
