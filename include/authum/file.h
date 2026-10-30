/**
 * @file file.h
 * @brief A module-owned file (::authum_file_t) — no lh, no libc.
 *
 * The module reads and writes its own files (allow lists, key stores, data
 * files) through this facade, so it never names a platform: the OS call lives
 * in lh, under `lh/os/fs/file.h`, and this header names no lh type.
 *
 * @code
 * authum_file_t file;
 *
 * authum_file_init(&file);
 * if (authum_file_open_cstr(&file, "modules/qproto/allow.txt", authum_file_mode_read))
 * {
 *     authum_char_t line[256];
 *     authum_usize_t read;
 *
 *     while (authum_file_read(&file, lh_addr_of(line), sizeof line, &read) && read)
 *     {
 *         ...
 *     }
 *     authum_file_close(&file);
 * }
 * @endcode
 */

#ifndef AUTHUM_FILE_H
#define AUTHUM_FILE_H

#include <authum/path.h>

/**
 * @typedef authum_file_mode_t
 * @brief How a file is opened.
 */
typedef enum authum_file_mode
{
    /** An existing file, read-only. */
    authum_file_mode_read = 0,
    /** A new or replaced file, write-only. */
    authum_file_mode_write = 1,
    /** A new or existing file, read and write. */
    authum_file_mode_readwrite = 2,
    /** A new or existing file, appended to. */
    authum_file_mode_append = 3
} authum_file_mode_t;

/**
 * @struct authum_file
 * @brief An open file. The state is opaque; keep the object on the stack.
 */
struct authum_file
{
    /** Opaque state of the open file; null when the file is closed. */
    authum_ptr state;
};
typedef struct authum_file authum_file_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Initialize @p self as a closed file. Call once on uninitialized storage.
 */
void
authum_file_init(authum_file_t *self);

/**
 * @brief The opaque state of @p self.
 * @return Null when the file is closed.
 */
authum_ptr
authum_file_get_state(const authum_file_t *self);

/**
 * @brief Take @p state as the file's state; the old state is not released.
 */
void
authum_file_set_state(authum_file_t *self, authum_ptr state);

/**
 * @brief Whether @p self holds an open file.
 */
authum_bool_t
authum_file_is_open(const authum_file_t *self);

/**
 * @brief Open @p path in @p mode.
 *
 * @p self must be initialized with ::authum_file_init.
 *
 * @param self  File to fill; an already open file in @p self is closed first.
 * @param path  Path of the file.
 * @param mode  How to open it.
 * @return ::authum_bool_true when the file is open.
 */
authum_bool_t
authum_file_open(authum_file_t *self, const authum_path_t *path, authum_file_mode_t mode);

/**
 * @brief Open @p path (as text) in @p mode.
 *
 * @p self must be initialized with ::authum_file_init.
 */
authum_bool_t
authum_file_open_cstr(authum_file_t *self, const authum_char_t *path, authum_file_mode_t mode);

/**
 * @brief Read at most @p size bytes into @p buf.
 *
 * @param read Receives the number of bytes read; 0 at the end of the file.
 * @return ::authum_bool_true when the read itself succeeded.
 */
authum_bool_t
authum_file_read(authum_file_t *self, authum_ptr buf, authum_usize_t size, authum_usize_t *read);

/**
 * @brief Write @p size bytes.
 *
 * @param written Receives the number of bytes written.
 * @return ::authum_bool_true when the write itself succeeded.
 */
authum_bool_t
authum_file_write(authum_file_t *self, authum_cptr buf, authum_usize_t size,
                  authum_usize_t *written);

/**
 * @brief Read the whole file into @p buf.
 *
 * @param buf      Buffer that receives the bytes; @p buf_size is its capacity.
 * @param size     Receives the number of bytes read; 0 for an empty file.
 * @return ::authum_bool_true when the file was read and fits; false when the
 *         file is longer than the buffer, or the read failed.
 */
authum_bool_t
authum_file_read_all(authum_file_t *self, authum_ptr buf, authum_usize_t buf_size,
                     authum_usize_t *size);

/**
 * @brief Close the file and release its state. Safe on a closed file.
 */
void
authum_file_close(authum_file_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_FILE_H */
