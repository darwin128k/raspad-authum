/**
 * @file info.h
 * @brief Module identity exported as `authum_module_info` (no lh dependency).
 */

#ifndef AUTHUM_MODULE_INFO_H
#define AUTHUM_MODULE_INFO_H

#include <authum/char.h>
#include <authum/export.h>
#include <authum/module/info/fields.h>
#include <authum/version.h>

/**
 * @def AUTHUM_MODULE_INFO_SYM
 * @brief Exported symbol name of ::authum_module_info_t.
 */
#define AUTHUM_MODULE_INFO_SYM "authum_module_info"

/**
 * @def AUTHUM_MODULE_ABI
 * @brief Loader ABI (`3.0.0`) — lh-free module API (`authum_module_api`).
 *
 * 3.0 renames the module identity field `logtag` to `tag`: it is the mark the
 * module is shown with in a log record, and the struct already says what it is.
 * No 2.x module loads.
 *
 * From here the ABI is frozen: nothing that exists changes or moves. New API
 * is added at the end of a table, or as a new table or a new structure, and
 * bumps the minor; an input structure a module fills in never grows (a new
 * field is a new structure). A module therefore runs on a host of the same
 * major and the same or a later minor, which
 * ::authum_version_is_abi_compatible checks at load.
 *
 * 3.3.0 — the command being run and info strings in the engine table:
 * `authum_engine_cmd_copy_argv` and `authum_engine_info_copy_value` (the
 * engine's Cmd_Argv and Info_ValueForKey, which Windows engines do not export);
 * and, client-side, `authum_symbol_find_pattern` /
 * `authum_symbol_find_engine_pattern` (engine code found by its bytes).
 *
 * 3.2.0 — the 64-bit numbers: `authum_cstr_format_u64` and
 * `authum_cstr_format_i64` (the mask formatter has no length modifiers); and
 * `authum_task_ops.at_exit` (`authum_tasks_at_exit`): calls run on a module's
 * task thread when it ends.
 *
 * 3.1.0 — the module string, byte and wall-clock bricks: `authum_cstr_len`,
 * `authum_cstr_is_equal_n`, `authum_cstr_starts_with`, `authum_cstr_ends_with`,
 * `authum_cstr_find`, `authum_cstr_find_char`, `authum_cstr_format_hex_bytes`,
 * `authum_cstr_parse_hex_bytes`, `authum_char_is_digit` and its siblings,
 * `authum_char_to_upper`, `authum_memory_copy`, `authum_memory_set`,
 * `authum_memory_is_equal`, `authum_memory_find`, `authum_time_now_seconds`,
 * `authum_time_format_utc`.
 */
#define AUTHUM_MODULE_ABI AUTHUM_VERSION(3, 3, 0)

struct authum_module_info
{
    authum_module_info_fields(const authum_char_t *, authum_version_t);
};

typedef struct authum_module_info authum_module_info_t;

AUTHUM_EXTERN_C_BEGIN

AUTHUM_MODULE_EXPORT extern authum_module_info_t authum_module_info;

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_MODULE_INFO_H */
