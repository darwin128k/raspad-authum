/*
 * The DLL_FUNCTIONS Authum hands the engine: Authum's wrappers for the calls
 * modules care about, each of which publishes an Authum event, runs the hook
 * chain and then (unless a handler stopped the call) calls the real game's
 * function. Everything else is the game's own pointer, copied in
 * hooks_fill_entity_table.
 */
#include <authum/gamedll/game.h>
#include <authum/host/log.h>
#include <authum/unused.h>
#include <lh/cast/const.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>
#include <lh/os/timestamp.h>
#include <authum/gamedll/hooks.h>

#include <authum/host/engine.h>
#include <authum/host/hooks.h>
#include <authum/cstr.h>
#include <authum/event/client.h>
#include <authum/event/server.h>
#include <authum/host/events.h>
#include <authum/host/tasks.h>

#include <extdll.h>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;

/* HLSDK's STRING(), without pulling in util.h / the Metamod callback macros. */
#ifndef STRING
#    define STRING(offset)                                                                         \
        lh_math_add(lh_cast_static(const authum_char_t *, gpGlobals->pStringBase),                 \
                    lh_cast_static(authum_i32_t, offset))
#endif

namespace authum_gamedll
{
void
shutdown(void); /* defined in entry.cpp: brings Authum down */

namespace
{
DLL_FUNCTIONS g_game_dll;     /* the game's own callbacks */
NEW_DLL_FUNCTIONS g_game_new; /* the game's NEW_DLL_FUNCTIONS, if any */
authum_bool_t g_have_new = false;

void
copy(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *src)
{
    authum_cstr_copy(dst, dst_size, lh_math_ne(src, authum_null) ? src : "");
}

void
client_fill(authum_event_client_t *ev, edict_t *entity, const authum_char_t *name)
{
    lh_timestamp_t now = 0;

    lh_memory_std_set(ev, 0, sizeof lh_ptr_deref(ev));
    authum_unused(lh_os_timestamp_try_get_now(lh_addr_of(now)));
    ev->unix_time = lh_cast_static(authum_i64_t, now);
    if (lh_math_ne(gpGlobals, authum_null))
    {
        ev->game_time = gpGlobals->time;
    }
    if (lh_math_eq(entity, authum_null))
    {
        return;
    }
    ev->index = g_engfuncs.pfnIndexOfEdict(entity);
    ev->userid = g_engfuncs.pfnGetPlayerUserId(entity);
    copy(ev->auth_id, sizeof ev->auth_id, g_engfuncs.pfnGetPlayerAuthId(entity));
    if (lh_math_eq(name, authum_null) && lh_math_ne(gpGlobals, authum_null))
    {
        name = STRING(entity->v.netname);
    }
    copy(ev->name, sizeof ev->name, name);
}

void
server_fill(authum_event_server_t *ev)
{
    lh_timestamp_t now = 0;

    lh_memory_std_set(ev, 0, sizeof lh_ptr_deref(ev));
    authum_unused(lh_os_timestamp_try_get_now(lh_addr_of(now)));
    ev->unix_time = lh_cast_static(authum_i64_t, now);
    if (lh_math_eq(gpGlobals, authum_null))
    {
        return;
    }
    copy(ev->map, sizeof ev->map, STRING(gpGlobals->mapname));
    ev->max_clients = gpGlobals->maxClients;
    ev->game_time = gpGlobals->time;
    ev->frame_time = gpGlobals->frametime;
}

/** @brief The game events the hooks publish, as indexes into ::g_hook_event_ids. */
enum hook_event
{
    hook_event_client_command,
    hook_event_client_connect,
    hook_event_client_cvar_value,
    hook_event_client_disconnect,
    hook_event_client_info_changed,
    hook_event_client_kill,
    hook_event_client_put_in_server,
    hook_event_server_activate,
    hook_event_server_deactivate,
    hook_event_server_frame,
    hook_event_server_precache,
    hook_event_server_shutdown,
    hook_event_count
};

/** @brief The public name of each ::hook_event. */
const authum_char_t *const g_hook_event_names[hook_event_count] = {
    AUTHUM_EVENT_CLIENT_COMMAND,       AUTHUM_EVENT_CLIENT_CONNECT,
    AUTHUM_EVENT_CLIENT_CVAR_VALUE,    AUTHUM_EVENT_CLIENT_DISCONNECT,
    AUTHUM_EVENT_CLIENT_INFO_CHANGED,  AUTHUM_EVENT_CLIENT_KILL,
    AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, AUTHUM_EVENT_SERVER_ACTIVATE,
    AUTHUM_EVENT_SERVER_DEACTIVATE,    AUTHUM_EVENT_SERVER_FRAME,
    AUTHUM_EVENT_SERVER_PRECACHE,      AUTHUM_EVENT_SERVER_SHUTDOWN,
};

/** @brief Host-lifetime ID of each ::hook_event; zero until resolved. */
authum_event_id_t g_hook_event_ids[hook_event_count];

/**
 * @brief The host-lifetime ID of @p event, resolved on first use.
 *
 * A failed resolution leaves zero, so the next publication tries again; a
 * publication under zero reaches nobody.
 */
authum_event_id_t
hook_event_id(hook_event event)
{
    return authum_events_host_get_cached_id(lh_addr_of(g_hook_event_ids[event]),
                                            g_hook_event_names[event]);
}

/* -- the wrappers: publish, then call the game (or supersede) -------------- */

qboolean
h_client_connect(edict_t *entity, const authum_char_t *name, const authum_char_t *address,
                 authum_char_t reject[128])
{
    authum_event_client_t ev;
    authum_hook_call_t call;
    client_fill(lh_addr_of(ev), entity, name);
    copy(ev.address, sizeof ev.address, address);
    authum_engine_host_set_client_address(ev.index, address);
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_client_connect), lh_addr_of(ev)));
    lh_memory_std_set(lh_addr_of(call), 0, sizeof call);
    call.point = authum_hook_point_client_connect;
    call.index = ev.index;
    copy(call.name, sizeof call.name, name);
    copy(call.address, sizeof call.address, address);
    call.proceed = authum_bool_true;
    authum_bool_t overran = authum_bool_false;
    if (!authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                   lh_addr_of(call), &overran))
    {
        authum_engine_host_set_client_address(ev.index, authum_null);
        copy(reject, 128,
             lh_math_ne(call.reject[0], lh_char_map_nul) ? call.reject : "Connection refused.");
        authum_log_warning("client %s refused by %s: %s", call.name,
                           call.decided_by ? call.decided_by : "-", reject);
        return FALSE;
    }
    if (overran)
    {
        authum_log_warning("a hook on %s overran its budget and was dropped",
                           authum_hook_point_name(authum_hook_point_client_connect));
    }
    return lh_math_ne(g_game_dll.pfnClientConnect, authum_null)
               ? g_game_dll.pfnClientConnect(entity, call.name, call.address, reject)
               : TRUE;
}

