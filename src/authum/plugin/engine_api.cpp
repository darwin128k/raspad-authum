/**
 * @file engine_api.cpp
 * @brief Engine calls Authum hooks (GetEngineFunctions): the game's user
 *        messages (`authum.message.<Name>`) and its log lines
 *        (`authum.server.log`).
 *
 * A user message is held back only when someone listens to its event: Begin,
 * every Write and End are superseded and recorded, End publishes the whole
 * message, and unless a handler blocked it, it is sent again with the
 * (possibly changed) arguments. While a handler runs, messages it sends pass
 * straight through, so a handler cannot loop on its own message.
 */

#include <authum/event/message.h>
#include <authum/event/server.h>
#include <authum/events/host.h>

#include <extdll.h>
#include <meta_api.h>

#include <cstdarg>
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
extern plugin_info_t Plugin_info;

namespace
{

enum
{
    authum_engine_api_event_max = 64
};

/** The message being held back, between Begin and End. */
authum_event_message_t g_message;
bool g_message_open = false;
/** Re-sending a message, or inside a handler: let every call through. */
int g_message_pass = 0;
/** Inside an authum.server.log handler. */
int g_log_pass = 0;

void
authum_engine_api_arg(authum_i32_t kind, authum_i32_t value, authum_float_t real, const char *text)
{
    authum_event_message_arg_t *arg;
    std::size_t n;

    if (g_message.argc >= authum_event_message_args_max)
    {
        return;
    }
    arg = &g_message.args[g_message.argc++];
    arg->kind = kind;
    arg->value = value;
    arg->real = real;
    arg->text = 0u;
    if (kind == authum_message_arg_string)
    {
        text = text != nullptr ? text : "";
        n = std::strlen(text) + 1u;
        if (g_message.text_used + n > authum_event_message_text_max)
        {
            n = authum_event_message_text_max - g_message.text_used;
        }
        arg->text = g_message.text_used;
        if (n > 0u)
        {
            std::memcpy(g_message.text + g_message.text_used, text, n - 1u);
            g_message.text[g_message.text_used + n - 1u] = '\0';
            g_message.text_used += static_cast<authum_u32_t>(n);
        }
    }
}

/** Send @p ev to the engine as the game would have. */
void
authum_engine_api_resend(const authum_event_message_t *ev, edict_t *target)
{
    authum_i32_t i;

    ++g_message_pass;
    g_engfuncs.pfnMessageBegin(ev->dest, ev->msg_id, ev->has_origin ? ev->origin : nullptr, target);
    for (i = 0; i < ev->argc; ++i)
    {
        const authum_event_message_arg_t *arg = &ev->args[i];

        switch (arg->kind)
        {
        case authum_message_arg_byte:
            g_engfuncs.pfnWriteByte(arg->value);
            break;
        case authum_message_arg_char:
            g_engfuncs.pfnWriteChar(arg->value);
            break;
        case authum_message_arg_short:
            g_engfuncs.pfnWriteShort(arg->value);
            break;
        case authum_message_arg_long:
            g_engfuncs.pfnWriteLong(arg->value);
            break;
        case authum_message_arg_angle:
            g_engfuncs.pfnWriteAngle(arg->real);
            break;
        case authum_message_arg_coord:
            g_engfuncs.pfnWriteCoord(arg->real);
            break;
        case authum_message_arg_string:
            g_engfuncs.pfnWriteString(ev->text + arg->text);
            break;
        case authum_message_arg_entity:
            g_engfuncs.pfnWriteEntity(arg->value);
            break;
        default:
            break;
        }
    }
    g_engfuncs.pfnMessageEnd();
    --g_message_pass;
}

edict_t *g_message_target = nullptr;

void
authum_engine_api_message_begin(int dest, int type, const float *origin, edict_t *target)
{
    char event[authum_engine_api_event_max];
    const char *name;

    if (g_message_pass > 0 || gpMetaUtilFuncs == nullptr ||
        gpMetaUtilFuncs->pfnGetUserMsgName == nullptr)
    {
        RETURN_META(MRES_IGNORED);
    }
    name = gpMetaUtilFuncs->pfnGetUserMsgName(PLID, type, nullptr);
    if (name == nullptr)
    {
        RETURN_META(MRES_IGNORED); /* an engine (svc_*) message, not the game's */
    }
    std::snprintf(event, sizeof event, "%s%s", AUTHUM_EVENT_MESSAGE_PREFIX, name);
    if (!authum_events_host_has(event))
    {
        RETURN_META(MRES_IGNORED);
    }
    std::memset(&g_message, 0, sizeof g_message);
    g_message.size = sizeof g_message;
    g_message.dest = dest;
    g_message.msg_id = type;
    std::snprintf(g_message.name, sizeof g_message.name, "%s", name);
    g_message.index = (target != nullptr && g_engfuncs.pfnIndexOfEdict != nullptr)
                          ? g_engfuncs.pfnIndexOfEdict(target)
                          : 0;
    if (origin != nullptr)
    {
        g_message.has_origin = authum_bool_true;
        g_message.origin[0] = origin[0];
        g_message.origin[1] = origin[1];
        g_message.origin[2] = origin[2];
    }
    g_message_target = target;
    g_message_open = true;
    RETURN_META(MRES_SUPERCEDE);
}

#define AUTHUM_ENGINE_API_WRITE(fn, kind, type, as_value, as_real, as_text)                        \
    void fn(type v)                                                                                \
    {                                                                                              \
        if (!g_message_open || g_message_pass > 0)                                                 \
        {                                                                                          \
            RETURN_META(MRES_IGNORED);                                                             \
        }                                                                                          \
        authum_engine_api_arg(kind, as_value, as_real, as_text);                                   \
        RETURN_META(MRES_SUPERCEDE);                                                               \
    }

AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_byte, authum_message_arg_byte, int, v, 0.0f,
                        nullptr)
AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_char, authum_message_arg_char, int, v, 0.0f,
                        nullptr)
AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_short, authum_message_arg_short, int, v, 0.0f,
                        nullptr)
AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_long, authum_message_arg_long, int, v, 0.0f,
                        nullptr)
AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_entity, authum_message_arg_entity, int, v, 0.0f,
                        nullptr)
AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_angle, authum_message_arg_angle, float, 0, v,
                        nullptr)
AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_coord, authum_message_arg_coord, float, 0, v,
                        nullptr)
AUTHUM_ENGINE_API_WRITE(authum_engine_api_write_string, authum_message_arg_string, const char *, 0,
                        0.0f, v)

void
authum_engine_api_message_end(void)
{
    /* Big (≈3 KB) but on the stack: a handler may send a message of its own. */
    authum_event_message_t ev;
    char event[authum_engine_api_event_max];
    edict_t *target = g_message_target;

    if (!g_message_open || g_message_pass > 0)
    {
        RETURN_META(MRES_IGNORED);
    }
    g_message_open = false;
    lh_memory_copy(lh_addr_of(ev), sizeof ev, lh_addr_of(g_message), sizeof g_message);
    std::snprintf(event, sizeof event, "%s%s", AUTHUM_EVENT_MESSAGE_PREFIX, ev.name);

    ++g_message_pass;
    (void)authum_events_host_publish(event, &ev);
    --g_message_pass;

    if (!ev.block)
    {
        authum_engine_api_resend(&ev, target);
    }
    RETURN_META(MRES_SUPERCEDE);
}

/* The HLDS log: lines written with at_logged while `log on`. */
void
authum_engine_api_alert_message(ALERT_TYPE type, char *fmt, ...)
{
    authum_event_server_log_t ev;
    char tag[64];
    va_list args;
    std::size_t n;

    if (type != at_logged || fmt == nullptr || g_log_pass > 0)
    {
        RETURN_META(MRES_IGNORED);
    }
    std::memset(&ev, 0, sizeof ev);
    ev.size = sizeof ev;
    va_start(args, fmt);
    std::vsnprintf(ev.text, sizeof ev.text, fmt, args);
    va_end(args);
    n = std::strlen(ev.text);
    while (n > 0u && (ev.text[n - 1u] == '\n' || ev.text[n - 1u] == '\r'))
    {
        ev.text[--n] = '\0';
    }
    /* Authum's own console lines come back through here: not news. */
    std::snprintf(tag, sizeof tag, "[%s]", Plugin_info.logtag != nullptr ? Plugin_info.logtag : "");
    if (n == 0u || std::strncmp(ev.text, tag, std::strlen(tag)) == 0)
    {
        RETURN_META(MRES_IGNORED);
    }
    ev.game_time = gpGlobals != nullptr ? gpGlobals->time : 0.0f;
    ev.unix_time = static_cast<authum_i64_t>(std::time(nullptr));

    ++g_log_pass;
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_LOG, &ev);
    --g_log_pass;
    RETURN_META(MRES_IGNORED);
}

enginefuncs_t g_engine_functions = {};

} // namespace

C_DLLEXPORT FORCE_STACK_ALIGN int
GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pengfuncsFromEngine)) ||
        lh_null_eq(lh_ptr_cast(lh_void, interfaceVersion)))
    {
        return FALSE;
    }
    if (*interfaceVersion != ENGINE_INTERFACE_VERSION)
    {
        *interfaceVersion = ENGINE_INTERFACE_VERSION;
        return FALSE;
    }
    g_engine_functions.pfnMessageBegin = authum_engine_api_message_begin;
    g_engine_functions.pfnWriteByte = authum_engine_api_write_byte;
    g_engine_functions.pfnWriteChar = authum_engine_api_write_char;
    g_engine_functions.pfnWriteShort = authum_engine_api_write_short;
    g_engine_functions.pfnWriteLong = authum_engine_api_write_long;
    g_engine_functions.pfnWriteAngle = authum_engine_api_write_angle;
    g_engine_functions.pfnWriteCoord = authum_engine_api_write_coord;
    g_engine_functions.pfnWriteString = authum_engine_api_write_string;
    g_engine_functions.pfnWriteEntity = authum_engine_api_write_entity;
    g_engine_functions.pfnMessageEnd = authum_engine_api_message_end;
    g_engine_functions.pfnAlertMessage = authum_engine_api_alert_message;
    lh_memory_copy(pengfuncsFromEngine, sizeof(enginefuncs_t), lh_addr_of(g_engine_functions),
                   sizeof(enginefuncs_t));
    return TRUE;
}
