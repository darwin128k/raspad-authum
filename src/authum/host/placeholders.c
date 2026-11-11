#include <authum/host/placeholders.h>
#include <authum/memory.h>
#include <authum/unused.h>

/** The standard names, indexed by ::authum_placeholder_standard_t. */
static const authum_char_t *const
    authum_placeholder_standard_names[authum_placeholder_standard_count] = {
        "game",   "module", "data",    "config", "lang",
        "addons", "authum", "modules", "models", "sprites"};

/** Registered names are host-wide: a module publishes, the others may use. */
lh_str_template_registry_t authum_placeholder_registry;
authum_bool_t authum_placeholder_registry_ready;
const authum_char_t *
authum_placeholder_standard_name(authum_placeholder_standard_t standard)
{
    if (lh_math_ge(lh_cast_static(authum_u32_t, standard),
                   lh_cast_static(authum_u32_t, authum_placeholder_standard_count)))
    {
        return authum_null;
    }
    return authum_placeholder_standard_names[standard];
}

authum_bool_t
authum_placeholder_canonical(const authum_char_t *input, authum_char_t *out, authum_usize_t size)
{
    if (authum_null_eq(input) || authum_null_eq(out) || !size)
    {
        return authum_bool_false;
    }
    lh_fs_path_t path;
    lh_fs_path_init(lh_addr_of(path));
    lh_str_view_t view;
    lh_str_view_init(lh_addr_of(view), input);
    lh_fs_path_set(lh_addr_of(path), view, lh_os_system_fs_path_style_native());
    lh_str_t rendered;
    lh_str_init(lh_addr_of(rendered));
    const authum_char_t *text = input;
    if (lh_os_fs_path_canonical(lh_addr_of(path), lh_addr_of(path)))
    {
        lh_fs_path_to_str(lh_addr_of(path), lh_os_system_fs_path_style_native(),
                          lh_addr_of(rendered));
        text = lh_str_get_data(lh_addr_of(rendered));
    }
    const authum_u64_t length = lh_str_ptr_len(text);
    const authum_bool_t fits = lh_math_lt(length, size) ? authum_bool_true : authum_bool_false;
    if (fits)
    {
        /* Authum keeps unresolved input verbatim, including new data directories. */
        lh_memory_std_move(out, text, lh_math_add_one(lh_cast_static(authum_usize_t, length)));
    }
    lh_str_deinit(lh_addr_of(rendered));
    lh_fs_path_deinit(lh_addr_of(path));
    return fits;
}

lh_str_template_registry_t *
authum_placeholder_get_registry(void)
{
    if (!authum_placeholder_registry_ready)
    {
        lh_str_template_registry_init(lh_addr_of(authum_placeholder_registry));
        authum_placeholder_registry_ready = authum_bool_true;
    }
    return lh_addr_of(authum_placeholder_registry);
}

authum_usize_t
authum_placeholder_find_standard(const lh_str_view_t *name)
{
    for (authum_usize_t index = 0u; lh_math_lt(index, authum_placeholder_standard_count); ++index)
    {
        lh_str_view_t standard;

        lh_str_view_init(lh_addr_of(standard), authum_placeholder_standard_names[index]);
        if (lh_str_view_equals(lh_addr_of(standard), name, lh_bool_false))
        {
            return index;
        }
    }
    return authum_placeholder_standard_count;
}

authum_bool_t
authum_placeholder_name_valid(const authum_char_t *name)
{
    lh_str_view_t view;

    lh_str_view_init(lh_addr_of(view), name);
    return lh_math_lt(lh_str_view_get_size(lh_addr_of(view)), authum_placeholder_name_max) &&
                   lh_str_template_is_name(lh_addr_of(view))
               ? authum_bool_true
               : authum_bool_false;
}

/**
 * @brief The text of @p path, or null when it is empty.
 *
 * Reads ::authum_path_t directly: this file is also compiled into hosts that
 * do not link the whole SDK (the FastDL test host).
 */
const authum_char_t *
authum_placeholder_path_text(const authum_path_t *path)
{
    return (path && path->size) ? path->data : authum_null;
}

authum_bool_t
authum_placeholder_is_standard(const lh_str_view_t *name)
{
    return lh_math_lt(authum_placeholder_find_standard(name), authum_placeholder_standard_count)
               ? authum_bool_true
               : authum_bool_false;
}

lh_bool_t
authum_placeholder_lookup(lh_ptr user, const lh_str_view_t *name, lh_str_view_t *value)
{
    const authum_placeholder_owner_t *owner = user;
    const authum_usize_t index = authum_placeholder_find_standard(name);

    if (lh_math_lt(index, authum_placeholder_standard_count))
    {
        /* A standard name the host does not know for this module has no value. */
        if (authum_null_eq(owner->standard[index]) ||
            lh_math_eq(owner->standard[index][0], lh_char_map_nul))
        {
            return lh_bool_false;
        }
        lh_str_view_init(value, owner->standard[index]);
        return lh_bool_true;
    }
    return lh_str_template_registry_try_get_value(authum_placeholder_get_registry(), name, value);
}

authum_bool_t
authum_placeholder_unregister_impl(authum_ptr owner, const authum_char_t *name)
{
    if (authum_null_eq(name))
    {
        return authum_bool_false;
    }

    lh_str_view_t name_view;

    lh_str_view_init(lh_addr_of(name_view), name);
    return lh_str_template_registry_try_remove(authum_placeholder_get_registry(), owner,
                                               lh_addr_of(name_view))
               ? authum_bool_true
               : authum_bool_false;
}