void
h_client_put_in_server(edict_t *entity)
{
    authum_event_client_t ev;
    authum_hook_call_t call;
    client_fill(lh_addr_of(ev), entity, authum_null);
    authum_unused(authum_events_host_publish_id(hook_event_id(hook_event_client_put_in_server),
                                                lh_addr_of(ev)));
    lh_memory_std_set(lh_addr_of(call), 0, sizeof call);
    call.point = authum_hook_point_client_put_in_server;
    call.index = ev.index;
    copy(call.name, sizeof call.name, ev.name);
    call.proceed = authum_bool_true;
    if (!authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_put_in_server,
                                   lh_addr_of(call), authum_null))
    {
        authum_log_info("%s was kept off the server by %s", call.name,
                        call.decided_by ? call.decided_by : "-");
        return; /* superseded: the game never puts the player in */
    }
    if (lh_math_ne(g_game_dll.pfnClientPutInServer, authum_null))
    {
        g_game_dll.pfnClientPutInServer(entity);
    }
}

void
h_client_user_info_changed(edict_t *entity, authum_char_t *infobuffer)
{
    authum_event_client_t ev;
    const authum_char_t *name = authum_null;
    if (lh_math_ne(infobuffer, authum_null))
    {
        name = g_engfuncs.pfnInfoKeyValue(infobuffer, lh_cast_const(authum_char_t *, "name"));
    }
    client_fill(lh_addr_of(ev), entity, name);
    authum_unused(authum_events_host_publish_id(hook_event_id(hook_event_client_info_changed),
                                                lh_addr_of(ev)));
    if (lh_math_ne(g_game_dll.pfnClientUserInfoChanged, authum_null))
    {
        g_game_dll.pfnClientUserInfoChanged(entity, infobuffer);
    }
}

