#include <authum/event/client.h>
#include <authum/event/server.h>
#include <authum/events/host.h>

#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>

#include <cstdio>
#include <cstring>
#include <ctime>

#include <lh/memory.h>
#include <lh/null.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/void.h>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;

void
authum_dllapi_copy(authum_char_t *dst, size_t dst_size, const char *src)
{
    std::snprintf(dst, dst_size, "%s", (src != nullptr) ? src : "");
}

/** Fill the fields every `authum.client.*` event carries. */
void
authum_dllapi_client_fill(authum_event_client_t *ev, edict_t *entity, const char *name)
{
    std::memset(ev, 0, sizeof(*ev));
    ev->size = static_cast<authum_usize_t>(sizeof(*ev));
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
    authum_dllapi_copy(ev->auth_id, sizeof(ev->auth_id), g_engfuncs.pfnGetPlayerAuthId(entity));
    if (name == nullptr && gpGlobals != nullptr)
    {
        name = STRING(entity->v.netname);
    }
    authum_dllapi_copy(ev->name, sizeof(ev->name), name);
}

qboolean
authum_dllapi_client_connect(edict_t *entity, const char *name, const char *address,
                             char reject_reason[128])
{
    authum_event_client_t ev;

    authum_dllapi_client_fill(&ev, entity, name);
    authum_dllapi_copy(ev.address, sizeof(ev.address), address);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_CONNECT, &ev);
    if (!ev.allow)
    {
        authum_dllapi_copy(reject_reason, 128,
                           ev.reject_reason[0] != '\0' ? ev.reject_reason : "Connection refused.");
        RETURN_META_VALUE(MRES_SUPERCEDE, FALSE);
    }
    RETURN_META_VALUE(MRES_IGNORED, TRUE);
}

void
authum_dllapi_client_put_in_server(edict_t *entity)
{
    authum_event_client_t ev;

    authum_dllapi_client_fill(&ev, entity, nullptr);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_PUT_IN_SERVER, &ev);
    RETURN_META(MRES_IGNORED);
}

void
authum_dllapi_client_user_info_changed(edict_t *entity, char *infobuffer)
{
    authum_event_client_t ev;
    const char *name = nullptr;

    if (infobuffer != nullptr)
    {
        name = g_engfuncs.pfnInfoKeyValue(infobuffer, const_cast<char *>("name"));
    }
    authum_dllapi_client_fill(&ev, entity, name);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_INFO_CHANGED, &ev);
    RETURN_META(MRES_IGNORED);
}

void
authum_dllapi_client_disconnect(edict_t *entity)
{
    authum_event_client_t ev;

    authum_dllapi_client_fill(&ev, entity, nullptr);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_DISCONNECT, &ev);
    RETURN_META(MRES_IGNORED);
}

void
authum_dllapi_client_command(edict_t *entity)
{
    authum_event_client_command_t ev;
    int i;

    std::memset(&ev, 0, sizeof(ev));
    ev.size = static_cast<authum_usize_t>(sizeof(ev));
    authum_dllapi_client_fill(&ev.client, entity, nullptr);
    ev.argc = g_engfuncs.pfnCmd_Argc();
    for (i = 0; i < ev.argc && i < authum_event_client_command_argv_max; ++i)
    {
        authum_dllapi_copy(ev.argv[i], sizeof(ev.argv[i]), g_engfuncs.pfnCmd_Argv(i));
    }
    authum_dllapi_copy(ev.args, sizeof(ev.args), g_engfuncs.pfnCmd_Args());
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_COMMAND, &ev);
    if (ev.block)
    {
        RETURN_META(MRES_SUPERCEDE);
    }
    RETURN_META(MRES_IGNORED);
}

/** Fill the fields every `authum.server.*` event carries. */
void
authum_dllapi_server_fill(authum_event_server_t *ev)
{
    std::memset(ev, 0, sizeof(*ev));
    ev->size = static_cast<authum_usize_t>(sizeof(*ev));
    ev->unix_time = static_cast<authum_i64_t>(std::time(nullptr));
    if (gpGlobals == nullptr)
    {
        return;
    }
    authum_dllapi_copy(ev->map, sizeof(ev->map), STRING(gpGlobals->mapname));
    ev->max_clients = gpGlobals->maxClients;
    ev->game_time = gpGlobals->time;
    ev->frame_time = gpGlobals->frametime;
}

