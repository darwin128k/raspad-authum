#define _GNU_SOURCE
#include <authum/host/placeholders.h>

#include <authum/null.h>

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#    include <direct.h>
#else
#    include <limits.h>
#endif

#ifndef AUTHUM_PLACEHOLDER_PATH_MAX
#    if defined(_WIN32)
#        define AUTHUM_PLACEHOLDER_PATH_MAX 4096
#    else
#        define AUTHUM_PLACEHOLDER_PATH_MAX PATH_MAX
#    endif
#endif

/** The standard names, indexed by ::authum_placeholder_standard_t. */
static const char *const authum_placeholder_standard_names[authum_placeholder_standard_count] = {
    "game", "module", "data", "config", "lang", "addons", "authum", "modules", "models", "sprites"};

typedef struct authum_placeholder_entry
{
    struct authum_placeholder_entry *next;
    authum_ptr owner;
    authum_char_t name[authum_placeholder_name_max];
    authum_char_t value[authum_placeholder_value_max];
} authum_placeholder_entry_t;

/** Registered names are host-wide: a module publishes, the others may use. */
static authum_placeholder_entry_t *authum_placeholder_entries;

const char *
authum_placeholder_standard_name(authum_placeholder_standard_t standard)
{
    if ((unsigned)standard >= (unsigned)authum_placeholder_standard_count)
    {
        return authum_null;
    }
    return authum_placeholder_standard_names[standard];
}

authum_bool_t
authum_placeholder_canonical(const authum_char_t *input, authum_char_t *out, authum_usize_t size)
{
    authum_char_t resolved[AUTHUM_PLACEHOLDER_PATH_MAX];

    if (!input || !out || !size)
    {
        return authum_bool_false;
    }
#if defined(_WIN32)
    if (!_fullpath(resolved, input, (int)sizeof resolved))
#else
    if (!realpath(input, resolved))
#endif
    {
        /* Keep the path as written: the caller decides whether it may exist. */
        if (strlen(input) + 1 > sizeof resolved)
        {
            return authum_bool_false;
        }
        strcpy(resolved, input);
    }
    if (strlen(resolved) + 1 > size)
    {
        return authum_bool_false;
    }
    strcpy(out, resolved);
    return authum_bool_true;
}

static authum_bool_t
authum_placeholder_absolute(const authum_char_t *path)
{
    if (path[0] == '/')
    {
        return authum_bool_true;
    }
#if defined(_WIN32)
    if (path[0] && path[1] == ':')
    {
        return authum_bool_true;
    }
#endif
    return authum_bool_false;
}

