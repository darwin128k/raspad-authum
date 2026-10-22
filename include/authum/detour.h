/**
 * @file detour.h
 * @brief 5-byte `jmp rel32` detour on an x86 function (HLDS is 32-bit).
 *
 * ::authum_detour_attach overwrites the first 5 bytes of @c target with a jump
 * to @c hook and keeps the originals. The engine runs its frame on one thread,
 * so a hook calls the original by ::authum_detour_suspend (original bytes
 * back), calling @c target, then ::authum_detour_resume (jump again):
 *
 * @code
 * authum_detour_t g_detour;
 *
 * int my_hook(void *a)
 * {
 *     int r;
 *     authum_detour_suspend(&g_detour);
 *     r = ((int (*)(void *))g_detour.target)(a);
 *     authum_detour_resume(&g_detour);
 *     return r;
 * }
 *
 * authum_detour_attach(&g_detour, target, (authum_ptr)&my_hook);
 * ...
 * authum_detour_detach(&g_detour);
 * @endcode
 *
 * Not thread-safe, not reentrant across threads: only for engine-thread code.
 * On non-x86 targets every call fails / does nothing.
 */

#ifndef AUTHUM_DETOUR_H
#define AUTHUM_DETOUR_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Bytes a detour overwrites at the target (`E9` + rel32). */
#define AUTHUM_DETOUR_SIZE 5

    /**
     * @struct authum_detour
     * @brief One patched function. Zero-initialize before the first attach.
     */
    typedef struct authum_detour
    {
        authum_ptr target;                       /**< patched function */
        authum_ptr hook;                         /**< where the jump goes */
        authum_byte_t saved[AUTHUM_DETOUR_SIZE]; /**< original bytes */
        authum_bool_t attached;                  /**< between attach and detach */
        authum_bool_t patched;                   /**< jump currently written */
    } authum_detour_t;

    /**
     * Save the first ::AUTHUM_DETOUR_SIZE bytes of @p target and write a jump
     * to @p hook. Fails (returns false, changes nothing) when already attached,
     * on null pointers, when the page cannot be made writable, or on non-x86.
     */
    authum_bool_t
    authum_detour_attach(authum_detour_t *self, authum_ptr target, authum_ptr hook);

    /** Restore the original bytes for good. Safe on a detached detour. */
    void
    authum_detour_detach(authum_detour_t *self);

    /** Put the original bytes back for a call-through; keeps the detour attached. */
    void
    authum_detour_suspend(authum_detour_t *self);

    /** Write the jump again after ::authum_detour_suspend. */
    void
    authum_detour_resume(authum_detour_t *self);

    /** Make @p size bytes at @p address writable + executable (both bricks use it). */
    authum_bool_t
    authum_detour_unprotect(authum_ptr address, authum_usize_t size);

    /** Write the `E9 rel32` jump from @p target to @p hook (no save, no checks). */
    void
    authum_detour_write_jump(authum_ptr target, authum_ptr hook);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_DETOUR_H */