void
h_client_disconnect(edict_t *entity)
{
    authum_event_client_t ev;
    client_fill(lh_addr_of(ev), entity, authum_null);
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_client_disconnect), lh_addr_of(ev)));
    authum_engine_host_set_client_address(ev.index, authum_null);
    if (lh_math_ne(g_game_dll.pfnClientDisconnect, authum_null))
    {
        g_game_dll.pfnClientDisconnect(entity);
    }
}

void
h_client_kill(edict_t *entity)
{
    authum_event_client_t ev;
    authum_hook_call_t call;
    client_fill(lh_addr_of(ev), entity, authum_null);
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_client_kill), lh_addr_of(ev)));
    lh_memory_std_set(lh_addr_of(call), 0, sizeof call);
    call.point = authum_hook_point_client_kill;
    call.index = ev.index;
    copy(call.name, sizeof call.name, ev.name);
    call.proceed = authum_bool_true;
    if (!authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_kill,
                                   lh_addr_of(call), authum_null))
    {
        authum_log_info("kill by %s refused by %s", call.name,
                        call.decided_by ? call.decided_by : "-");
        return; /* superseded: the player is not killed */
    }
    if (lh_math_ne(g_game_dll.pfnClientKill, authum_null))
    {
        g_game_dll.pfnClientKill(entity);
    }
}

void
h_client_command(edict_t *entity)
{
    authum_event_client_command_t ev;
    authum_hook_call_t call;
    authum_i32_t i;
    lh_memory_std_set(lh_addr_of(ev), 0, sizeof ev);
    client_fill(lh_addr_of(ev.client), entity, authum_null);
    ev.argc = g_engfuncs.pfnCmd_Argc();
    for (i = 0; lh_math_lt(i, ev.argc) && lh_math_lt(i, authum_event_client_command_argv_max);
         i = lh_math_add_one(i))
    {
        copy(ev.argv[i], sizeof ev.argv[i], g_engfuncs.pfnCmd_Argv(i));
    }
    copy(ev.args, sizeof ev.args, g_engfuncs.pfnCmd_Args());
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_client_command), lh_addr_of(ev)));
    lh_memory_std_set(lh_addr_of(call), 0, sizeof call);
    call.point = authum_hook_point_client_command;
    call.index = ev.client.index;
    copy(call.name, sizeof call.name, ev.client.name);
    copy(call.command, sizeof call.command, ev.argc > 0 ? ev.argv[0] : "");
    copy(call.args, sizeof call.args, ev.args);
    call.proceed = authum_bool_true;
    if (!authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_command,
                                   lh_addr_of(call), authum_null))
    {
        authum_log_info("command from %s swallowed by %s", call.args,
                        call.decided_by ? call.decided_by : "-");
        return; /* superseded: the game never sees the command */
    }
    if (lh_math_ne(g_game_dll.pfnClientCommand, authum_null))
    {
        g_game_dll.pfnClientCommand(entity);
    }
}

authum_i32_t
h_spawn(edict_t *entity)
{
    /* worldspawn is the map's first entity: precaching is allowed from here. */
    if (lh_math_ne(entity, authum_null) && lh_math_ne(gpGlobals, authum_null) &&
        lh_math_eq(lh_str_ptr_equals(STRING(entity->v.classname), "worldspawn", lh_bool_false),
                   lh_bool_true))
    {
        authum_event_server_t ev;
        authum_engine_host_set_precache(authum_bool_true);
        server_fill(lh_addr_of(ev));
        authum_unused(authum_events_host_publish_id(hook_event_id(hook_event_server_precache),
                                                    lh_addr_of(ev)));
    }
    /* The engine spawns every entity through this slot. The game's own
     * `Spawn` is the old-style entry; the modern one is `DispatchSpawn`
     * (the game exports it as a C++ name) — that is the one Metamod puts
     * here as well. */
    {
        typedef void (*dispatch_fn_t)(edict_t *);
        dispatch_fn_t dispatch =
            lh_cast_reinterpret(dispatch_fn_t, gamedll_symbol("_Z13DispatchSpawnP7edict_s"));
        if (lh_math_ne(dispatch, authum_null))
        {
            dispatch(entity);
            return 1;
        }
    }
    return lh_math_ne(g_game_dll.pfnSpawn, authum_null) ? g_game_dll.pfnSpawn(entity) : 0;
}

