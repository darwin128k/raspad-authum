/**
 * @file env.h
 * @brief Where things live: game, Authum, and this module's config and data.
 *
 * Filled by `authum_mm` from its own image path (and the engine game dir as a
 * fallback). Paths are ::authum_path_t values owned by the host, valid until
 * `stop`. Cross-platform: absolute paths on the current volume when the host
 * can resolve them.
 *
 * HLDS layout, the AMX Mod X way — what the admin edits apart from what a
 * module writes:
 * @code
 *   <game>/                        ← env->game
 *     addons/                      ← env->addons
 *       authum/                    ← env->authum (authum_mm.so)
 *         modules/                 ← env->modules: qproto.so, gatekeeper.so, …
 *         configs/                 ← env->configs: authum.cfg, modules.list,
 *           qproto.cfg                 one <module>.cfg per module,
 *           gatekeeper/            ← env->module_config: its other files
 *         data/                    ← env->data
 *           qproto/                ← env->module_data: what the module writes
 *           lang/                  ← env->lang: translations
 *         logs/                    ← the host's (see <authum/logger.h>)
 * @endcode
 *
 * Use ::authum_env_config_path and ::authum_env_data_path (or the
 * `authum_host_*` shortcuts) instead of joining paths by hand: they also find
 * files left in the old `modules/<module>/` folder.
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
     * @brief Snapshot of well-known directories for one loaded module.
     *
     * Each path is null when unknown; otherwise points at a host-owned
     * ::authum_path_t valid until `stop`. Append only (see <authum/abi.h>).
     */
    struct authum_env
    {
        /** sizeof this table as built by the host — see <authum/abi.h>. */
        authum_usize_t size;
        const authum_path_t *game;    /**< Game dir (`cstrike`, …). */
        const authum_path_t *models;  /**< `game/models`. */
        const authum_path_t *sprites; /**< `game/sprites`. */
        const authum_path_t *addons;  /**< `game/addons`. */
        const authum_path_t *authum;  /**< `addons/authum`, where `authum_mm` is. */
        const authum_path_t *modules; /**< `authum/modules`. */
        /**
         * The directory this module's image is in: `modules/` for
         * `modules/qproto.so`, `modules/qproto/` in the old per-module folder.
         */
        const authum_path_t *module;
        const authum_path_t *configs;       /**< `authum/configs`. */
        const authum_path_t *data;          /**< `authum/data`. */
        const authum_path_t *lang;          /**< `authum/data/lang`. */
        const authum_path_t *module_config; /**< `configs/<module>` (may not exist). */
        const authum_path_t *module_data;   /**< `data/<module>` (created at load). */
    };
    typedef struct authum_env authum_env_t;

    /** Where ::authum_env_config_path / ::authum_env_data_path found a file. */
    enum
    {
        /** Not found; the path given back is where it belongs. */
        authum_env_found_none = 0,
        /** In its place under `configs/` or `data/`. */
        authum_env_found_here = 1,
        /** Only in the old `modules/<module>/` folder: that path is given back. */
        authum_env_found_legacy = 2
    };
    typedef authum_u32_t authum_env_found_t;

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
     * Config file @p leaf of this module (`qproto.cfg`, `whitelist.ini`).
     * Looks in `configs/<leaf>`, then `configs/<module>/<leaf>`, then the old
     * `modules/<module>/` folder; @p out gets the first that exists, or
     * `configs/<leaf>` when there is none. Returns `authum_env_found_*`.
     */
    authum_env_found_t
    authum_env_config_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out);

    /**
     * Data file @p leaf of this module: `data/<module>/<leaf>`, folder created.
     * A file still in the old module folder is moved there first (same disk:
     * a rename); then `authum_env_found_legacy` says it happened.
     */
    authum_env_found_t
    authum_env_data_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENV_H */
