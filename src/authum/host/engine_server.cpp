/**
 * @file engine_server.cpp
 * @brief ::authum_engine_t ops — thin wrappers over host `g_engfuncs`.
 */

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/float.h>
#include <lh/cast/const.h>
#include <lh/cast/reinterpret.h>
#include <lh/cast/static.h>
#include <lh/char/map.h>
#include <lh/math/ops.h>
#include <lh/util/ptr.h>
#include <authum/memory.h>
#include <lh/memory/std.h>
#include <authum/char.h>
#include <authum/engine.h>
#include <authum/host/engine.h>
#include <authum/null.h>
#include <authum/size.h>

#include <extdll.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;

namespace
{

enum
{
    authum_engine_slots = 33, /* 1 … 32, the GoldSrc player limit */
    authum_engine_address_max = 64
};

authum_char_t authum_engine_addresses[authum_engine_slots][authum_engine_address_max];

void
authum_engine_copy(authum_char_t *buf, authum_usize_t buf_size, const authum_char_t *src)
{
    std::snprintf(buf, lh_cast_static(authum_usize_t, buf_size), "%s",
                  lh_math_ne(src, authum_null) ? src : "");
}

/** Engine string table entry (HLSDK `STRING`, without util.h). */
const authum_char_t *
authum_engine_string(string_t offset)
{
    return lh_math_ne(gpGlobals, authum_null) ? lh_math_add(gpGlobals->pStringBase, offset) : "";
}

/** Edict of a connected player in slot @p index, or null. */
edict_t *
authum_engine_client_edict(authum_i32_t index)
{
    edict_t *entity;

    if (lh_math_eq(gpGlobals, authum_null) || lh_math_lt(index, 1) ||
        lh_math_gt(index, gpGlobals->maxClients) ||
        lh_math_eq(g_engfuncs.pfnPEntityOfEntIndex, authum_null) ||
        lh_math_eq(g_engfuncs.pfnGetPlayerUserId, authum_null))
    {
        return authum_null;
    }
    entity = g_engfuncs.pfnPEntityOfEntIndex(index);
    if (lh_math_eq(entity, authum_null) || entity->free ||
        lh_math_le(g_engfuncs.pfnGetPlayerUserId(entity), 0))
    {
        return authum_null;
    }
    return entity;
}

/* --- server --------------------------------------------------------------- */

void
authum_engine_get_game_dir_impl(authum_char_t *buf, authum_usize_t buf_size)
{
    authum_char_t tmp[256];

    tmp[0] = lh_char_map_nul;
    if (lh_math_ne(g_engfuncs.pfnGetGameDir, authum_null))
    {
        g_engfuncs.pfnGetGameDir(tmp);
    }
    authum_engine_copy(buf, buf_size, tmp);
}

void
authum_engine_server_print_impl(const authum_char_t *text)
{
    if (lh_math_ne(g_engfuncs.pfnServerPrint, authum_null))
    {
        g_engfuncs.pfnServerPrint(text);
    }
}

void
authum_engine_server_command_impl(const authum_char_t *cmd)
{
    if (lh_math_ne(g_engfuncs.pfnServerCommand, authum_null))
    {
        /* HLSDK typedef takes char*, not const char*. */
        g_engfuncs.pfnServerCommand(lh_cast_const(authum_char_t *, cmd));
    }
}

authum_i32_t
authum_engine_max_clients_impl(void)
{
    return lh_math_ne(gpGlobals, authum_null) ? gpGlobals->maxClients : 0;
}

authum_float_t
authum_engine_game_time_impl(void)
{
    return lh_math_ne(gpGlobals, authum_null) ? gpGlobals->time : 0.0f;
}

void
authum_engine_get_map_name_impl(authum_char_t *buf, authum_usize_t buf_size)
{
    authum_engine_copy(
        buf, buf_size,
        authum_engine_string(lh_math_ne(gpGlobals, authum_null) ? gpGlobals->mapname : 0));
}

authum_bool_t
authum_engine_cvar_get_impl(const authum_char_t *name, authum_char_t *buf, authum_usize_t buf_size)
{
    cvar_t *cvar;

    if (lh_math_eq(g_engfuncs.pfnCVarGetPointer, authum_null))
    {
        return authum_bool_false;
    }
    cvar = g_engfuncs.pfnCVarGetPointer(name);
    if (lh_math_eq(cvar, authum_null))
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, cvar->string);
    return authum_bool_true;
}

