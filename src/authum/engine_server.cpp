/**
 * @file engine_server.cpp
 * @brief ::authum_engine_t ops — thin wrappers over host `g_engfuncs`.
 */

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/engine.h>
#include <authum/engine/host.h>
#include <authum/null.h>
#include <authum/size.h>

#include <extdll.h>
#include <meta_api.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;
extern plugin_info_t Plugin_info;
extern mutil_funcs_t *gpMetaUtilFuncs;

namespace
{

enum
{
    authum_engine_slots = 33, /* 1 … 32, the GoldSrc player limit */
    authum_engine_address_max = 64
};

char authum_engine_addresses[authum_engine_slots][authum_engine_address_max];

void
authum_engine_copy(authum_char_t *buf, authum_usize_t buf_size, const char *src)
{
    std::snprintf(buf, static_cast<size_t>(buf_size), "%s", src != nullptr ? src : "");
}

/** Engine string table entry (HLSDK `STRING`, without util.h). */
const char *
authum_engine_string(string_t offset)
{
    return gpGlobals != nullptr ? gpGlobals->pStringBase + offset : "";
}

/** Edict of a connected player in slot @p index, or null. */
edict_t *
authum_engine_client_edict(authum_i32_t index)
{
    edict_t *entity;

    if (gpGlobals == nullptr || index < 1 || index > gpGlobals->maxClients ||
        g_engfuncs.pfnPEntityOfEntIndex == nullptr || g_engfuncs.pfnGetPlayerUserId == nullptr)
    {
        return nullptr;
    }
    entity = g_engfuncs.pfnPEntityOfEntIndex(index);
    if (entity == nullptr || entity->free || g_engfuncs.pfnGetPlayerUserId(entity) <= 0)
    {
        return nullptr;
    }
    return entity;
}

/* --- server --------------------------------------------------------------- */

void
authum_engine_get_game_dir_impl(authum_char_t *buf, authum_usize_t buf_size)
{
    char tmp[256];

    tmp[0] = '\0';
    if (g_engfuncs.pfnGetGameDir != nullptr)
    {
        g_engfuncs.pfnGetGameDir(tmp);
    }
    authum_engine_copy(buf, buf_size, tmp);
}

void
authum_engine_server_print_impl(const authum_char_t *text)
{
    if (g_engfuncs.pfnServerPrint != nullptr)
    {
        g_engfuncs.pfnServerPrint(text);
    }
}

void
authum_engine_server_command_impl(const authum_char_t *cmd)
{
    if (g_engfuncs.pfnServerCommand != nullptr)
    {
        /* HLSDK typedef takes char*, not const char*. */
        g_engfuncs.pfnServerCommand(const_cast<char *>(cmd));
    }
}

authum_i32_t
authum_engine_max_clients_impl(void)
{
    return gpGlobals != nullptr ? gpGlobals->maxClients : 0;
}

authum_float_t
authum_engine_game_time_impl(void)
{
    return gpGlobals != nullptr ? gpGlobals->time : 0.0f;
}

void
authum_engine_get_map_name_impl(authum_char_t *buf, authum_usize_t buf_size)
{
    authum_engine_copy(buf, buf_size,
                       authum_engine_string(gpGlobals != nullptr ? gpGlobals->mapname : 0));
}

authum_bool_t
authum_engine_cvar_get_impl(const authum_char_t *name, authum_char_t *buf, authum_usize_t buf_size)
{
    cvar_t *cvar;

    if (g_engfuncs.pfnCVarGetPointer == nullptr)
    {
        return authum_bool_false;
    }
    cvar = g_engfuncs.pfnCVarGetPointer(name);
    if (cvar == nullptr)
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, cvar->string);
    return authum_bool_true;
}

authum_bool_t
authum_engine_cvar_set_impl(const authum_char_t *name, const authum_char_t *value)
{
    if (g_engfuncs.pfnCVarGetPointer == nullptr || g_engfuncs.pfnCVarSetString == nullptr ||
        g_engfuncs.pfnCVarGetPointer(name) == nullptr)
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnCVarSetString(name, value);
    return authum_bool_true;
}