void
h_server_activate(edict_t *edicts, authum_i32_t edict_count, authum_i32_t client_max)
{
    authum_event_server_t ev;
    /* Call the game first: the map is spawned after it returns. */
    if (lh_math_ne(g_game_dll.pfnServerActivate, authum_null))
    {
        g_game_dll.pfnServerActivate(edicts, edict_count, client_max);
    }
    authum_engine_host_set_precache(authum_bool_false);
    authum_logs_reload();
    server_fill(lh_addr_of(ev));
    ev.edict_count = edict_count;
    ev.max_clients = client_max;
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_server_activate), lh_addr_of(ev)));
}

void
h_server_deactivate(void)
{
    authum_event_server_t ev;
    server_fill(lh_addr_of(ev));
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_server_deactivate), lh_addr_of(ev)));
    if (lh_math_ne(g_game_dll.pfnServerDeactivate, authum_null))
    {
        g_game_dll.pfnServerDeactivate();
    }
}

void
h_start_frame(void)
{
    authum_tasks_host_pump();
    authum_event_server_t ev;
    server_fill(lh_addr_of(ev));
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_server_frame), lh_addr_of(ev)));
    if (lh_math_ne(g_game_dll.pfnStartFrame, authum_null))
    {
        g_game_dll.pfnStartFrame();
    }
}

void
h_game_shutdown(void)
{
    authum_event_server_t ev;
    server_fill(lh_addr_of(ev));
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_server_shutdown), lh_addr_of(ev)));
    if (g_have_new && lh_math_ne(g_game_new.pfnGameShutdown, authum_null))
    {
        g_game_new.pfnGameShutdown();
    }
    shutdown(); /* stop modules, gate and log: the server is going down */
}

void
h_cvar_value2(const edict_t *entity, authum_i32_t request_id, const authum_char_t *cvar,
              const authum_char_t *value)
{
    authum_event_client_cvar_t ev;
    lh_memory_std_set(lh_addr_of(ev), 0, sizeof ev);
    client_fill(lh_addr_of(ev.client), lh_cast_const(edict_t *, entity), authum_null);
    ev.request_id = request_id;
    copy(ev.cvar, sizeof ev.cvar, cvar);
    copy(ev.value, sizeof ev.value, value);
    authum_unused(
        authum_events_host_publish_id(hook_event_id(hook_event_client_cvar_value), lh_addr_of(ev)));
    if (g_have_new && lh_math_ne(g_game_new.pfnCvarValue2, authum_null))
    {
        g_game_new.pfnCvarValue2(entity, request_id, cvar, value);
    }
}
} // namespace

authum_bool_t
hooks_fill_entity_table(DLL_FUNCTIONS *out, authum_i32_t interface_version)
{
    /* Take the game's whole table, then overlay Authum's wrappers. */
    lh_memory_std_set(lh_addr_of(g_game_dll), 0, sizeof g_game_dll);
    if (!gamedll_entity_api(lh_addr_of(g_game_dll), interface_version))
    {
        return false;
    }
    lh_memory_std_copy(out, lh_addr_of(g_game_dll), sizeof lh_ptr_deref(out));

    out->pfnClientConnect = h_client_connect;
    out->pfnClientConnect = h_client_connect;
    out->pfnClientDisconnect = h_client_disconnect;
    out->pfnClientPutInServer = h_client_put_in_server;
    out->pfnClientUserInfoChanged = h_client_user_info_changed;
    out->pfnClientCommand = h_client_command;
    out->pfnClientKill = h_client_kill;
    out->pfnSpawn = h_spawn;
    out->pfnServerActivate = h_server_activate;
    out->pfnServerDeactivate = h_server_deactivate;
    out->pfnStartFrame = h_start_frame;
    return true;
}

authum_bool_t
hooks_fill_new_table(NEW_DLL_FUNCTIONS *out, authum_i32_t *interface_version)
{
    lh_memory_std_set(lh_addr_of(g_game_new), 0, sizeof g_game_new);
    g_have_new = gamedll_new_functions(lh_addr_of(g_game_new), interface_version);
    /* The engine always needs a valid table from us; overlay our two hooks
     * over whatever the game provided (or over an empty table). */
    lh_memory_std_copy(out, lh_addr_of(g_game_new), sizeof lh_ptr_deref(out));
    out->pfnCvarValue2 = h_cvar_value2;
    out->pfnGameShutdown = h_game_shutdown;
    return true;
}

} // namespace authum_gamedll
