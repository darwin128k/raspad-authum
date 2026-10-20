/**
 * @file env.h
 * @brief Host filesystem roots for modules (game / addons / models / …).
 *
 * Filled by `authum_mm` from the module image path (and engine game dir as a
 * fallback). Paths are ::authum_path_t values owned by the host, valid until
 * `stop`. Cross-platform: absolute paths on the current volume when the host
 * can resolve them.
 *
 * Typical HLDS layout:
 * @code
 *   <game>/                  ← env->game
 *     models/                ← env->models
 *     sprites/               ← env->sprites
 *     addons/                ← env->addons
 *       authum/              ← env->authum
 *         modules/           ← env->modules
 *           qproto/          ← env->module
 * @endcode
 */

#ifndef AUTHUM_ENV_H
#define AUTHUM_ENV_H

#include <authum/null.h>
#include <authum/path.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct authum_env
 * @brief Snapshot of well-known directories for one loaded module.
 *
 * Each field is null when unknown; otherwise points at a host-owned
 * ::authum_path_t valid until `stop`.
 */
struct authum_env
{
    const authum_path_t *game; /**< Game dir (`cstrike`, …). */
    const authum_path_t *models; /**< `game/models`. */
    const authum_path_t *sprites; /**< `game/sprites`. */
    const authum_path_t *addons; /**< `game/addons`. */
    const authum_path_t *authum; /**< `addons/authum`. */
    const authum_path_t *modules; /**< `authum/modules`. */
    const authum_path_t *module; /**< This module’s directory. */
};
typedef struct authum_env authum_env_t;

const authum_path_t *
authum_env_game(const authum_env_t *env);

const authum_path_t *
authum_env_models(const authum_env_t *env);

const authum_path_t *
authum_env_sprites(const authum_env_t *env);

const authum_path_t *
authum_env_addons(const authum_env_t *env);

const authum_path_t *
authum_env_authum(const authum_env_t *env);

const authum_path_t *
authum_env_modules(const authum_env_t *env);

const authum_path_t *
authum_env_module(const authum_env_t *env);

const authum_char_t *
authum_env_module_cstr(const authum_env_t *env);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENV_H */
