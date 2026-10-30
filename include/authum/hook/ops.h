/**
 * @file ops.h
 * @brief Host ops table behind ::authum_hooks_t (filled by `authum`).
 *
 * Modules call the `authum_hooks_*` functions in <authum/hook.h>, never these
 * pointers.
 */

#ifndef AUTHUM_HOOK_OPS_H
#define AUTHUM_HOOK_OPS_H

#include <authum/hook/def.h>

struct authum_hook_ops
{
    authum_bool_t (*add)(authum_ptr owner, const authum_char_t *tag, authum_hook_point_t point,
                         authum_hook_fn fn, authum_ptr user);
    authum_bool_t (*remove)(authum_ptr owner, authum_hook_point_t point, authum_hook_fn fn,
                            authum_ptr user);
};
typedef struct authum_hook_ops authum_hook_ops_t;

#endif /* AUTHUM_HOOK_OPS_H */
