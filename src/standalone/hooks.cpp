/*
 * The DLL_FUNCTIONS Authum hands the engine: Authum's wrappers for the calls
 * modules care about, each of which publishes an Authum event and then (unless
 * a handler blocked it) calls the real game's function. Everything else is the
 * game's own pointer, copied in gamedll_fill_table.
 *
 * This is the standalone counterpart of src/authum/plugin/dllapi.cpp: same
 * events, but it calls the game directly instead of returning MRES_* to Metamod.
 */
#include "gamedll.h"
#include "hooks.h"

#include <authum/engine/host.h>
#include <authum/event/client.h>
#include <authum/event/server.h>
#include <authum/events/host.h>

#include <extdll.h>

#include <cstdio>
#include <cstring>
#include <ctime>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;

/* HLSDK's STRING(), without pulling in util.h / the Metamod callback macros. */
#ifndef STRING
#    define STRING(offset) (static_cast<const char *>(gpGlobals->pStringBase) + static_cast<int>(offset))
#endif

namespace authum_standalone
{
void shutdown(void); /* defined in entry.cpp: brings Authum down */

namespace
{
DLL_FUNCTIONS g_game_dll;      /* the game's own callbacks */
NEW_DLL_FUNCTIONS g_game_new;  /* the game's NEW_DLL_FUNCTIONS, if any */
bool g_have_new = false;

void
copy(authum_char_t *dst, size_t dst_size, const char *src)
{
    std::snprintf(dst, dst_size, "%s", src != nullptr ? src : "");
}

void
client_fill(authum_event_client_t *ev, edict_t *entity, const char *name)
{
    std::memset(ev, 0, sizeof *ev);
    ev->size = static_cast<authum_usize_t>(sizeof *ev);
    ev->allow = authum_bool_true;
    ev->unix_time = static_cast<authum_i64_t>(std::time(nullptr));
    if (gpGlobals != nullptr)
    {
        ev->game_time = gpGlobals->time;
    }
    if (entity == nullptr)
    {
        return;
    }
    ev->index = g_engfuncs.pfnIndexOfEdict(entity);
    ev->userid = g_engfuncs.pfnGetPlayerUserId(entity);
    copy(ev->auth_id, sizeof ev->auth_id, g_engfuncs.pfnGetPlayerAuthId(entity));
    if (name == nullptr && gpGlobals != nullptr)
    {
        name = STRING(entity->v.netname);
    }
    copy(ev->name, sizeof ev->name, name);
}

void
server_fill(authum_event_server_t *ev)
{
    std::memset(ev, 0, sizeof *ev);
    ev->size = static_cast<authum_usize_t>(sizeof *ev);
    ev->unix_time = static_cast<authum_i64_t>(std::time(nullptr));
    if (gpGlobals == nullptr)
    {
        return;
    }
    copy(ev->map, sizeof ev->map, STRING(gpGlobals->mapname));
    ev->max_clients = gpGlobals->maxClients;
    ev->game_time = gpGlobals->time;
    ev->frame_time = gpGlobals->frametime;
}

/* -- the wrappers: publish, then call the game (or supersede) -------------- */

qboolean
h_client_connect(edict_t *entity, const char *name, const char *address, char reject[128])
{
    authum_event_client_t ev;
    client_fill(&ev, entity, name);
    copy(ev.address, sizeof ev.address, address);
    authum_engine_host_set_client_address(ev.index, address);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_CONNECT, &ev);
    if (!ev.allow)
    {
        authum_engine_host_set_client_address(ev.index, nullptr);
        copy(reject, 128, ev.reject_reason[0] != '\0' ? ev.reject_reason : "Connection refused.");
        return FALSE;
    }
    return g_game_dll.pfnClientConnect != nullptr
               ? g_game_dll.pfnClientConnect(entity, name, address, reject)
               : TRUE;
}

void
h_client_put_in_server(edict_t *entity)
{
    authum_event_client_t ev;
    client_fill(&ev, entity, nullptr);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, &ev);
    if (g_game_dll.pfnClientPutInServer != nullptr)
    {
        g_game_dll.pfnClientPutInServer(entity);
    }
}

void
h_client_user_info_changed(edict_t *entity, char *infobuffer)
{
    authum_event_client_t ev;
    const char *name = nullptr;
    if (infobuffer != nullptr)
    {
        name = g_engfuncs.pfnInfoKeyValue(infobuffer, const_cast<char *>("name"));
    }
    client_fill(&ev, entity, name);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_INFO_CHANGED, &ev);
    if (g_game_dll.pfnClientUserInfoChanged != nullptr)
    {
        g_game_dll.pfnClientUserInfoChanged(entity, infobuffer);
    }
}

void
h_client_disconnect(edict_t *entity)
{
    authum_event_client_t ev;
    client_fill(&ev, entity, nullptr);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_DISCONNECT, &ev);
    authum_engine_host_set_client_address(ev.index, nullptr);
    if (g_game_dll.pfnClientDisconnect != nullptr)
    {
        g_game_dll.pfnClientDisconnect(entity);
    }
}

