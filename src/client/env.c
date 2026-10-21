#include <authum/env.h>

const authum_path_t *
authum_env_game(const authum_env_t *env)
{
    return (!authum_null_eq(env)) ? env->game : authum_null;
}

const authum_path_t *
authum_env_models(const authum_env_t *env)
{
    return (!authum_null_eq(env)) ? env->models : authum_null;
}

const authum_path_t *
authum_env_sprites(const authum_env_t *env)
{
    return (!authum_null_eq(env)) ? env->sprites : authum_null;
}

const authum_path_t *
authum_env_addons(const authum_env_t *env)
{
    return (!authum_null_eq(env)) ? env->addons : authum_null;
}

const authum_path_t *
authum_env_authum(const authum_env_t *env)
{
    return (!authum_null_eq(env)) ? env->authum : authum_null;
}

const authum_path_t *
authum_env_modules(const authum_env_t *env)
{
    return (!authum_null_eq(env)) ? env->modules : authum_null;
}

const authum_path_t *
authum_env_module(const authum_env_t *env)
{
    return (!authum_null_eq(env)) ? env->module : authum_null;
}

const authum_char_t *
authum_env_module_cstr(const authum_env_t *env)
{
    return authum_path_as_cstr_or_null(authum_env_module(env));
}
