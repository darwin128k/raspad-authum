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

#include <stddef.h>

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
    void (*get_game_dir)(char *buf, size_t buf_size);
    /** Mirror of `pfnServerPrint` (text as-is, typically already `\n`-terminated). */
    void (*server_print)(const char *text);
    /** Mirror of `pfnServerCommand` (e.g. `kick #3\n`). */
    void (*server_command)(const char *cmd);
} authum_engine_t;

static inline void
authum_engine_get_game_dir(const authum_engine_t *engine, char *buf, size_t buf_size)
{
    if (engine != 0 && engine->get_game_dir != 0) {
        engine->get_game_dir(buf, buf_size);
    } else if (buf != 0 && buf_size > 0u) {
        buf[0] = '\0';
    }
}

static inline void
authum_engine_server_print(const authum_engine_t *engine, const char *text)
{
    if (engine != 0 && engine->server_print != 0 && text != 0) {
        engine->server_print(text);
    }
}

static inline void
authum_engine_server_command(const authum_engine_t *engine, const char *cmd)
{
    if (engine != 0 && engine->server_command != 0 && cmd != 0) {
        engine->server_command(cmd);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENGINE_H */