authum_bool_t
authum_engine_cvar_set_impl(const authum_char_t *name, const authum_char_t *value)
{
    if (lh_math_eq(g_engfuncs.pfnCVarGetPointer, authum_null) ||
        lh_math_eq(g_engfuncs.pfnCVarSetString, authum_null) ||
        lh_math_eq(g_engfuncs.pfnCVarGetPointer(name), authum_null))
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
    return lh_math_ne(authum_engine_client_edict(index), authum_null) ? authum_bool_true
                                                                      : authum_bool_false;
}

authum_i32_t
authum_engine_client_userid_impl(authum_i32_t index)
{
    edict_t *entity = authum_engine_client_edict(index);

    return lh_math_ne(entity, authum_null) ? g_engfuncs.pfnGetPlayerUserId(entity) : 0;
}

authum_i32_t
authum_engine_client_index_by_userid_impl(authum_i32_t userid)
{
    authum_i32_t index;

    if (lh_math_le(userid, 0) || lh_math_eq(gpGlobals, authum_null))
    {
        return 0;
    }
    for (index = 1; lh_math_le(index, gpGlobals->maxClients); index = lh_math_add_one(index))
    {
        if (lh_math_eq(authum_engine_client_userid_impl(index), userid))
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

    if (lh_math_eq(entity, authum_null))
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

    if (lh_math_eq(entity, authum_null) || lh_math_eq(g_engfuncs.pfnGetPlayerAuthId, authum_null))
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
    if (lh_math_eq(authum_engine_client_edict(index), authum_null) ||
        lh_math_ge(index, authum_engine_slots))
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, authum_engine_addresses[index]);
    return lh_math_ne(authum_engine_addresses[index][0], lh_char_map_nul) ? authum_bool_true
                                                                          : authum_bool_false;
}

authum_bool_t
authum_engine_client_get_info_impl(authum_i32_t index, const authum_char_t *key, authum_char_t *buf,
                                   authum_usize_t buf_size)
{
    edict_t *entity = authum_engine_client_edict(index);
    authum_char_t *info;

    if (lh_math_eq(entity, authum_null) ||
        lh_math_eq(g_engfuncs.pfnGetInfoKeyBuffer, authum_null) ||
        lh_math_eq(g_engfuncs.pfnInfoKeyValue, authum_null))
    {
        return authum_bool_false;
    }
    info = g_engfuncs.pfnGetInfoKeyBuffer(entity);
    if (lh_math_eq(info, authum_null))
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size,
                       g_engfuncs.pfnInfoKeyValue(info, lh_cast_const(authum_char_t *, key)));
    return authum_bool_true;
}

authum_bool_t
authum_engine_client_print_impl(authum_i32_t index, authum_i32_t kind, const authum_char_t *text)
{
    edict_t *entity;
    authum_bool_t sent = authum_bool_false;
    authum_i32_t i;

    if (lh_math_eq(g_engfuncs.pfnClientPrintf, authum_null) || lh_math_eq(gpGlobals, authum_null))
    {
        return authum_bool_false;
    }
    /* index 0: everyone connected. */
    for (i = (lh_math_eq(index, 0) ? 1 : index);
         lh_math_le(i, (lh_math_eq(index, 0) ? gpGlobals->maxClients : index));
         i = lh_math_add_one(i))
    {
        entity = authum_engine_client_edict(i);
        if (lh_math_ne(entity, authum_null))
        {
            g_engfuncs.pfnClientPrintf(entity, lh_cast_static(PRINT_TYPE, kind), text);
            sent = authum_bool_true;
        }
    }
    return sent;
}

authum_bool_t
authum_engine_client_command_impl(authum_i32_t index, const authum_char_t *cmd)
{
    edict_t *entity = authum_engine_client_edict(index);

    if (lh_math_eq(entity, authum_null) || lh_math_eq(g_engfuncs.pfnClientCommand, authum_null))
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnClientCommand(entity, lh_cast_const(authum_char_t *, "%s\n"), cmd);
    return authum_bool_true;
}

