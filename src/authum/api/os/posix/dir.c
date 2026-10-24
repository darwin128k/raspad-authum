#include "../os.h"

#include <errno.h>
#include <sys/stat.h>

authum_bool_t
authum_os_dir_ensure(const authum_char_t *dir)
{
    struct stat st;

    if (stat(dir, &st) == 0)
    {
        return S_ISDIR(st.st_mode) ? authum_bool_true : authum_bool_false;
    }
    if (mkdir(dir, 0755) == 0)
    {
        return authum_bool_true;
    }
    return (errno == EEXIST) ? authum_bool_true : authum_bool_false;
}