authum_bool_t
authum_placeholder_is_standard(const authum_char_t *name, authum_usize_t length)
{
    for (authum_usize_t index = 0; index < authum_placeholder_standard_count; ++index)
    {
        if (strlen(authum_placeholder_standard_names[index]) == length &&
            !strncmp(authum_placeholder_standard_names[index], name, length))
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

static authum_bool_t
authum_placeholder_name_valid(const authum_char_t *name)
{
    authum_usize_t length = strlen(name);
    if (!length || length >= authum_placeholder_name_max)
    {
        return authum_bool_false;
    }
    for (authum_usize_t index = 0; index < length; ++index)
    {
        authum_char_t c = name[index];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_'))
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
static const char *
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
    for (authum_usize_t index = 0; index < authum_placeholder_standard_count; ++index)
    {
        if (strlen(authum_placeholder_standard_names[index]) == length &&
            !strncmp(authum_placeholder_standard_names[index], name, length) &&
            owner->standard[index] && owner->standard[index][0])
        {
            return owner->standard[index];
        }
    }
    for (const authum_placeholder_entry_t *entry = authum_placeholder_entries; entry;
         entry = entry->next)
    {
        if (strlen(entry->name) == length && !strncmp(entry->name, name, length))
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

    if (authum_null_eq(owner) || !template || !*template || !out || !out_size)
    {
        return authum_bool_false;
    }
    for (const authum_char_t *cursor = template; *cursor;)
    {
        if (*cursor == '{')
        {
            const authum_char_t *end = strchr(cursor + 1, '}');
            authum_usize_t length;
            const authum_char_t *value;
            if (!end)
            {
                return authum_bool_false;
            }
            length = (authum_usize_t)(end - cursor - 1);
            value = authum_placeholder_lookup(owner, cursor + 1, length);
            if (authum_null_eq(value))
            {
                return authum_bool_false;
            }
            authum_usize_t value_length = strlen(value);
            if (used + value_length + 1 > sizeof result)
            {
                return authum_bool_false;
            }
            memcpy(result + used, value, value_length);
            used += value_length;
            cursor = end + 1;
            continue;
        }
        if (*cursor == '}')
        {
            return authum_bool_false;
        }
        if (used + 2 > sizeof result)
        {
            return authum_bool_false;
        }
        result[used++] = *cursor++;
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
        authum_usize_t base_length = base ? strlen(base) : 0;
        if (!base_length || used + base_length + 2 > sizeof result)
        {
            return authum_bool_false;
        }
        memmove(result + base_length + 1, result, used + 1);
        memcpy(result, base, base_length);
        result[base_length] = '/';
        used += base_length + 1;
    }
    if (used + 1 > out_size)
    {
        return authum_bool_false;
    }
    memcpy(out, result, used + 1);
    return authum_bool_true;
}

static authum_bool_t
authum_placeholder_register_impl(authum_ptr owner, const authum_char_t *name,
                                 const authum_char_t *value)
{
    authum_placeholder_entry_t *entry;

    if (!name || !value || !*value || !authum_placeholder_name_valid(name))
    {
        return authum_bool_false;
    }
    if (authum_placeholder_is_standard(name, strlen(name)))
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
        if (!strcmp(existing->name, name))
        {
            return authum_bool_false;
        }
    }
    entry = calloc(1, sizeof *entry);
    if (!entry)
    {
        return authum_bool_false;
    }
    entry->owner = owner;
    strncpy(entry->name, name, sizeof entry->name - 1);
    if (!authum_placeholder_canonical(value, entry->value, sizeof entry->value))
    {
        free(entry);
        return authum_bool_false;
    }
    entry->next = authum_placeholder_entries;
    authum_placeholder_entries = entry;
    return authum_bool_true;
}

static authum_bool_t
authum_placeholder_unregister_impl(authum_ptr owner, const authum_char_t *name)
{
    authum_placeholder_entry_t **link = &authum_placeholder_entries;

    if (!name)
    {
        return authum_bool_false;
    }
    while (*link)
    {
        authum_placeholder_entry_t *entry = *link;
        if (entry->owner == owner && !strcmp(entry->name, name))
        {
            *link = entry->next;
            free(entry);
            return authum_bool_true;
        }
        link = &entry->next;
    }
    return authum_bool_false;
}

static const authum_placeholder_ops_t authum_placeholder_ops = {
    sizeof authum_placeholder_ops, authum_placeholder_expand_impl, authum_placeholder_register_impl,
    authum_placeholder_unregister_impl};

void
authum_placeholder_host_init(authum_placeholder_owner_t *owner,
                             const authum_char_t *const standard[authum_placeholder_standard_count],
                             authum_placeholder_service_t *service)
{
    if (!owner || !service)
    {
        return;
    }
    memset(owner, 0, sizeof *owner);
    if (standard)
    {
        for (authum_usize_t index = 0; index < authum_placeholder_standard_count; ++index)
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
    service->ops = &authum_placeholder_ops;
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
        for (authum_usize_t index = 0; index < authum_placeholder_standard_count; ++index)
        {
            standard[index] = authum_placeholder_path_text(paths[index]);
        }
    }
    authum_placeholder_host_init(owner, standard, service);
}

void
authum_placeholder_host_drop_owner(authum_placeholder_owner_t *owner)
{
    authum_placeholder_entry_t **link = &authum_placeholder_entries;

    if (authum_null_eq(owner))
    {
        return;
    }
    while (*link)
    {
        authum_placeholder_entry_t *entry = *link;
        if (entry->owner == owner)
        {
            *link = entry->next;
            free(entry);
            continue;
        }
        link = &entry->next;
    }
}