authum_bool_t
authum_engine_query_client_cvar_impl(authum_i32_t index, const authum_char_t *cvar,
                                     authum_i32_t request_id)
{
    edict_t *entity = authum_engine_client_edict(index);

    if (lh_math_eq(entity, authum_null) ||
        lh_math_eq(g_engfuncs.pfnQueryClientCvarValue2, authum_null))
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
    authum_i32_t p = 0;
    authum_i32_t l = 0;

    if (lh_math_eq(entity, authum_null) || lh_math_eq(g_engfuncs.pfnGetPlayerStats, authum_null))
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnGetPlayerStats(entity, lh_addr_of(p), lh_addr_of(l));
    lh_ptr_deref(ping) = p;
    lh_ptr_deref(loss) = l;
    return authum_bool_true;
}

authum_bool_t
authum_engine_client_set_info_impl(authum_i32_t index, const authum_char_t *key,
                                   const authum_char_t *value)
{
    edict_t *entity = authum_engine_client_edict(index);
    authum_char_t *info;

    if (lh_math_eq(entity, authum_null) ||
        lh_math_eq(g_engfuncs.pfnGetInfoKeyBuffer, authum_null) ||
        lh_math_eq(g_engfuncs.pfnSetClientKeyValue, authum_null))
    {
        return authum_bool_false;
    }
    info = g_engfuncs.pfnGetInfoKeyBuffer(entity);
    if (lh_math_eq(info, authum_null))
    {
        return authum_bool_false;
    }
    g_engfuncs.pfnSetClientKeyValue(index, info, lh_cast_const(authum_char_t *, key),
                                    lh_cast_const(authum_char_t *, value));
    return authum_bool_true;
}

/* --- cvars and messages ---------------------------------------------------- */

authum_char_t *
authum_engine_strdup(const authum_char_t *text)
{
    authum_usize_t n = lh_math_add(std::strlen(text), 1u);
    authum_char_t *copy = lh_cast_static(authum_char_t *, authum_memory_alloc(n));

    if (lh_math_ne(copy, authum_null))
    {
        lh_memory_std_copy(copy, text, n);
    }
    return copy;
}

authum_bool_t
authum_engine_cvar_register_impl(const authum_char_t *name, const authum_char_t *value,
                                 authum_i32_t flags)
{
    cvar_t *cvar;

    if (lh_math_eq(g_engfuncs.pfnCVarGetPointer, authum_null) ||
        lh_math_eq(g_engfuncs.pfnCVarRegister, authum_null))
    {
        return authum_bool_false;
    }
    if (lh_math_ne(g_engfuncs.pfnCVarGetPointer(name), authum_null))
    {
        return authum_bool_true;
    }
    /* The engine keeps the pointer for good: this memory is never freed. */
    cvar = lh_cast_static(cvar_t *, authum_memory_alloc_zero(1u, sizeof(cvar_t)));
    if (lh_math_eq(cvar, authum_null))
    {
        return authum_bool_false;
    }
    cvar->name = authum_engine_strdup(name);
    cvar->string = authum_engine_strdup(value);
    cvar->flags = flags;
    if (lh_math_eq(cvar->name, authum_null) || lh_math_eq(cvar->string, authum_null))
    {
        authum_memory_free(cvar->name);
        authum_memory_free(cvar->string);
        authum_memory_free(cvar);
        return authum_bool_false;
    }
    g_engfuncs.pfnCVarRegister(cvar);
    return lh_math_ne(g_engfuncs.pfnCVarGetPointer(name), authum_null) ? authum_bool_true
                                                                       : authum_bool_false;
}

authum_i32_t
authum_engine_message_id_impl(const authum_char_t *name)
{
    /* The engine's own registry: a name the game already registered keeps its
     * id, so this is a lookup for every message in use. */
    if (lh_math_eq(g_engfuncs.pfnRegUserMsg, authum_null) || lh_math_eq(name, authum_null))
    {
        return 0;
    }
    return g_engfuncs.pfnRegUserMsg(name, 0);
}

