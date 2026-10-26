#include <authum/env.h>

#include <stdio.h>

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

const authum_path_t *
authum_env_module_config(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, module_config);
}

const authum_path_t *
authum_env_module_lang(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, module_lang);
}

const authum_path_t *
authum_env_module_data(const authum_env_t *env)
{
    return AUTHUM_ENV_GET(env, module_data);
}

authum_bool_t
authum_env_config_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out)
{
    FILE *file;

    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    authum_path_clear(out);
    if (authum_null_eq(authum_env_module_config(env)) || authum_null_eq(leaf) || leaf[0] == '\0' ||
        !authum_path_join(out, authum_env_module_config(env), leaf))
    {
        return authum_bool_false;
    }
    file = fopen(authum_path_as_cstr(out), "rb");
    if (authum_null_eq(file))
    {
        return authum_bool_false;
    }
    fclose(file);
    return authum_bool_true;
}

authum_bool_t
authum_env_data_path(const authum_env_t *env, const authum_char_t *leaf, authum_path_t *out)
{
    const authum_path_t *data = authum_env_module_data(env);

    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    authum_path_clear(out);
    if (authum_null_eq(data) || authum_null_eq(leaf) || leaf[0] == '\0')
    {
        return authum_bool_false;
    }
    return authum_path_join(out, data, leaf);
}
