#include <authum/host/placeholders.h>
#include <authum/cstr.h>
#include <authum/memory.h>
#include <authum/unused.h>
#include <lh/char/letter.h>
#include <lh/char/digit.h>
#include <lh/memory/std.h>
#include <lh/util/str/ptr.h>


#include <stdlib.h>

#include <lh/os/system/fs/path.h>
#include <lh/os/fs/path.h>

/** The standard names, indexed by ::authum_placeholder_standard_t. */
static const authum_char_t *const
    authum_placeholder_standard_names[authum_placeholder_standard_count] = {
        "game",   "module", "data",    "config", "lang",
        "addons", "authum", "modules", "models", "sprites"};

typedef struct authum_placeholder_entry
{
    struct authum_placeholder_entry *next;
    authum_ptr owner;
    authum_char_t name[authum_placeholder_name_max];
    authum_char_t value[authum_placeholder_value_max];
} authum_placeholder_entry_t;

/** Registered names are host-wide: a module publishes, the others may use. */
static authum_placeholder_entry_t *authum_placeholder_entries;

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
    lh_str_view_t view;
    lh_fs_path_init(lh_addr_of(path));
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

static authum_bool_t
authum_placeholder_absolute(const authum_char_t *path)
{
    lh_fs_path_t parsed;
    lh_str_view_t text;
    lh_fs_path_init(lh_addr_of(parsed));
    lh_str_view_init(lh_addr_of(text), path);
    lh_fs_path_set(lh_addr_of(parsed), text, lh_os_system_fs_path_style_native());
    const authum_bool_t absolute =
        lh_fs_path_is_absolute(lh_addr_of(parsed)) ? authum_bool_true : authum_bool_false;
    lh_fs_path_deinit(lh_addr_of(parsed));
    return absolute;
}