authum_bool_t
authum_engine_message_begin_impl(authum_i32_t dest, authum_i32_t msg_id, authum_i32_t index)
{
    edict_t *target = authum_null;

    if (lh_math_eq(g_engfuncs.pfnMessageBegin, authum_null))
    {
        return authum_bool_false;
    }
    if (lh_math_eq(dest, authum_message_dest_one) ||
        lh_math_eq(dest, authum_message_dest_one_unreliable))
    {
        target = authum_engine_client_edict(index);
        if (lh_math_eq(target, authum_null))
        {
            return authum_bool_false;
        }
    }
    g_engfuncs.pfnMessageBegin(dest, msg_id, authum_null, target);
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
        g_engfuncs.pfnWriteString(lh_math_ne(text, authum_null) ? text : "");
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
authum_bool_t authum_engine_precache_open = false;

/* The engine keeps the pointer for the whole map: hand it an engine string. */
authum_char_t *
authum_engine_persistent(const authum_char_t *path)
{
    return lh_cast_const(authum_char_t *, authum_engine_string(g_engfuncs.pfnAllocString(path)));
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

/* --- entities ---------------------------------------------------------------- */

/** Live entity @p index (0 = the world), or null. */
edict_t *
authum_engine_entity_edict(authum_i32_t index)
{
    edict_t *entity;

    if (lh_math_eq(gpGlobals, authum_null) || lh_math_lt(index, 0) ||
        lh_math_ge(index, gpGlobals->maxEntities) ||
        lh_math_eq(g_engfuncs.pfnPEntityOfEntIndex, authum_null))
    {
        return authum_null;
    }
    entity = g_engfuncs.pfnPEntityOfEntIndex(index);
    return (lh_math_ne(entity, authum_null) && !entity->free) ? entity : authum_null;
}

authum_float_t *
authum_engine_entity_float_field(entvars_t *v, authum_i32_t field)
{
    switch (field)
    {
    case authum_entity_float_health:
        return lh_addr_of(v->health);
    case authum_entity_float_armor:
        return lh_addr_of(v->armorvalue);
    case authum_entity_float_armor_type:
        return lh_addr_of(v->armortype);
    case authum_entity_float_frags:
        return lh_addr_of(v->frags);
    case authum_entity_float_max_health:
        return lh_addr_of(v->max_health);
    case authum_entity_float_gravity:
        return lh_addr_of(v->gravity);
    case authum_entity_float_max_speed:
        return lh_addr_of(v->maxspeed);
    case authum_entity_float_take_damage:
        return lh_addr_of(v->takedamage);
    case authum_entity_float_friction:
        return lh_addr_of(v->friction);
    case authum_entity_float_fov:
        return lh_addr_of(v->fov);
    case authum_entity_float_render_amount:
        return lh_addr_of(v->renderamt);
    case authum_entity_float_damage_taken:
        return lh_addr_of(v->dmg_take);
    default:
        return authum_null;
    }
}

authum_i32_t *
authum_engine_entity_i32_field(entvars_t *v, authum_i32_t field)
{
    switch (field)
    {
    case authum_entity_i32_flags:
        return lh_addr_of(v->flags);
    case authum_entity_i32_dead:
        return lh_addr_of(v->deadflag);
    case authum_entity_i32_team:
        return lh_addr_of(v->team);
    case authum_entity_i32_move_type:
        return lh_addr_of(v->movetype);
    case authum_entity_i32_solid:
        return lh_addr_of(v->solid);
    case authum_entity_i32_effects:
        return lh_addr_of(v->effects);
    case authum_entity_i32_buttons:
        return lh_addr_of(v->button);
    case authum_entity_i32_old_buttons:
        return lh_addr_of(v->oldbuttons);
    case authum_entity_i32_spawn_flags:
        return lh_addr_of(v->spawnflags);
    case authum_entity_i32_weapons:
        return lh_addr_of(v->weapons);
    case authum_entity_i32_render_mode:
        return lh_addr_of(v->rendermode);
    case authum_entity_i32_render_fx:
        return lh_addr_of(v->renderfx);
    case authum_entity_i32_impulse:
        return lh_addr_of(v->impulse);
    case authum_entity_i32_player_class:
        return lh_addr_of(v->playerclass);
    case authum_entity_i32_water_level:
        return lh_addr_of(v->waterlevel);
    default:
        return authum_null;
    }
}

/* vec3_t is a Vector of three floats in C++: its address is a float[3]. */
authum_float_t *
authum_engine_entity_vector_field(entvars_t *v, authum_i32_t field)
{
    switch (field)
    {
    case authum_entity_vector_origin:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->origin));
    case authum_entity_vector_velocity:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->velocity));
    case authum_entity_vector_angles:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->angles));
    case authum_entity_vector_view_angle:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->v_angle));
    case authum_entity_vector_view_offset:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->view_ofs));
    case authum_entity_vector_mins:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->mins));
    case authum_entity_vector_maxs:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->maxs));
    case authum_entity_vector_punch_angle:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->punchangle));
    case authum_entity_vector_render_color:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->rendercolor));
    case authum_entity_vector_angular_velocity:
        return lh_cast_reinterpret(authum_float_t *, lh_addr_of(v->avelocity));
    default:
        return authum_null;
    }
}

