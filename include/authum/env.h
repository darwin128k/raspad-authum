/**
 * @file env.h
 * @brief Where things live: the game, Authum, and this module's own folder.
 *
 * Filled by `authum` from its own directory (and the engine game dir as a
 * fallback). Paths are ::authum_path_t values owned by the host, valid until
 * `stop`. Absolute paths on the current volume when the host can resolve them.
 *
 * Every module is a sandbox: one folder holds everything that is its own.
 * @code
 *   <game>/                        ← env->game
 *     addons/                      ← env->addons
 *       authum/                    ← env->authum: authum.so
 *         configs/                     authum.cfg, modules.list
 *         logs/                        Authum's own log
 *         modules/                 ← env->modules
 *           qproto/                ← env->module
 *             qproto.so
 *             configs/             ← env->module_config: the admin's (qproto.cfg, …)
 *             lang/                ← env->module_lang: translations
 *             data/                ← env->module_data: what the module writes
 *             logs/                    the module's log (see <authum/logger.h>)
 * @endcode
 *
 * A package or an update replaces the image and `lang/`; it never touches
 * `configs/`, `data/` or `logs/`.
 *
 * Build file paths with ::authum_env_config_path and ::authum_env_data_path
 * (or their `authum_host_*` shortcuts).
 */

#ifndef AUTHUM_ENV_H
#define AUTHUM_ENV_H

#include <authum/path.h>

/**
 * @struct authum_env
 * @brief Well-known directories for one loaded module.
 *
 * Each path is null when unknown; otherwise points at a host-owned
 * ::authum_path_t valid until `stop`. Append only (see ::AUTHUM_MODULE_ABI).
 */
struct authum_env
{
    const authum_path_t *game;          /**< Game dir (`cstrike`, …). */
    const authum_path_t *models;        /**< `game/models`. */
    const authum_path_t *sprites;       /**< `game/sprites`. */
    const authum_path_t *addons;        /**< `game/addons`. */
    const authum_path_t *authum;        /**< `addons/authum`, where `authum` is. */
    const authum_path_t *modules;       /**< `authum/modules`. */
    const authum_path_t *module;        /**< `modules/<module>`: this module's folder. */
    const authum_path_t *module_config; /**< `modules/<module>/configs`. */
    const authum_path_t *module_lang;   /**< `modules/<module>/lang`. */
    const authum_path_t *module_data;   /**< `modules/<module>/data` (location only). */
};
typedef struct authum_env authum_env_t;

AUTHUM_EXTERN_C_BEGIN

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

const authum_path_t *
authum_env_module_config(const authum_env_t *env);

const authum_path_t *
authum_env_module_lang(const authum_env_t *env);

const authum_path_t *
authum_env_module_data(const authum_env_t *env);

/**
 * `modules/<module>/configs/<leaf>` into @p out — a config file
 * (`qproto.cfg`, `whitelist.ini`). True when the file exists.
 */
authum_bool_t
authum_env_config_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out);

/**
 * `modules/<module>/data/<leaf>` into @p out without filesystem changes.
 * True when the path could be built; directories and files may not exist.
 */
authum_bool_t
authum_env_data_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_ENV_H */
