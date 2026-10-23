/**
 * @file env.h
 * @brief Where things live: game, Authum, and this module's config and data.
 *
 * Filled by `authum_mm` from its own directory (and the engine game dir as a
 * fallback). Paths are ::authum_path_t values owned by the host, valid until
 * `stop`. Absolute paths on the current volume when the host can resolve them.
 *
 * One layout, the AMX Mod X way — what the admin edits apart from what a
 * module writes:
 * @code
 *   <game>/                        ← env->game
 *     addons/                      ← env->addons
 *       authum/                    ← env->authum (authum_mm.so)
 *         modules/                 ← env->modules: qproto.so, gatekeeper.so, …
 *         configs/                 ← env->configs: authum.cfg, modules.list,
 *           qproto.cfg                 one <module>.cfg per module
 *           gatekeeper/            ← env->module_config: a module's other files
 *         data/                    ← env->data
 *           qproto/                ← env->module_data: what the module writes
 *           lang/                  ← env->lang: translations
 *         logs/                    ← the host's (see <authum/logger.h>)
 * @endcode
 *
 * Build file paths with ::authum_env_config_path, ::authum_env_module_config_path
 * and ::authum_env_data_path (or their `authum_host_*` shortcuts).
 */

#ifndef AUTHUM_ENV_H
#define AUTHUM_ENV_H

#include <authum/abi.h>
#include <authum/bool.h>
#include <authum/null.h>
#include <authum/path.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @struct authum_env
     * @brief Well-known directories for one loaded module.
     *
     * Each path is null when unknown; otherwise points at a host-owned
     * ::authum_path_t valid until `stop`. Append only (see <authum/abi.h>).
     */
    struct authum_env
    {
        /** sizeof this table as built by the host — see <authum/abi.h>. */
        authum_usize_t size;
        const authum_path_t *game;          /**< Game dir (`cstrike`, …). */
        const authum_path_t *models;        /**< `game/models`. */
        const authum_path_t *sprites;       /**< `game/sprites`. */
        const authum_path_t *addons;        /**< `game/addons`. */
        const authum_path_t *authum;        /**< `addons/authum`, where `authum_mm` is. */
        const authum_path_t *modules;       /**< `authum/modules`. */
        const authum_path_t *configs;       /**< `authum/configs`. */
        const authum_path_t *data;          /**< `authum/data`. */
        const authum_path_t *lang;          /**< `authum/data/lang`. */
        const authum_path_t *module_config; /**< `configs/<module>` (may not exist). */
        const authum_path_t *module_data;   /**< `data/<module>` (created at load). */
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
    authum_env_configs(const authum_env_t *env);

    const authum_path_t *
    authum_env_data(const authum_env_t *env);

    const authum_path_t *
    authum_env_lang(const authum_env_t *env);

    const authum_path_t *
    authum_env_module_config(const authum_env_t *env);

    const authum_path_t *
    authum_env_module_data(const authum_env_t *env);

    /**
     * `configs/<leaf>` into @p out — the module's own `<module>.cfg`.
     * True when the file exists.
     */
    authum_bool_t
    authum_env_config_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out);

    /**
     * `configs/<module>/<leaf>` into @p out — a module's other config files
     * (`whitelist.ini`). True when the file exists.
     */
    authum_bool_t
    authum_env_module_config_path(const authum_env_t *env, const authum_char_t *leaf,
                                  authum_path_t *out);

    /**
     * `data/<module>/<leaf>` into @p out, the folder created. True when the
     * path could be built (the file itself may not exist yet).
     */
    authum_bool_t
    authum_env_data_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENV_H */