authum_i32_t
authum_engine_entity_max_impl(void)
{
    return lh_math_ne(gpGlobals, authum_null) ? gpGlobals->maxEntities : 0;
}

authum_bool_t
authum_engine_entity_is_valid_impl(authum_i32_t index)
{
    return lh_math_ne(authum_engine_entity_edict(index), authum_null) ? authum_bool_true
                                                                      : authum_bool_false;
}

authum_bool_t
authum_engine_entity_get_classname_impl(authum_i32_t index, authum_char_t *buf,
                                        authum_usize_t buf_size)
{
    edict_t *entity = authum_engine_entity_edict(index);

    if (lh_math_eq(entity, authum_null))
    {
        return authum_bool_false;
    }
    authum_engine_copy(buf, buf_size, authum_engine_string(entity->v.classname));
    return authum_bool_true;
}

authum_i32_t
authum_engine_entity_find_by_classname_impl(authum_i32_t start, const authum_char_t *classname)
{
    edict_t *from = lh_math_gt(start, 0) ? authum_engine_entity_edict(start) : authum_null;
    edict_t *found;

    if ((lh_math_gt(start, 0) && lh_math_eq(from, authum_null)) ||
        lh_math_eq(g_engfuncs.pfnFindEntityByString, authum_null) ||
        lh_math_eq(g_engfuncs.pfnIndexOfEdict, authum_null))
    {
        return 0;
    }
    found = g_engfuncs.pfnFindEntityByString(from, "classname", classname);
    return lh_math_ne(found, authum_null) ? g_engfuncs.pfnIndexOfEdict(found) : 0;
}

authum_bool_t
authum_engine_entity_get_float_impl(authum_i32_t index, authum_i32_t field, authum_float_t *out)
{
    edict_t *entity = authum_engine_entity_edict(index);
    authum_float_t *value = lh_math_ne(entity, authum_null)
                                ? authum_engine_entity_float_field(lh_addr_of(entity->v), field)
                                : authum_null;

    if (lh_math_eq(value, authum_null))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = lh_ptr_deref(value);
    return authum_bool_true;
}

authum_bool_t
authum_engine_entity_set_float_impl(authum_i32_t index, authum_i32_t field, authum_float_t value)
{
    edict_t *entity = authum_engine_entity_edict(index);
    authum_float_t *slot = lh_math_ne(entity, authum_null)
                               ? authum_engine_entity_float_field(lh_addr_of(entity->v), field)
                               : authum_null;

    if (lh_math_eq(slot, authum_null))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(slot) = value;
    return authum_bool_true;
}

authum_bool_t
authum_engine_entity_get_i32_impl(authum_i32_t index, authum_i32_t field, authum_i32_t *out)
{
    edict_t *entity = authum_engine_entity_edict(index);
    authum_i32_t *value = lh_math_ne(entity, authum_null)
                              ? authum_engine_entity_i32_field(lh_addr_of(entity->v), field)
                              : authum_null;

    if (lh_math_eq(value, authum_null))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = lh_ptr_deref(value);
    return authum_bool_true;
}

