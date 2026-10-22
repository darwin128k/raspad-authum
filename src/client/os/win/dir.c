#include "../os.h"

#include <windows.h>

authum_bool_t
authum_os_dir_ensure(const authum_char_t *dir)
{
    DWORD attributes = GetFileAttributesA(dir);

    if (attributes != INVALID_FILE_ATTRIBUTES)
    {
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? authum_bool_true : authum_bool_false;
    }
    if (CreateDirectoryA(dir, NULL))
    {
        return authum_bool_true;
    }
    return GetLastError() == ERROR_ALREADY_EXISTS ? authum_bool_true : authum_bool_false;
}