/* --- players -------------------------------------------------------------- */

authum_bool_t
authum_engine_client_is_connected_impl(authum_i32_t index)
{
    return authum_engine_client_edict(index) != nullptr ? authum_bool_true : authum_bool_false;
}

authum_i32_t
authum_engine_client_userid_impl(authum_i32_t index)
{
    edict_t *entity = authum_engine_client_edict(index);

    return entity != nullptr ? g_engfuncs.pfnGetPlayerUserId(entity) : 0;
}

authum_i32_t
authum_engine_client_index_by_userid_impl(authum_i32_t userid)
{
    authum_i32_t index;

    if (userid <= 0 || gpGlobals == nullptr)
    {
        return 0;
    }
    for (index = 1; index <= gpGlobals->maxClients; ++index)
    {
        if (authum_engine_client_userid_impl(index) == userid)
        {
            return index;
        }
    }
    return 0;
}

authum_bool_t
authum_engine_client_get_name_impl(authum_i32_t index, authum_char_t *buf, authum_usize_t buf_size)
{
    edict_t *entity = authum_engine_client_edict(index);

    if (entity == nullptr)
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, authum_engine_string(entity->v.netname));
    return authum_bool_true;
}

authum_bool_t
authum_engine_client_get_auth_id_impl(authum_i32_t index, authum_char_t *buf,
                                      authum_usize_t buf_size)
{
    edict_t *entity = authum_engine_client_edict(index);

    if (entity == nullptr || g_engfuncs.pfnGetPlayerAuthId == nullptr)
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, g_engfuncs.pfnGetPlayerAuthId(entity));
    return authum_bool_true;
}

authum_bool_t
authum_engine_client_get_address_impl(authum_i32_t index, authum_char_t *buf,
                                      authum_usize_t buf_size)
{
    if (authum_engine_client_edict(index) == nullptr || index >= authum_engine_slots)
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, authum_engine_addresses[index]);
    return authum_engine_addresses[index][0] != '\0' ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_engine_client_get_info_impl(authum_i32_t index, const authum_char_t *key, authum_char_t *buf,
                                   authum_usize_t buf_size)
{
    edict_t *entity = authum_engine_client_edict(index);
    char *info;

    if (entity == nullptr || g_engfuncs.pfnGetInfoKeyBuffer == nullptr ||
        g_engfuncs.pfnInfoKeyValue == nullptr)
    {
        return authum_bool_false;
    }
    info = g_engfuncs.pfnGetInfoKeyBuffer(entity);
    if (info == nullptr)
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, g_engfuncs.pfnInfoKeyValue(info, const_cast<char *>(key)));
    return authum_bool_true;
}

authum_bool_t
authum_engine_client_print_impl(authum_i32_t index, authum_i32_t kind, const authum_char_t *text)
{
    edict_t *entity;
    authum_bool_t sent = authum_bool_false;
    authum_i32_t i;

    if (g_engfuncs.pfnClientPrintf == nullptr || gpGlobals == nullptr)
    {
        return authum_bool_false;
    }
    /* index 0: everyone connected. */
    for (i = (index == 0 ? 1 : index); i <= (index == 0 ? gpGlobals->maxClients : index); ++i)
    {
        entity = authum_engine_client_edict(i);
        if (entity != nullptr)
        {
            g_engfuncs.pfnClientPrintf(entity, static_cast<PRINT_TYPE>(kind), text);
            sent = authum_bool_true;
        }
    }
    return sent;
}

authum_bool_t
authum_engine_client_command_impl(authum_i32_t index, const authum_char_t *cmd)
{
    edict_t *entity = authum_engine_client_edict(index);

    if (entity == nullptr || g_engfuncs.pfnClientCommand == nullptr)
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnClientCommand(entity, const_cast<char *>("%s\n"), cmd);
    return authum_bool_true;
}

