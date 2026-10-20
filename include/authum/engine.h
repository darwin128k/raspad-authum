/**
 * @file engine.h
 * @brief Host engine facade for modules — subset of engfuncs, no HLSDK.
 *
 * Full `enginefuncs_t` / `globalvars_t` stay inside `authum_mm`. Modules that
 * only need game-dir / console / server-command go through ::authum_engine_t
 * on ::authum_host_t.engine. Raw pointers remain on the host as an escape hatch
 * (`engine_funcs` / `globals`) for modules that link HLSDK privately.
 */

#ifndef AUTHUM_ENGINE_H
#define AUTHUM_ENGINE_H

#include <authum/byte.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct authum_engine
 * @brief Ops table filled by the host from HLDS engfuncs.
 */
typedef struct authum_engine
{
    /**
     * Write the game directory (e.g. `cstrike`) into @p buf.
     * Always NUL-terminates when @p buf_size > 0.
     */
    void (*get_game_dir)(authum_char_t *buf, authum_usize_t buf_size);
    /** Mirror of `pfnServerPrint` (text as-is, typically already `\n`-terminated). */
    void (*server_print)(const authum_char_t *text);
    /** Mirror of `pfnServerCommand` (e.g. `kick #3\n`). */
    void (*server_command)(const authum_char_t *cmd);
} authum_engine_t;

void
authum_engine_get_game_dir(const authum_engine_t *engine, authum_char_t *buf, authum_usize_t buf_size);

void
authum_engine_server_print(const authum_engine_t *engine, const authum_char_t *text);

void
authum_engine_server_command(const authum_engine_t *engine, const authum_char_t *cmd);

/**
 * Issue `kick #userid "reason"`. Quotes, `;` and line breaks in @p reason are
 * replaced with spaces so the command cannot be split or injected.
 */
void
authum_engine_kick(const authum_engine_t *engine, authum_i32_t userid, const authum_char_t *reason);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENGINE_H */
