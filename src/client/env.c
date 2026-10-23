#include <authum/env.h>

#include <stdio.h>
#include <string.h>

/** @p field of @p env when the host's table reaches it, else null. */
#define AUTHUM_ENV_GET(env, field)                                                                 \
    ((!authum_null_eq(env) && AUTHUM_ABI_FITS(env, field)) ? (env)->field : authum_null)

const authum_path_t *
authum_env_game(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, game);
}

const authum_path_t *
authum_env_models(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, models);
}

const authum_path_t *
authum_env_sprites(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, sprites);
}

const authum_path_t *
authum_env_addons(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, addons);
}

const authum_path_t *
authum_env_authum(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, authum);
}

const authum_path_t *
authum_env_modules(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, modules);
}

const authum_path_t *
authum_env_module(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, module);
}

const authum_char_t *
authum_env_module_cstr(const authum_env_t *env)
{
    return authum_path_as_cstr_or_null(authum_env_module(env));
}

const authum_path_t *
authum_env_configs(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, configs);
}

const authum_path_t *
authum_env_data(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, data);
}

const authum_path_t *
authum_env_lang(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, lang);
}

const authum_path_t *
authum_env_module_config(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, module_config);
}

const authum_path_t *
authum_env_module_data(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, module_data);
}

authum_bool_t
authum_env_file_exists(const authum_path_t *path)
{
    FILE *file = fopen(authum_path_as_cstr(path), "rb");

    if (authum_null_eq(file))
    {
        return authum_bool_false;
    }
    fclose(file);
    return authum_bool_true;
}

/**
 * The old per-module folder (`modules/qproto/`), or null for a module that
 * sits right in `modules/` — that folder is shared, nothing of ours is there.
 */
const authum_path_t *
authum_env_legacy_dir(const authum_env_t *env)
{
    const authum_path_t *module = authum_env_module(env);
    const authum_path_t *modules = authum_env_modules(env);

    if (authum_null_eq(module) || authum_path_is_empty(module) ||
        (!authum_null_eq(modules) &&
         strcmp(authum_path_as_cstr(module), authum_path_as_cstr(modules)) == 0))
    {
        return authum_null;
    }
    return module;
}

/** `dir/leaf` into @p out when it exists. */
authum_bool_t
authum_env_try(const authum_path_t *dir, const authum_char_t *leaf, authum_path_t *out)
{
    authum_path_t candidate;

    if (authum_null_eq(dir) || !authum_path_join(&candidate, dir, leaf) ||
        !authum_env_file_exists(&candidate))
    {
        return authum_bool_false;
    }
    return authum_path_assign(out, &candidate);
}

authum_env_found_t
authum_env_config_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out)
{
    if (authum_null_eq(leaf) || leaf[0] == '\0' || authum_null_eq(out))
    {
        return authum_env_found_none;
    }
    authum_path_clear(out);
    if (authum_env_try(authum_env_configs(env), leaf, out) ||
        authum_env_try(authum_env_module_config(env), leaf, out))
    {
        return authum_env_found_here;
    }
    if (authum_env_try(authum_env_legacy_dir(env), leaf, out))
    {
        return authum_env_found_legacy;
    }
    if (!authum_null_eq(authum_env_configs(env)))
    {
        authum_path_join(out, authum_env_configs(env), leaf);
    }
    return authum_env_found_none;
}

authum_env_found_t
authum_env_data_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out)
{
    const authum_path_t *data = authum_env_module_data(env);
    authum_path_t legacy;

    if (authum_null_eq(leaf) || leaf[0] == '\0' || authum_null_eq(out))
    {
        return authum_env_found_none;
    }
    authum_path_clear(out);
    if (authum_null_eq(data))
    {
        /* An older host: the module folder is all there is. */
        return authum_env_try(authum_env_module(env), leaf, out) ? authum_env_found_here
                                                                 : authum_env_found_none;
    }
    authum_path_ensure_dir(authum_env_data(env));
    authum_path_ensure_dir(data);
    if (!authum_path_join(out, data, leaf))
    {
        return authum_env_found_none;
    }
    if (authum_env_file_exists(out))
    {
        return authum_env_found_here;
    }
    /* Move what the old folder had; same disk, so a rename. */
    if (authum_env_try(authum_env_legacy_dir(env), leaf, &legacy) &&
        rename(authum_path_as_cstr(&legacy), authum_path_as_cstr(out)) == 0)
    {
        return authum_env_found_legacy;
    }
    return authum_env_found_none;
}