/* Post hook: the game DLL has spawned the map by now. */
void
authum_dllapi_server_activate_post(edict_t * /* edicts */, int edict_count, int client_max)
{
    authum_event_server_t ev;

    authum_dllapi_server_fill(&ev);
    ev.edict_count = edict_count;
    ev.max_clients = client_max;
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_ACTIVATE, &ev);
    RETURN_META(MRES_IGNORED);
}

void
authum_dllapi_server_deactivate(void)
{
    authum_event_server_t ev;

    authum_dllapi_server_fill(&ev);
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_DEACTIVATE, &ev);
    RETURN_META(MRES_IGNORED);
}

void
authum_dllapi_start_frame(void)
{
    authum_event_server_t ev;

    authum_dllapi_server_fill(&ev);
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_FRAME, &ev);
    RETURN_META(MRES_IGNORED);
}

void
authum_dllapi_cvar_value2(const edict_t *entity, int request_id, const char *cvar,
                          const char *value)
{
    authum_event_client_cvar_t ev;

    std::memset(&ev, 0, sizeof(ev));
    ev.size = static_cast<authum_usize_t>(sizeof(ev));
    authum_dllapi_client_fill(&ev.client, const_cast<edict_t *>(entity), nullptr);
    ev.request_id = request_id;
    authum_dllapi_copy(ev.cvar, sizeof(ev.cvar), cvar);
    authum_dllapi_copy(ev.value, sizeof(ev.value), value);
    (void)authum_events_host_publish(AUTHUM_EVENT_CLIENT_CVAR_VALUE, &ev);
    RETURN_META(MRES_IGNORED);
}

static DLL_FUNCTIONS g_function_table = {};

C_DLLEXPORT FORCE_STACK_ALIGN int
GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pFunctionTable)) ||
        lh_null_eq(lh_ptr_cast(lh_void, interfaceVersion)))
    {
        return FALSE;
    }
    if (*interfaceVersion != INTERFACE_VERSION)
    {
        *interfaceVersion = INTERFACE_VERSION;
        return FALSE;
    }
    g_function_table.pfnClientConnect = authum_dllapi_client_connect;
    g_function_table.pfnClientDisconnect = authum_dllapi_client_disconnect;
    g_function_table.pfnClientPutInServer = authum_dllapi_client_put_in_server;
    g_function_table.pfnClientUserInfoChanged = authum_dllapi_client_user_info_changed;
    g_function_table.pfnClientCommand = authum_dllapi_client_command;
    g_function_table.pfnServerDeactivate = authum_dllapi_server_deactivate;
    g_function_table.pfnStartFrame = authum_dllapi_start_frame;
    lh_memory_copy(pFunctionTable, sizeof(DLL_FUNCTIONS), lh_addr_of(g_function_table),
                   sizeof(DLL_FUNCTIONS));
    return TRUE;
}

static DLL_FUNCTIONS g_function_table_post = {};

C_DLLEXPORT FORCE_STACK_ALIGN int
GetEntityAPI2_Post(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pFunctionTable)) ||
        lh_null_eq(lh_ptr_cast(lh_void, interfaceVersion)))
    {
        return FALSE;
    }
    if (*interfaceVersion != INTERFACE_VERSION)
    {
        *interfaceVersion = INTERFACE_VERSION;
        return FALSE;
    }
    g_function_table_post.pfnServerActivate = authum_dllapi_server_activate_post;
    lh_memory_copy(pFunctionTable, sizeof(DLL_FUNCTIONS), lh_addr_of(g_function_table_post),
                   sizeof(DLL_FUNCTIONS));
    return TRUE;
}

static NEW_DLL_FUNCTIONS g_new_function_table = {};

C_DLLEXPORT FORCE_STACK_ALIGN int
GetNewDLLFunctions(NEW_DLL_FUNCTIONS *pNewFunctionTable, int *interfaceVersion)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pNewFunctionTable)) ||
        lh_null_eq(lh_ptr_cast(lh_void, interfaceVersion)))
    {
        return FALSE;
    }
    if (*interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
    {
        *interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
        return FALSE;
    }
    g_new_function_table.pfnCvarValue2 = authum_dllapi_cvar_value2;
    lh_memory_copy(pNewFunctionTable, sizeof(NEW_DLL_FUNCTIONS), lh_addr_of(g_new_function_table),
                   sizeof(NEW_DLL_FUNCTIONS));
    return TRUE;
}
