#include <authum/version.h>
#include <lh/math/ops.h>

authum_bool_t
authum_version_is_compatible(const authum_version_t *have, const authum_version_t *need)
{
    if (authum_null_eq(have) || authum_null_eq(need))
    {
        return authum_bool_false;
    }
    if (lh_math_ne(have->major, need->major))
    {
        return authum_bool_false;
    }
    if (lh_math_eq(need->major, 0u))
    {
        return (lh_math_eq(have->minor, need->minor) && lh_math_ge(have->patch, need->patch))
                   ? authum_bool_true
                   : authum_bool_false;
    }
    if (lh_math_ne(have->minor, need->minor))
    {
        return (lh_math_gt(have->minor, need->minor)) ? authum_bool_true : authum_bool_false;
    }
    return (lh_math_ge(have->patch, need->patch)) ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_version_is_abi_compatible(const authum_version_t *module, const authum_version_t *host)
{
    if (authum_null_eq(module) || authum_null_eq(host) || lh_math_ne(module->major, host->major))
    {
        return authum_bool_false;
    }
    if (lh_math_eq(host->major, 0u))
    {
        return (lh_math_eq(module->minor, host->minor)) ? authum_bool_true : authum_bool_false;
    }
    return authum_bool_true;
}