authum_bool_t
authum_engine_query_client_cvar_impl(authum_i32_t index, const authum_char_t *cvar,
                                     authum_i32_t request_id)
{
    edict_t *entity = authum_engine_client_edict(index);

    if (entity == nullptr || g_engfuncs.pfnQueryClientCvarValue2 == nullptr)
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnQueryClientCvarValue2(entity, cvar, request_id);
    return authum_bool_true;
}

authum_bool_t
authum_engine_client_get_stats_impl(authum_i32_t index, authum_i32_t *ping, authum_i32_t *loss)
{
    edict_t *entity = authum_engine_client_edict(index);
    int p = 0;
    int l = 0;

    if (entity == nullptr || g_engfuncs.pfnGetPlayerStats == nullptr)
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnGetPlayerStats(entity, &p, &l);
    *ping = p;
    *loss = l;
    return authum_bool_true;
}

authum_bool_t
authum_engine_client_set_info_impl(authum_i32_t index, const authum_char_t *key,
                                   const authum_char_t *value)
{
    edict_t *entity = authum_engine_client_edict(index);
    char *info;

    if (entity == nullptr || g_engfuncs.pfnGetInfoKeyBuffer == nullptr ||
        g_engfuncs.pfnSetClientKeyValue == nullptr)
    {
        return authum_bool_false;
    }
    info = g_engfuncs.pfnGetInfoKeyBuffer(entity);
    if (info == nullptr)
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnSetClientKeyValue(index, info, const_cast<char *>(key),
                                    const_cast<char *>(value));
    return authum_bool_true;
}

/* --- cvars and messages ---------------------------------------------------- */

char *
authum_engine_strdup(const char *text)
{
    std::size_t n = std::strlen(text) + 1u;
    char *copy = static_cast<char *>(std::malloc(n));

    if (copy != nullptr)
    {
        std::memcpy(copy, text, n);
    }
    return copy;
}

authum_bool_t
authum_engine_cvar_register_impl(const authum_char_t *name, const authum_char_t *value,
                                 authum_i32_t flags)
{
    cvar_t *cvar;

    if (g_engfuncs.pfnCVarGetPointer == nullptr || g_engfuncs.pfnCVarRegister == nullptr)
    {
        return authum_bool_false;
    }
    if (g_engfuncs.pfnCVarGetPointer(name) != nullptr)
    {
        return authum_bool_true;
    }
    /* The engine keeps the pointer for good: this memory is never freed. */
    cvar = static_cast<cvar_t *>(std::calloc(1u, sizeof(cvar_t)));
    if (cvar == nullptr)
    {
        return authum_bool_false;
    }
    cvar->name = authum_engine_strdup(name);
    cvar->string = authum_engine_strdup(value);
    cvar->flags = flags;
    if (cvar->name == nullptr || cvar->string == nullptr)
    {
        std::free(cvar->name);
        std::free(cvar->string);
        std::free(cvar);
        return authum_bool_false;
    }
    g_engfuncs.pfnCVarRegister(cvar);
    return g_engfuncs.pfnCVarGetPointer(name) != nullptr ? authum_bool_true : authum_bool_false;
}

authum_i32_t
authum_engine_message_id_impl(const authum_char_t *name)
{
    if (gpMetaUtilFuncs == nullptr || gpMetaUtilFuncs->pfnGetUserMsgID == nullptr)
    {
        return 0;
    }
    return gpMetaUtilFuncs->pfnGetUserMsgID(PLID, name, nullptr);
}

authum_bool_t
authum_engine_message_begin_impl(authum_i32_t dest, authum_i32_t msg_id, authum_i32_t index)
{
    edict_t *target = nullptr;

    if (g_engfuncs.pfnMessageBegin == nullptr)
    {
        return authum_bool_false;
    }
    if (dest == authum_message_dest_one || dest == authum_message_dest_one_unreliable)
    {
        target = authum_engine_client_edict(index);
        if (target == nullptr)
        {
            return authum_bool_false;
        }
    }
    g_engfuncs.pfnMessageBegin(dest, msg_id, nullptr, target);
    return authum_bool_true;
}