authum_bool_t
authum_engine_entity_set_i32_impl(authum_i32_t index, authum_i32_t field, authum_i32_t value)
{
    edict_t *entity = authum_engine_entity_edict(index);
    authum_i32_t *slot = lh_math_ne(entity, authum_null)
                             ? authum_engine_entity_i32_field(lh_addr_of(entity->v), field)
                             : authum_null;

    if (lh_math_eq(slot, authum_null))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(slot) = value;
    return authum_bool_true;
}

authum_bool_t
authum_engine_entity_get_vector_impl(authum_i32_t index, authum_i32_t field, authum_float_t out[3])
{
    edict_t *entity = authum_engine_entity_edict(index);
    authum_float_t *value = lh_math_ne(entity, authum_null)
                                ? authum_engine_entity_vector_field(lh_addr_of(entity->v), field)
                                : authum_null;

    if (lh_math_eq(value, authum_null))
    {
        return authum_bool_false;
    }
    out[0] = value[0];
    out[1] = value[1];
    out[2] = value[2];
    return authum_bool_true;
}

authum_bool_t
authum_engine_entity_set_vector_impl(authum_i32_t index, authum_i32_t field,
                                     const authum_float_t value[3])
{
    edict_t *entity = authum_engine_entity_edict(index);
    authum_float_t *slot = lh_math_ne(entity, authum_null)
                               ? authum_engine_entity_vector_field(lh_addr_of(entity->v), field)
                               : authum_null;
    authum_float_t mins[3];
    authum_float_t maxs[3];

    if (lh_math_eq(slot, authum_null))
    {
        return authum_bool_false;
    }
    /* Position and size go through the engine so it relinks the entity. */
    if (lh_math_eq(field, authum_entity_vector_origin) &&
        lh_math_ne(g_engfuncs.pfnSetOrigin, authum_null))
    {
        g_engfuncs.pfnSetOrigin(entity, value);
        return authum_bool_true;
    }
    if ((lh_math_eq(field, authum_entity_vector_mins) ||
         lh_math_eq(field, authum_entity_vector_maxs)) &&
        lh_math_ne(g_engfuncs.pfnSetSize, authum_null))
    {
        lh_memory_std_copy(
            mins,
            lh_math_eq(field, authum_entity_vector_mins)
                ? value
                : lh_cast_reinterpret(const authum_float_t *, lh_addr_of(entity->v.mins)),
            sizeof mins);
        lh_memory_std_copy(
            maxs,
            lh_math_eq(field, authum_entity_vector_maxs)
                ? value
                : lh_cast_reinterpret(const authum_float_t *, lh_addr_of(entity->v.maxs)),
            sizeof maxs);
        g_engfuncs.pfnSetSize(entity, mins, maxs);
        return authum_bool_true;
    }
    slot[0] = value[0];
    slot[1] = value[1];
    slot[2] = value[2];
    return authum_bool_true;
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
    authum_engine_entity_max_impl,
    authum_engine_entity_is_valid_impl,
    authum_engine_entity_get_classname_impl,
    authum_engine_entity_find_by_classname_impl,
    authum_engine_entity_get_float_impl,
    authum_engine_entity_set_float_impl,
    authum_engine_entity_get_i32_impl,
    authum_engine_entity_set_i32_impl,
    authum_engine_entity_get_vector_impl,
    authum_engine_entity_set_vector_impl,
};

} // namespace

extern "C" const authum_engine_t *
authum_engine(void)
{
    return lh_addr_of(authum_engine_ops);
}

extern "C" void
authum_engine_host_set_precache(authum_bool_t open)
{
    authum_engine_precache_open = lh_math_ne(open, 0);
}

extern "C" void
authum_engine_host_set_client_address(authum_i32_t index, const authum_char_t *address)
{
    if (lh_math_lt(index, 1) || lh_math_ge(index, authum_engine_slots))
    {
        return;
    }
    authum_engine_copy(authum_engine_addresses[index], sizeof authum_engine_addresses[index],
                       address);
}
