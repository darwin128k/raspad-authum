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
#include <authum/gamedll/engine.h>
#include <authum/null.h>
#include <authum/unused.h>
#include <lh/cast/const.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>
#include <lh/util/ptr.h>

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
    engine_msg_max = 256, /* message numbers are a byte */
    engine_log_max = 2048
};

/* Message number → the name the game registered it under ("" = none). */
authum_char_t g_msg_names[engine_msg_max][authum_event_message_name_max];

/* Message number -> host-lifetime event ID; resolved once after registration. */
authum_event_id_t g_msg_events[engine_msg_max];

/* The message being held back, between Begin and End. */
authum_event_message_t g_message;
authum_bool_t g_message_open = false;
authum_event_id_t g_message_event = 0u;
edict_t *g_message_target = authum_null;

/* Inside an authum.server.log handler. */
authum_i32_t g_log_pass = 0;

void
engine_arg(authum_i32_t kind, authum_i32_t value, authum_float_t real, const authum_char_t *text)
{
    authum_event_message_arg_t *arg;
    authum_usize_t n;

    if (lh_math_ge(g_message.argc, authum_event_message_args_max))
    {
        return;
    }
    arg = lh_addr_of(g_message.args[g_message.argc++]);
    arg->kind = kind;
    arg->value = value;
    arg->real = real;
    arg->text = 0u;
    if (lh_math_eq(kind, authum_message_arg_string))
    {
        text = lh_math_ne(text, authum_null) ? text : "";
        n = lh_math_add(std::strlen(text), 1u);
        if (lh_math_gt(lh_math_add(g_message.text_used, n), authum_event_message_text_max))
        {
            n = lh_math_sub(authum_event_message_text_max, g_message.text_used);
        }
        arg->text = g_message.text_used;
        if (lh_math_gt(n, 0u))
        {
            lh_memory_std_copy(lh_math_add(g_message.text, g_message.text_used), text,
                               lh_math_sub(n, 1u));
            g_message.text[lh_math_sub(lh_math_add(g_message.text_used, n), 1u)] = lh_char_map_nul;
            g_message.text_used += lh_cast_static(authum_u32_t, n);
        }
    }
}