void
authum_engine_message_write_impl(authum_i32_t kind, authum_i32_t value, authum_float_t real,
                                 const authum_char_t *text)
{
    switch (kind)
    {
    case authum_message_arg_byte:
        g_engfuncs.pfnWriteByte(value);
        break;
    case authum_message_arg_char:
        g_engfuncs.pfnWriteChar(value);
        break;
    case authum_message_arg_short:
        g_engfuncs.pfnWriteShort(value);
        break;
    case authum_message_arg_long:
        g_engfuncs.pfnWriteLong(value);
        break;
    case authum_message_arg_angle:
        g_engfuncs.pfnWriteAngle(real);
        break;
    case authum_message_arg_coord:
        g_engfuncs.pfnWriteCoord(real);
        break;
    case authum_message_arg_string:
        g_engfuncs.pfnWriteString(text != nullptr ? text : "");
        break;
    case authum_message_arg_entity:
        g_engfuncs.pfnWriteEntity(value);
        break;
    default:
        break;
    }
}

void
authum_engine_message_end_impl(void)
{
    g_engfuncs.pfnMessageEnd();
}

/* --- precache --------------------------------------------------------------- */

/* Between worldspawn's Spawn and ServerActivate; any other time the engine
 * stops the server with a Host_Error. */
bool authum_engine_precache_open = false;

/* The engine keeps the pointer for the whole map: hand it an engine string. */
char *
authum_engine_persistent(const authum_char_t *path)
{
    return const_cast<char *>(authum_engine_string(g_engfuncs.pfnAllocString(path)));
}

authum_i32_t
authum_engine_precache_model_impl(const authum_char_t *path)
{
    return authum_engine_precache_open ? g_engfuncs.pfnPrecacheModel(authum_engine_persistent(path))
                                       : 0;
}

authum_i32_t
authum_engine_precache_sound_impl(const authum_char_t *path)
{
    return authum_engine_precache_open ? g_engfuncs.pfnPrecacheSound(authum_engine_persistent(path))
                                       : 0;
}

authum_i32_t
authum_engine_precache_generic_impl(const authum_char_t *path)
{
    return authum_engine_precache_open
               ? g_engfuncs.pfnPrecacheGeneric(authum_engine_persistent(path))
               : 0;
}

const authum_engine_t authum_engine_ops = {
    sizeof(authum_engine_t),
    authum_engine_get_game_dir_impl,
    authum_engine_server_print_impl,
    authum_engine_server_command_impl,
    authum_engine_max_clients_impl,
    authum_engine_game_time_impl,
    authum_engine_get_map_name_impl,
    authum_engine_cvar_get_impl,
    authum_engine_cvar_set_impl,
    authum_engine_client_is_connected_impl,
    authum_engine_client_userid_impl,
    authum_engine_client_index_by_userid_impl,
    authum_engine_client_get_name_impl,
    authum_engine_client_get_auth_id_impl,
    authum_engine_client_get_address_impl,
    authum_engine_client_get_info_impl,
    authum_engine_client_print_impl,
    authum_engine_client_command_impl,
    authum_engine_query_client_cvar_impl,
    authum_engine_client_get_stats_impl,
    authum_engine_client_set_info_impl,
    authum_engine_cvar_register_impl,
    authum_engine_message_id_impl,
    authum_engine_message_begin_impl,
    authum_engine_message_write_impl,
    authum_engine_message_end_impl,
    authum_engine_precache_model_impl,
    authum_engine_precache_sound_impl,
    authum_engine_precache_generic_impl,
};

} // namespace

extern "C" const authum_engine_t *
authum_engine(void)
{
    return &authum_engine_ops;
}

extern "C" void
authum_engine_host_set_precache(authum_bool_t open)
{
    authum_engine_precache_open = open != 0;
}

extern "C" void
authum_engine_host_set_client_address(authum_i32_t index, const authum_char_t *address)
{
    if (index < 1 || index >= authum_engine_slots)
    {
        return;
    }
    authum_engine_copy(authum_engine_addresses[index], sizeof authum_engine_addresses[index],
                       address);
}
