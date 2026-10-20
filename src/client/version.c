#include <authum/version.h>

authum_bool_t
authum_version_is_compatible(const authum_version_t *have, const authum_version_t *need)
{
    if (authum_null_eq(have) || authum_null_eq(need)) {
        return authum_bool_false;
    }
    if (have->major != need->major) {
        return authum_bool_false;
    }
    if (need->major == 0u) {
        return (have->minor == need->minor && have->patch >= need->patch) ? authum_bool_true
                                                                         : authum_bool_false;
    }
    if (have->minor != need->minor) {
        return (have->minor > need->minor) ? authum_bool_true : authum_bool_false;
    }
    return (have->patch >= need->patch) ? authum_bool_true : authum_bool_false;
}