/* Send @p ev to the engine as the game would have. */
void
engine_send(const authum_event_message_t *ev, edict_t *target)
{
    authum_i32_t i;

    g_engfuncs.pfnMessageBegin(ev->dest, ev->msg_id, ev->has_origin ? ev->origin : authum_null,
                               target);
    for (i = 0; lh_math_lt(i, ev->argc); i = lh_math_add_one(i))
    {
        const authum_event_message_arg_t *arg = lh_addr_of(ev->args[i]);

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
            g_engfuncs.pfnWriteString(lh_math_add(ev->text, arg->text));
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

AUTHUM_GAME_CALL authum_i32_t
engine_reg_user_msg(const authum_char_t *name, authum_i32_t size)
{
    authum_i32_t id = g_engfuncs.pfnRegUserMsg(name, size);

    if (lh_math_ne(name, authum_null) && lh_math_gt(id, 0) && lh_math_lt(id, engine_msg_max))
    {
        authum_char_t event[engine_event_max];
        std::snprintf(g_msg_names[id], sizeof g_msg_names[id], "%s", name);
        std::snprintf(event, sizeof event, "%s%s", AUTHUM_EVENT_MESSAGE_PREFIX, g_msg_names[id]);
        g_msg_events[id] = authum_events_host_get_id(event);
    }
    return id;
}

AUTHUM_GAME_CALL void
engine_message_begin(authum_i32_t dest, authum_i32_t type, const authum_float_t *origin,
                     edict_t *target)
{
    authum_char_t event[engine_event_max];
    const authum_char_t *name =
        (lh_math_gt(type, 0) && lh_math_lt(type, engine_msg_max)) ? g_msg_names[type] : "";

    /* An engine (svc_*) message, or nobody listens: straight through. */
    if (g_message_open || lh_math_eq(name[0], lh_char_map_nul))
    {
        g_engfuncs.pfnMessageBegin(dest, type, origin, target);
        return;
    }
    if (lh_math_eq(g_msg_events[type], 0u))
    {
        std::snprintf(event, sizeof event, "%s%s", AUTHUM_EVENT_MESSAGE_PREFIX, name);
        g_msg_events[type] = authum_events_host_get_id(event);
    }
    if (!authum_events_host_has_id(g_msg_events[type]))
    {
        g_engfuncs.pfnMessageBegin(dest, type, origin, target);
        return;
    }
    lh_memory_std_set(lh_addr_of(g_message), 0, sizeof g_message);
    g_message.size = sizeof g_message;
    g_message.dest = dest;
    g_message.msg_id = type;
    std::snprintf(g_message.name, sizeof g_message.name, "%s", name);
    g_message.index =
        (lh_math_ne(target, authum_null) && lh_math_ne(g_engfuncs.pfnIndexOfEdict, authum_null))
            ? g_engfuncs.pfnIndexOfEdict(target)
            : 0;
    if (lh_math_ne(origin, authum_null))
    {
        g_message.has_origin = authum_bool_true;
        g_message.origin[0] = origin[0];
        g_message.origin[1] = origin[1];
        g_message.origin[2] = origin[2];
    }
    g_message_target = target;
    g_message_event = g_msg_events[type];
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

AUTHUM_ENGINE_WRITE(engine_write_byte, pfnWriteByte, authum_message_arg_byte, authum_i32_t, v, 0.0f,
                    authum_null)
AUTHUM_ENGINE_WRITE(engine_write_char, pfnWriteChar, authum_message_arg_char, authum_i32_t, v, 0.0f,
                    authum_null)
AUTHUM_ENGINE_WRITE(engine_write_short, pfnWriteShort, authum_message_arg_short, authum_i32_t, v,
                    0.0f, authum_null)
AUTHUM_ENGINE_WRITE(engine_write_long, pfnWriteLong, authum_message_arg_long, authum_i32_t, v, 0.0f,
                    authum_null)
AUTHUM_ENGINE_WRITE(engine_write_entity, pfnWriteEntity, authum_message_arg_entity, authum_i32_t, v,
                    0.0f, authum_null)
AUTHUM_ENGINE_WRITE(engine_write_angle, pfnWriteAngle, authum_message_arg_angle, authum_float_t, 0,
                    v, authum_null)
AUTHUM_ENGINE_WRITE(engine_write_coord, pfnWriteCoord, authum_message_arg_coord, authum_float_t, 0,
                    v, authum_null)
AUTHUM_ENGINE_WRITE(engine_write_string, pfnWriteString, authum_message_arg_string,
                    const authum_char_t *, 0, 0.0f, v)

AUTHUM_GAME_CALL void
engine_message_end(void)
{
    /* Big (≈3 KB) but on the stack: a handler may send a message of its own. */
    authum_event_message_t ev;
    edict_t *target = g_message_target;
    authum_event_id_t event = g_message_event;

    if (!g_message_open)
    {
        g_engfuncs.pfnMessageEnd();
        return;
    }
    g_message_open = false;
    lh_memory_std_copy(lh_addr_of(ev), lh_addr_of(g_message), sizeof ev);
    authum_unused(authum_events_host_publish_id(event, lh_addr_of(ev)));
    if (!ev.block)
    {
        engine_send(lh_addr_of(ev), target);
    }
}

/* The HLDS log: lines the game writes with at_logged while `log on`. The
 * line always reaches the engine; Authum only reads it. */
AUTHUM_GAME_CALL void
engine_alert_message(ALERT_TYPE type, authum_char_t *fmt, ...)
{
    authum_char_t line[engine_log_max];
    authum_event_server_log_t ev;
    va_list args;
    authum_usize_t n;

    if (lh_math_eq(fmt, authum_null))
    {
        return;
    }
    va_start(args, fmt);
    std::vsnprintf(line, sizeof line, fmt, args);
    va_end(args);
    g_engfuncs.pfnAlertMessage(type, lh_cast_const(authum_char_t *, "%s"), line);

    if (lh_math_ne(type, at_logged) || lh_math_gt(g_log_pass, 0))
    {
        return;
    }
    lh_memory_std_set(lh_addr_of(ev), 0, sizeof ev);
    ev.size = sizeof ev;
    std::snprintf(ev.text, sizeof ev.text, "%s", line);
    n = std::strlen(ev.text);
    while (lh_math_gt(n, 0u) && (lh_math_eq(ev.text[lh_math_sub(n, 1u)], lh_char_map_lf) ||
                                 lh_math_eq(ev.text[lh_math_sub(n, 1u)], lh_char_map_cr)))
    {
        ev.text[--n] = lh_char_map_nul;
    }
    if (lh_math_eq(n, 0u))
    {
        return;
    }
    ev.game_time = lh_math_ne(gpGlobals, authum_null) ? gpGlobals->time : 0.0f;
    ev.unix_time = lh_cast_static(authum_i64_t, std::time(authum_null));
    g_log_pass = lh_math_add_one(g_log_pass);
    static authum_event_id_t log_id = 0u;
    authum_events_host_get_cached_id(lh_addr_of(log_id), AUTHUM_EVENT_SERVER_LOG);
    authum_unused(authum_events_host_publish_id(log_id, lh_addr_of(ev)));
    g_log_pass = lh_math_sub_one(g_log_pass);
}
} // namespace

void
engine_hook_table(enginefuncs_t *out, const enginefuncs_t *engine)
{
    lh_ptr_deref(out) = lh_ptr_deref(engine);
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
