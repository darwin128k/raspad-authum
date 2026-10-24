/*
 * User messages and log lines of the real game, as Authum events.
 *
 * A user message is held back only when someone listens to its event: Begin,
 * every Write and End are recorded instead of reaching the engine, End
 * publishes the whole message, and unless a handler blocked it, it goes to the
 * engine once, with the (possibly changed) arguments. Nothing sits between
 * Authum and the engine, so nobody else sees a message twice.
 *
 * The engine knows a message by number; the name comes from RegUserMsg, which
 * the game calls for each of its messages before it sends any.
 */
#include "engine.h"

#include <authum/event/message.h>
#include <authum/event/server.h>
#include <authum/host/events.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;

/* The game calls these; on i386 Linux it does not keep the stack aligned for
 * SSE, so they realign it like the engine-facing entry points do. */
#if defined(__GNUC__) && defined(__i386__) && !defined(_WIN32)
#    define AUTHUM_GAME_CALL __attribute__((force_align_arg_pointer))
#else
#    define AUTHUM_GAME_CALL
#endif

namespace authum_gamedll
{
namespace
{
enum
{
    engine_event_max = 64,
    engine_msg_max = 256,  /* message numbers are a byte */
    engine_log_max = 2048
};

/* Message number → the name the game registered it under ("" = none). */
char g_msg_names[engine_msg_max][authum_event_message_name_max];

/* The message being held back, between Begin and End. */
authum_event_message_t g_message;
bool g_message_open = false;
edict_t *g_message_target = nullptr;

/* Inside an authum.server.log handler. */
int g_log_pass = 0;

void
engine_arg(authum_i32_t kind, authum_i32_t value, authum_float_t real, const char *text)
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

/* Send @p ev to the engine as the game would have. */
void
engine_send(const authum_event_message_t *ev, edict_t *target)
{
    authum_i32_t i;

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
}

AUTHUM_GAME_CALL int
engine_reg_user_msg(const char *name, int size)
{
    int id = g_engfuncs.pfnRegUserMsg(name, size);

    if (name != nullptr && id > 0 && id < engine_msg_max)
    {
        std::snprintf(g_msg_names[id], sizeof g_msg_names[id], "%s", name);
    }
    return id;
}

AUTHUM_GAME_CALL void
engine_message_begin(int dest, int type, const float *origin, edict_t *target)
{
    char event[engine_event_max];
    const char *name = (type > 0 && type < engine_msg_max) ? g_msg_names[type] : "";

    /* An engine (svc_*) message, or nobody listens: straight through. */
    if (g_message_open || name[0] == '\0')
    {
        g_engfuncs.pfnMessageBegin(dest, type, origin, target);
        return;
    }
    std::snprintf(event, sizeof event, "%s%s", AUTHUM_EVENT_MESSAGE_PREFIX, name);
    if (!authum_events_host_has(event))
    {
        g_engfuncs.pfnMessageBegin(dest, type, origin, target);
        return;
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
}

/* A Write while a message is held is recorded; otherwise it goes on. */
#define AUTHUM_ENGINE_WRITE(fn, engine_fn, kind, type, as_value, as_real, as_text)                 \
    AUTHUM_GAME_CALL void fn(type v)                                                               \
    {                                                                                              \
        if (!g_message_open)                                                                       \
        {                                                                                          \
            g_engfuncs.engine_fn(v);                                                               \
            return;                                                                                \
        }                                                                                          \
        engine_arg(kind, as_value, as_real, as_text);                                              \
    }

AUTHUM_ENGINE_WRITE(engine_write_byte, pfnWriteByte, authum_message_arg_byte, int, v, 0.0f, nullptr)
AUTHUM_ENGINE_WRITE(engine_write_char, pfnWriteChar, authum_message_arg_char, int, v, 0.0f, nullptr)
AUTHUM_ENGINE_WRITE(engine_write_short, pfnWriteShort, authum_message_arg_short, int, v, 0.0f,
                    nullptr)
AUTHUM_ENGINE_WRITE(engine_write_long, pfnWriteLong, authum_message_arg_long, int, v, 0.0f, nullptr)
AUTHUM_ENGINE_WRITE(engine_write_entity, pfnWriteEntity, authum_message_arg_entity, int, v, 0.0f,
                    nullptr)
AUTHUM_ENGINE_WRITE(engine_write_angle, pfnWriteAngle, authum_message_arg_angle, float, 0, v,
                    nullptr)
AUTHUM_ENGINE_WRITE(engine_write_coord, pfnWriteCoord, authum_message_arg_coord, float, 0, v,
                    nullptr)
AUTHUM_ENGINE_WRITE(engine_write_string, pfnWriteString, authum_message_arg_string, const char *, 0,
                    0.0f, v)

AUTHUM_GAME_CALL void
engine_message_end(void)
{
    /* Big (≈3 KB) but on the stack: a handler may send a message of its own. */
    authum_event_message_t ev;
    char event[engine_event_max];
    edict_t *target = g_message_target;

    if (!g_message_open)
    {
        g_engfuncs.pfnMessageEnd();
        return;
    }
    g_message_open = false;
    std::memcpy(&ev, &g_message, sizeof ev);
    std::snprintf(event, sizeof event, "%s%s", AUTHUM_EVENT_MESSAGE_PREFIX, ev.name);
    (void)authum_events_host_publish(event, &ev);
    if (!ev.block)
    {
        engine_send(&ev, target);
    }
}

/* The HLDS log: lines the game writes with at_logged while `log on`. The
 * line always reaches the engine; Authum only reads it. */
AUTHUM_GAME_CALL void
engine_alert_message(ALERT_TYPE type, char *fmt, ...)
{
    char line[engine_log_max];
    authum_event_server_log_t ev;
    va_list args;
    std::size_t n;

    if (fmt == nullptr)
    {
        return;
    }
    va_start(args, fmt);
    std::vsnprintf(line, sizeof line, fmt, args);
    va_end(args);
    g_engfuncs.pfnAlertMessage(type, const_cast<char *>("%s"), line);

    if (type != at_logged || g_log_pass > 0)
    {
        return;
    }
    std::memset(&ev, 0, sizeof ev);
    ev.size = sizeof ev;
    std::snprintf(ev.text, sizeof ev.text, "%s", line);
    n = std::strlen(ev.text);
    while (n > 0u && (ev.text[n - 1u] == '\n' || ev.text[n - 1u] == '\r'))
    {
        ev.text[--n] = '\0';
    }
    if (n == 0u)
    {
        return;
    }
    ev.game_time = gpGlobals != nullptr ? gpGlobals->time : 0.0f;
    ev.unix_time = static_cast<authum_i64_t>(std::time(nullptr));
    ++g_log_pass;
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_LOG, &ev);
    --g_log_pass;
}
} // namespace

void
engine_hook_table(enginefuncs_t *out, const enginefuncs_t *engine)
{
    *out = *engine;
    out->pfnRegUserMsg = engine_reg_user_msg;
    out->pfnMessageBegin = engine_message_begin;
    out->pfnWriteByte = engine_write_byte;
    out->pfnWriteChar = engine_write_char;
    out->pfnWriteShort = engine_write_short;
    out->pfnWriteLong = engine_write_long;
    out->pfnWriteAngle = engine_write_angle;
    out->pfnWriteCoord = engine_write_coord;
    out->pfnWriteString = engine_write_string;
    out->pfnWriteEntity = engine_write_entity;
    out->pfnMessageEnd = engine_message_end;
    out->pfnAlertMessage = engine_alert_message;
}

} // namespace authum_gamedll