void
h_client_kill(edict_t *entity)
{
    authum_event_client_t ev;
    client_fill(&ev, entity, nullptr);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_KILL, &ev);
    if (!ev.allow)
    {
        return; /* superseded: the player is not killed */
    }
    if (g_game_dll.pfnClientKill != nullptr)
    {
        g_game_dll.pfnClientKill(entity);
    }
}

void
h_client_command(edict_t *entity)
{
    authum_event_client_command_t ev;
    int i;
    std::memset(&ev, 0, sizeof ev);
    ev.size = static_cast<authum_usize_t>(sizeof ev);
    client_fill(&ev.client, entity, nullptr);
    ev.argc = g_engfuncs.pfnCmd_Argc();
    for (i = 0; i < ev.argc && i < authum_event_client_command_argv_max; ++i)
    {
        copy(ev.argv[i], sizeof ev.argv[i], g_engfuncs.pfnCmd_Argv(i));
    }
    copy(ev.args, sizeof ev.args, g_engfuncs.pfnCmd_Args());
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_COMMAND, &ev);
    if (ev.block)
    {
        return; /* superseded: the game never sees the command */
    }
    if (g_game_dll.pfnClientCommand != nullptr)
    {
        g_game_dll.pfnClientCommand(entity);
    }
}

int
h_spawn(edict_t *entity)
{
    /* worldspawn is the map's first entity: precaching is allowed from here. */
    if (entity != nullptr && gpGlobals != nullptr &&
        std::strcmp(STRING(entity->v.classname), "worldspawn") == 0)
    {
        authum_event_server_t ev;
        authum_engine_host_set_precache(authum_bool_true);
        server_fill(&ev);
        (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_PRECACHE, &ev);
    }
    return g_game_dll.pfnSpawn != nullptr ? g_game_dll.pfnSpawn(entity) : 0;
}

void
h_server_activate(edict_t *edicts, int edict_count, int client_max)
{
    authum_event_server_t ev;
    /* Call the game first: the map is spawned after it returns. */
    if (g_game_dll.pfnServerActivate != nullptr)
    {
        g_game_dll.pfnServerActivate(edicts, edict_count, client_max);
    }
    authum_engine_host_set_precache(authum_bool_false);
    authum_logs_reload();
    server_fill(&ev);
    ev.edict_count = edict_count;
    ev.max_clients = client_max;
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_ACTIVATE, &ev);
}

void
h_server_deactivate(void)
{
    authum_event_server_t ev;
    server_fill(&ev);
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_DEACTIVATE, &ev);
    if (g_game_dll.pfnServerDeactivate != nullptr)
    {
        g_game_dll.pfnServerDeactivate();
    }
}

void
h_start_frame(void)
{
    authum_event_server_t ev;
    server_fill(&ev);
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_FRAME, &ev);
    authum_logs_flush();
    if (g_game_dll.pfnStartFrame != nullptr)
    {
        g_game_dll.pfnStartFrame();
    }
}

void
h_game_shutdown(void)
{
    authum_event_server_t ev;
    server_fill(&ev);
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_SHUTDOWN, &ev);
    authum_logs_flush();
    if (g_have_new && g_game_new.pfnGameShutdown != nullptr)
    {
        g_game_new.pfnGameShutdown();
    }
    shutdown(); /* stop modules, gate and log: the server is going down */
}

void
h_cvar_value2(const edict_t *entity, int request_id, const char *cvar, const char *value)
{
    authum_event_client_cvar_t ev;
    std::memset(&ev, 0, sizeof ev);
    ev.size = static_cast<authum_usize_t>(sizeof ev);
    client_fill(&ev.client, const_cast<edict_t *>(entity), nullptr);
    ev.request_id = request_id;
    copy(ev.cvar, sizeof ev.cvar, cvar);
    copy(ev.value, sizeof ev.value, value);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_CVAR_VALUE, &ev);
    if (g_have_new && g_game_new.pfnCvarValue2 != nullptr)
    {
        g_game_new.pfnCvarValue2(entity, request_id, cvar, value);
    }
}
} // namespace

bool
hooks_fill_entity_table(DLL_FUNCTIONS *out, int interface_version)
{
    /* Take the game's whole table, then overlay Authum's wrappers. */
    std::memset(&g_game_dll, 0, sizeof g_game_dll);
    if (!gamedll_entity_api(&g_game_dll, interface_version))
    {
        return false;
    }
    std::memcpy(out, &g_game_dll, sizeof *out);

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

bool
hooks_fill_new_table(NEW_DLL_FUNCTIONS *out, int *interface_version)
{
    std::memset(&g_game_new, 0, sizeof g_game_new);
    g_have_new = gamedll_new_functions(&g_game_new, interface_version);
    /* The engine always needs a valid table from us; overlay our two hooks
     * over whatever the game provided (or over an empty table). */
    std::memcpy(out, &g_game_new, sizeof *out);
    out->pfnCvarValue2 = h_cvar_value2;
    out->pfnGameShutdown = h_game_shutdown;
    return true;
}

} // namespace authum_standalone
