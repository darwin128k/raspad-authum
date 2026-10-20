#include <authum/event/client.h>
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

static DLL_FUNCTIONS g_function_table = {};

C_DLLEXPORT FORCE_STACK_ALIGN int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
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
    lh_memory_copy(pFunctionTable, sizeof(DLL_FUNCTIONS), lh_addr_of(g_function_table),
                   sizeof(DLL_FUNCTIONS));
    return TRUE;
}