authum_bool_t
authum_placeholder_is_standard(const authum_char_t *name, authum_usize_t length)
{
    for (authum_usize_t index = 0; lh_math_lt(index, authum_placeholder_standard_count);
         index = lh_math_add_one(index))
    {
        if (lh_math_eq(lh_str_ptr_len(authum_placeholder_standard_names[index]), length) &&
            lh_str_ptr_equals_by_size(authum_placeholder_standard_names[index], length, name, length,
                                      lh_bool_false))
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

static authum_bool_t
authum_placeholder_name_valid(const authum_char_t *name)
{
    authum_usize_t length = lh_str_ptr_len(name);
    if (!length || lh_math_ge(length, authum_placeholder_name_max))
    {
        return authum_bool_false;
    }
    for (authum_usize_t index = 0; lh_math_lt(index, length); index = lh_math_add_one(index))
    {
        authum_char_t c = name[index];
        if (!(lh_char_is_letter(c) || lh_char_is_digit(c) || lh_math_eq(c, lh_char_map_underscore)))
        {
            return authum_bool_false;
        }
    }
    return authum_bool_true;
}

/**
 * @brief The text of @p path, or null when it is empty.
 *
 * Reads ::authum_path_t directly: this file is also compiled into hosts that
 * do not link the whole SDK (the FastDL test host).
 */
static const authum_char_t *
authum_placeholder_path_text(const authum_path_t *path)
{
    return (path && path->size) ? path->data : authum_null;
}

/**
 * @brief The value of @p name, or null when nobody published it.
 *
 * Standard names resolve against the calling module; registered names are
 * shared by every module.
 */
static const authum_char_t *
authum_placeholder_lookup(authum_placeholder_owner_t *owner, const authum_char_t *name,
                          authum_usize_t length)
{
    for (authum_usize_t index = 0; lh_math_lt(index, authum_placeholder_standard_count);
         index = lh_math_add_one(index))
    {
        if (lh_math_eq(lh_str_ptr_len(authum_placeholder_standard_names[index]), length) &&
            lh_str_ptr_equals_by_size(authum_placeholder_standard_names[index], length, name, length,
                                      lh_bool_false) &&
            owner->standard[index] && owner->standard[index][0])
        {
            return owner->standard[index];
        }
    }
    for (const authum_placeholder_entry_t *entry = authum_placeholder_entries; entry;
         entry = entry->next)
    {
        if (lh_math_eq(lh_str_ptr_len(entry->name), length) &&
            lh_str_ptr_equals_by_size(entry->name, length, name, length, lh_bool_false))
        {
            return entry->value;
        }
    }
    return authum_null;
}

static authum_bool_t
authum_placeholder_expand_impl(authum_ptr owner_ptr, const authum_char_t *template,
                               authum_char_t *out, authum_usize_t out_size)
{
    authum_placeholder_owner_t *owner = owner_ptr;
    authum_char_t result[authum_placeholder_value_max];
    authum_usize_t used = 0;
    const authum_char_t *template_end;

    if (authum_null_eq(owner) || !template || !lh_ptr_deref(template) || !out || !out_size)
    {
        return authum_bool_false;
    }
    template_end = lh_math_add(template, lh_str_ptr_len(template));
    for (const authum_char_t *cursor = template; lh_ptr_deref(cursor);)
    {
        if (lh_math_eq(lh_ptr_deref(cursor), lh_char_map_lbrace))
        {
            const authum_char_t *end = lh_str_ptr_find_of_char(
                lh_math_add(cursor, 1),
                lh_cast_static(lh_usize_t, lh_math_sub(template_end, lh_math_add(cursor, 1))),
                lh_char_map_rbrace);
            authum_usize_t length;
            const authum_char_t *value;
            if (!end)
            {
                return authum_bool_false;
            }
            length = (authum_usize_t)(lh_math_sub(lh_math_sub(end, cursor), 1));
            value = authum_placeholder_lookup(owner, lh_math_add(cursor, 1), length);
            if (authum_null_eq(value))
            {
                return authum_bool_false;
            }
            authum_usize_t value_length = lh_str_ptr_len(value);
            if (lh_math_gt(lh_math_add(lh_math_add(used, value_length), 1), sizeof result))
            {
                return authum_bool_false;
            }
            lh_memory_std_copy(lh_math_add(result, used), value, value_length);
            used = lh_math_add(used, value_length);
            cursor = lh_math_add(end, 1);
            continue;
        }
        if (lh_math_eq(lh_ptr_deref(cursor), lh_char_map_rbrace))
        {
            return authum_bool_false;
        }
        if (lh_math_gt(lh_math_add(used, 2), sizeof result))
        {
            return authum_bool_false;
        }
        result[used++] = lh_ptr_deref(cursor++);
    }
    result[used] = 0;
    if (!used)
    {
        return authum_bool_false;
    }
    /* A relative result belongs to the module that asked for it. */
    if (!authum_placeholder_absolute(result))
    {
        const authum_char_t *base = owner->standard[authum_placeholder_module];
        authum_usize_t base_length = base ? lh_str_ptr_len(base) : 0;
        if (!base_length ||
            lh_math_gt(lh_math_add(lh_math_add(used, base_length), 2), sizeof result))
        {
            return authum_bool_false;
        }
        lh_memory_std_move(lh_math_add(lh_math_add(result, base_length), 1), result,
                           lh_math_add(used, 1));
        lh_memory_std_copy(result, base, base_length);
        result[base_length] = lh_char_map_slash;
        used = lh_math_add(used, lh_math_add(base_length, 1));
    }
    if (lh_math_gt(lh_math_add(used, 1), out_size))
    {
        return authum_bool_false;
    }
    lh_memory_std_copy(out, result, lh_math_add(used, 1));
    return authum_bool_true;
}

static authum_bool_t
authum_placeholder_register_impl(authum_ptr owner, const authum_char_t *name,
                                 const authum_char_t *value)
{
    authum_placeholder_entry_t *entry;

    if (!name || !value || !lh_ptr_deref(value) || !authum_placeholder_name_valid(name))
    {
        return authum_bool_false;
    }
    if (authum_placeholder_is_standard(name, lh_str_ptr_len(name)))
    {
        return authum_bool_false;
    }
    if (!authum_placeholder_absolute(value))
    {
        return authum_bool_false;
    }
    for (const authum_placeholder_entry_t *existing = authum_placeholder_entries; existing;
         existing = existing->next)
    {
        if (!lh_str_ptr_equals(existing->name, name, lh_bool_false))
        {
            return authum_bool_false;
        }
    }
    entry = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(entry));
    if (!entry)
    {
        return authum_bool_false;
    }
    entry->owner = owner;
    authum_unused(authum_cstr_copy(entry->name, sizeof entry->name, name));
    if (!authum_placeholder_canonical(value, entry->value, sizeof entry->value))
    {
        authum_memory_free(entry);
        return authum_bool_false;
    }
    entry->next = authum_placeholder_entries;
    authum_placeholder_entries = entry;
    return authum_bool_true;
}

static authum_bool_t
authum_placeholder_unregister_impl(authum_ptr owner, const authum_char_t *name)
{
    authum_placeholder_entry_t **link = lh_addr_of(authum_placeholder_entries);

    if (!name)
    {
        return authum_bool_false;
    }
    while (lh_ptr_deref(link))
    {
        authum_placeholder_entry_t *entry = lh_ptr_deref(link);
        if (lh_math_eq(entry->owner, owner) && !lh_str_ptr_equals(entry->name, name, lh_bool_false))
        {
            lh_ptr_deref(link) = entry->next;
            authum_memory_free(entry);
            return authum_bool_true;
        }
        link = lh_addr_of(entry->next);
    }
    return authum_bool_false;
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

void
authum_placeholder_host_drop_owner(authum_placeholder_owner_t *owner)
{
    authum_placeholder_entry_t **link = lh_addr_of(authum_placeholder_entries);

    if (authum_null_eq(owner))
    {
        return;
    }
    while (lh_ptr_deref(link))
    {
        authum_placeholder_entry_t *entry = lh_ptr_deref(link);
        if (lh_math_eq(entry->owner, owner))
        {
            lh_ptr_deref(link) = entry->next;
            authum_memory_free(entry);
            continue;
        }
        link = lh_addr_of(entry->next);
    }
}
