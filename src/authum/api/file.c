/**
 * @file file.c
 * @brief ::authum_file_t — the module's own file, over the lh filesystem.
 *
 * The lh file object lives behind the opaque pointer of ::authum_file_t: the
 * public header may not name an lh type, and a module may not link lh. The
 * module's path is an ::authum_path_t, the lh path is built from it here.
 */

#include <authum/file.h>
#include <authum/memory.h>

#include <lh/cast/const.h>
#include <lh/char/map.h>
#include <lh/os/fs/file.h>
#include <lh/os/system/fs/path.h>

/** The lh file behind @p self, or null when the file is closed. */
static lh_os_fs_file_t *
authum_file_backend(const authum_file_t *self)
{
    return lh_cast_static(lh_os_fs_file_t *, authum_file_get_state(self));
}

void
authum_file_init(authum_file_t *self)
{
    authum_file_set_state(self, authum_null);
}

authum_ptr
authum_file_get_state(const authum_file_t *self)
{
    return authum_null_eq(self) ? authum_null : self->state;
}

void
authum_file_set_state(authum_file_t *self, authum_ptr state)
{
    if (!authum_null_eq(self))
    {
        self->state = state;
    }
}

authum_bool_t
authum_file_is_open(const authum_file_t *self)
{
    return authum_null_eq(authum_file_get_state(self)) ? authum_bool_false : authum_bool_true;
}

authum_bool_t
authum_file_read(authum_file_t *self, authum_ptr buf, authum_usize_t size, authum_usize_t *read)
{
    lh_os_fs_file_t *file = authum_file_backend(self);
    lh_ssize_t count;

    if (authum_null_eq(file) || authum_null_eq(buf) || authum_null_eq(read))
    {
        return authum_bool_false;
    }
    count = lh_os_fs_file_read(file, buf, size);
    if (count < 0)
    {
        return authum_bool_false;
    }
    lh_ptr_deref(read) = lh_cast_static(authum_usize_t, count);
    return authum_bool_true;
}

authum_bool_t
authum_file_write(authum_file_t *self, authum_cptr buf, authum_usize_t size,
                  authum_usize_t *written)
{
    lh_os_fs_file_t *file = authum_file_backend(self);
    lh_ssize_t count;

    if (authum_null_eq(file) || authum_null_eq(buf) || authum_null_eq(written))
    {
        return authum_bool_false;
    }
    count = lh_os_fs_file_write(file, lh_cast_const(lh_ptr, buf), size);
    if (count < 0)
    {
        return authum_bool_false;
    }
    lh_ptr_deref(written) = lh_cast_static(authum_usize_t, count);
    return authum_bool_true;
}

authum_bool_t
authum_file_read_all(authum_file_t *self, authum_ptr buf, authum_usize_t buf_size,
                     authum_usize_t *size)
{
    authum_usize_t total = 0u;

    if (authum_null_eq(buf) || authum_null_eq(size))
    {
        return authum_bool_false;
    }
    for (;;)
    {
        authum_char_t extra;
        authum_usize_t count = 0u;
        authum_usize_t room = lh_math_sub(buf_size, total);

        if (lh_math_gt(room, 0u))
        {
            if (!authum_file_read(self,
                                  lh_ptr_add_by_offset(lh_char_t, buf, (lh_uaddr_t)total), room,
                                  &count))
            {
                return authum_bool_false;
            }
            if (lh_math_eq(count, 0u))
            {
                break;
            }
            total = lh_math_add(total, count);
            continue;
        }
        /* The buffer is full: one more byte tells whether the file fits. */
        if (!authum_file_read(self, &extra, 1u, &count))
        {
            return authum_bool_false;
        }
        if (lh_math_gt(count, 0u))
        {
            return authum_bool_false;
        }
        break;
    }
    lh_ptr_deref(size) = total;
    if (lh_math_lt(total, buf_size))
    {
        /* The text fits with room for a terminator: the file reads as a string. */
        lh_ptr_deref(lh_ptr_add_by_offset(lh_char_t, buf, (lh_uaddr_t)total)) = lh_char_map_nul;
    }
    return authum_bool_true;
}

void
authum_file_close(authum_file_t *self)
{
    lh_os_fs_file_t *file = authum_file_backend(self);

    if (authum_null_eq(file))
    {
        return;
    }
    lh_os_fs_file_deinit(file);
    authum_memory_free(file);
    authum_file_set_state(self, authum_null);
}

authum_bool_t
authum_file_open_cstr(authum_file_t *self, const authum_char_t *path, authum_file_mode_t mode)
{
    lh_fs_path_t lh_path;
    lh_str_view_t view;
    lh_os_fs_file_t *file;
    lh_bool_t ok;

    if (authum_null_eq(self) || authum_null_eq(path))
    {
        return authum_bool_false;
    }
    authum_file_close(self);
    lh_fs_path_init(lh_addr_of(lh_path));
    view = lh_initializer_of(lh_str_view_t, lh_str_view_initializer, path, lh_str_ptr_len(path));
    lh_fs_path_set(lh_addr_of(lh_path), view, lh_os_system_fs_path_style_native());
    file = lh_cast_static(lh_os_fs_file_t *, authum_memory_alloc(sizeof *file));
    if (authum_null_eq(file))
    {
        lh_fs_path_deinit(lh_addr_of(lh_path));
        return authum_bool_false;
    }
    lh_os_fs_file_init(file);
    /* The mode values are the same on both sides, in the same order. */
    ok = lh_os_fs_file_open(file, lh_addr_of(lh_path), lh_cast_static(lh_fs_file_mode_t, mode));
    lh_fs_path_deinit(lh_addr_of(lh_path));
    if (!ok)
    {
        lh_os_fs_file_deinit(file);
        authum_memory_free(file);
        return authum_bool_false;
    }
    authum_file_set_state(self, lh_cast_static(authum_ptr, file));
    return authum_bool_true;
}

authum_bool_t
authum_file_open(authum_file_t *self, const authum_path_t *path, authum_file_mode_t mode)
{
    if (authum_null_eq(path))
    {
        return authum_bool_false;
    }
    return authum_file_open_cstr(self, authum_path_get_cstr(path), mode);
}