static const authum_placeholder_ops_t authum_placeholder_ops = {
    .expand = authum_placeholder_expand_impl,
    .register_placeholder = authum_placeholder_register_impl,
    .unregister_placeholder = authum_placeholder_unregister_impl,
};

void
authum_placeholder_host_init(authum_placeholder_owner_t *owner,
                             const authum_char_t *const standard[authum_placeholder_standard_count],
                             authum_placeholder_service_t *service)
{
    if (!owner || !service)
    {
        return;
    }
    lh_memory_std_set(owner, 0, sizeof lh_ptr_deref(owner));
    if (standard)
    {
        for (authum_usize_t index = 0; lh_math_lt(index, authum_placeholder_standard_count);
             index = lh_math_add_one(index))
        {
            if (!standard[index] || !standard[index][0])
            {
                continue;
            }
            /* Canonicalize: a source root is opened without following a
             * symlink at its end, so the published path must be resolved. */
            if (!authum_placeholder_canonical(standard[index], owner->storage[index],
                                              sizeof owner->storage[index]))
            {
                continue;
            }
            owner->standard[index] = owner->storage[index];
        }
    }
    service->ops = lh_addr_of(authum_placeholder_ops);
    service->owner = owner;
}

void
authum_placeholder_host_drop_owner(authum_placeholder_owner_t *owner)
{
    if (!authum_null_eq(owner))
    {
        lh_str_template_registry_remove_owner(authum_placeholder_get_registry(), owner);
    }
}

authum_bool_t
authum_placeholder_expand_impl(authum_ptr owner_ptr, const authum_char_t *pattern,
                               authum_char_t *out, authum_usize_t out_size)
{
    authum_placeholder_owner_t *owner = owner_ptr;

    if (authum_null_eq(owner) || authum_null_eq(pattern) || authum_null_eq(out) ||
        lh_math_eq(out_size, 0u))
    {
        return authum_bool_false;
    }

    lh_str_view_t view;

    lh_str_view_init(lh_addr_of(view), pattern);

    lh_str_t result;

    lh_str_init(lh_addr_of(result));

    authum_bool_t ok = lh_str_template_try_expand(lh_addr_of(view), authum_placeholder_lookup,
                                                  owner, lh_addr_of(result)) &&
                               !lh_str_is_empty(lh_addr_of(result))
                           ? authum_bool_true
                           : authum_bool_false;

    /* A relative result belongs to the module that asked for it. */
    if (ok && !lh_fs_path_is_text_absolute(lh_str_get_data(lh_addr_of(result)),
                                           lh_os_system_fs_path_style_native()))
    {
        const authum_char_t *base = owner->standard[authum_placeholder_module];

        if (authum_null_eq(base) || lh_math_eq(base[0], lh_char_map_nul))
        {
            ok = authum_bool_false;
        }
        else
        {
            lh_str_view_t base_view;
            lh_str_t absolute;

            lh_str_view_init(lh_addr_of(base_view), base);
            lh_str_init_by_view(lh_addr_of(absolute), base_view);
            lh_str_push_back(lh_addr_of(absolute), lh_char_map_slash);
            lh_str_append_str(lh_addr_of(absolute), lh_addr_of(result));
            lh_str_assign(lh_addr_of(result), lh_addr_of(absolute));
            lh_str_deinit(lh_addr_of(absolute));
        }
    }

    const authum_usize_t size = lh_math_add_one(lh_str_get_size(lh_addr_of(result)));

    if (ok && lh_math_le(size, authum_placeholder_value_max) && lh_math_le(size, out_size))
    {
        lh_memory_std_copy(out, lh_str_get_data(lh_addr_of(result)), size);
    }
    else
    {
        ok = authum_bool_false;
    }
    lh_str_deinit(lh_addr_of(result));
    return ok;
}

authum_bool_t
authum_placeholder_register_impl(authum_ptr owner, const authum_char_t *name,
                                 const authum_char_t *value)
{
    if (authum_null_eq(name) || authum_null_eq(value) || lh_math_eq(value[0], lh_char_map_nul) ||
        !authum_placeholder_name_valid(name))
    {
        return authum_bool_false;
    }
    lh_str_view_t text;
    lh_str_view_init(lh_addr_of(text), name);
    authum_char_t canonical[authum_placeholder_value_max];
    if (authum_placeholder_is_standard(lh_addr_of(text)) ||
        !lh_fs_path_is_text_absolute(value, lh_os_system_fs_path_style_native()) ||
        !authum_placeholder_canonical(value, canonical, sizeof canonical))
    {
        return authum_bool_false;
    }

    lh_str_view_t name_view;

    lh_str_view_init(lh_addr_of(name_view), name);

    lh_str_view_t value_view;

    lh_str_view_init(lh_addr_of(value_view), canonical);
    return lh_str_template_registry_try_add(authum_placeholder_get_registry(), owner,
                                            lh_addr_of(name_view), lh_addr_of(value_view))
               ? authum_bool_true
               : authum_bool_false;
}

void
authum_placeholder_host_init_env(authum_placeholder_owner_t *owner, const authum_env_t *env,
                                 authum_placeholder_service_t *service)
{
    const authum_char_t *standard[authum_placeholder_standard_count] = {0};

    if (!owner || !service)
    {
        return;
    }
    if (env)
    {
        const authum_path_t *paths[authum_placeholder_standard_count] = {
            env->game,   env->module, env->module_data, env->module_config, env->module_lang,
            env->addons, env->authum, env->modules,     env->models,        env->sprites};
        for (authum_usize_t index = 0; lh_math_lt(index, authum_placeholder_standard_count);
             index = lh_math_add_one(index))
        {
            standard[index] = authum_placeholder_path_text(paths[index]);
        }
    }
    authum_placeholder_host_init(owner, standard, service);
}
