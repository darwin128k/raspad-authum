/*
 * Entry: Authum as the HLDS game library.
 *
 * The engine loads this .so as the game DLL (liblist.gam gamedll_linux). We
 * take the engine's functions, load the real game DLL behind us, bring Authum
 * up (loader, log, gate, the `authum` command), and answer the engine's four
 * entry points with the game's tables, Authum's event hooks overlaid.
 *
 * The order matters: the loader binds Authum's own image first (the host
 * path is what says where gamedll.txt and the logs live), then the log opens,
 * then the real game DLL is loaded and handed the engine's functions.
 */
#include <authum/gamedll/engine.h>
#include <authum/ptr.h>
#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/float.h>
#include <authum/null.h>
#include <authum/size.h>
#include <authum/unused.h>
#include <lh/cast/const.h>
#include <lh/cast/reinterpret.h>
#include <lh/char/map.h>
#include <lh/math/ops.h>
#include <lh/util/ptr.h>
#include <authum/gamedll/game.h>
#include <authum/gamedll/hooks.h>

#include <authum/host/console.h>
#include <authum/export.h>
#include <authum/host/gate.h>
#include <authum/host/loader.h>
#include <authum/host/log.h>
#include <authum/host/module.h>

#include <extdll.h>
#include <eiface.h>

#include <cstdio>
#include <cstring>

#include <lh/fs/path.h>
#include <lh/os/module.h>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;

/* The engine does not align the stack for a game-DLL entry point; the game's
 * SSE code faults on a misaligned one, so every export realigns it itself.
 * The engine resolves these by name, so they must be exported (on Windows
 * that means dllexport, a visibility attribute is not enough). */
#if defined(__GNUC__) && defined(__i386__) && !defined(_WIN32)
#    define AUTHUM_ENTRY extern "C" AUTHUM_IMAGE_EXPORT __attribute__((force_align_arg_pointer))
#elif defined(_WIN32)
#    define AUTHUM_ENTRY extern "C" AUTHUM_IMAGE_EXPORT
#elif defined(__GNUC__)
#    define AUTHUM_ENTRY extern "C" AUTHUM_IMAGE_EXPORT
#else
#    define AUTHUM_ENTRY extern "C"
#endif

/* GiveFnptrsToDll alone is __stdcall on Windows (HLSDK's DLLEXPORT): the
 * engine calls it so and expects it to pop its own arguments — a cdecl one
 * leaves the engine's stack 8 bytes off. Its export name stays undecorated
 * (no @8): MinGW links with --kill-at, MSVC gets the alias below. */
#if defined(_WIN32)
#    define AUTHUM_GIVE_CALL __stdcall
#    if defined(_MSC_VER)
#        pragma comment(linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8")
#    endif
#else
#    define AUTHUM_GIVE_CALL
#endif

namespace
{
authum_bool_t g_loader_up = false; /* the loader bound Authum's own image */
/* The engine as the game sees it: Authum's message and log hooks over it. */
enginefuncs_t g_game_engfuncs;
authum_bool_t g_started = false; /* modules are up */

void
authum_command(void)
{
    const authum_char_t *argv[64];
    authum_usize_t argc = 0u;
    authum_i32_t engine_argc = g_engfuncs.pfnCmd_Argc();
    authum_i32_t i;

    for (i = 1; lh_math_lt(i, engine_argc) && lh_math_lt(argc, 64u); i = lh_math_add_one(i))
    {
        argv[argc++] = g_engfuncs.pfnCmd_Argv(i);
    }
    authum_console_run(
        argc, argv,
        [](authum_ptr, const authum_char_t *text)
        {
            if (lh_math_ne(text, authum_null) && lh_math_ne(g_engfuncs.pfnServerPrint, authum_null))
            {
                g_engfuncs.pfnServerPrint(text);
            }
        },
        authum_null);
}

/* Console-level lines also go to the server console (what `log_console` in
 * authum.cfg asks for): the engine's print, tagged with the module's name. */
void
log_mirror(authum_logger_level_t /* level */, const authum_char_t *name, const authum_char_t *text)
{
    authum_char_t line[1024];
    const authum_char_t *end;

    if (lh_math_eq(text, authum_null) || lh_math_eq(g_engfuncs.pfnServerPrint, authum_null))
    {
        return;
    }
    /* ServerPrint prints as it is: a line needs its own newline. */
    end = (lh_math_ne(text[0], lh_char_map_nul) &&
           lh_math_eq(text[lh_math_sub(std::strlen(text), 1u)], lh_char_map_lf))
              ? ""
              : "\n";
    if (lh_math_ne(name, authum_null) && lh_math_ne(std::strcmp(name, "authum"), 0))
    {
        std::snprintf(line, sizeof line, "[%s] %s%s", name, text, end);
    }
    else
    {
        std::snprintf(line, sizeof line, "%s%s", text, end);
    }
    g_engfuncs.pfnServerPrint(line);
}

/* Authum's own folder (addons/authum) holds the logs, next to the image. */
void
log_init(void)
{
    const lh_fs_path_t *image = lh_os_module_get_path(lh_addr_of(authum));
    lh_fs_path_t parent;

    lh_fs_path_init(lh_addr_of(parent));
    if (lh_math_ne(image, authum_null) && lh_fs_path_parent(lh_addr_of(parent), image))
    {
        authum_unused(authum_log_init_sink(lh_addr_of(parent), log_mirror));
    }
    lh_fs_path_deinit(lh_addr_of(parent));
}

void
start_host(void)
{
    if (g_started)
    {
        return;
    }
    authum_log_info("Authum starting");
    authum_unused(authum_loader_load_modules(lh_addr_of(authum)));
    if (lh_math_ne(authum_gate_init(), lh_bool_true))
    {
        authum_log_error("gamedll: connect gate init failed");
    }
    g_engfuncs.pfnAddServerCommand(lh_cast_const(authum_char_t *, "authum"), authum_command);
    authum_log_info("server command: authum  (authum help — module commands)");
    g_started = true;
}
} // namespace

namespace authum_gamedll
{
void
shutdown(void)
{
    if (g_started)
    {
        authum_gate_deinit();
        g_started = false;
    }
    if (g_loader_up)
    {
        authum_loader_deinit();
        g_loader_up = false;
    }
    authum_log_deinit();
}
} // namespace authum_gamedll

AUTHUM_ENTRY void AUTHUM_GIVE_CALL
GiveFnptrsToDll(enginefuncs_t *engine_funcs, globalvars_t *globals)
{
    g_engfuncs = lh_ptr_deref(engine_funcs);
    gpGlobals = globals;

    /* The loader binds Authum's own image: its path says where gamedll.txt
     * and the logs live (addons/authum), whatever the engine's cwd is. */
    if (lh_math_ne(authum_loader_init(), lh_bool_true))
    {
        authum_log_error("gamedll: loader init failed");
        return;
    }
    g_loader_up = true;

    log_init();

    /* The real game DLL sits behind us: its path is in gamedll.txt, beside
     * authum.so, because liblist.gam now names Authum instead. It gets the
     * engine with Authum's hooks over user messages and the log. */
    authum_gamedll::engine_hook_table(lh_addr_of(g_game_engfuncs), engine_funcs);
    if (!authum_gamedll::gamedll_load(lh_addr_of(g_game_engfuncs), globals))
    {
        return;
    }

    start_host();
}

AUTHUM_ENTRY authum_i32_t
GetEntityAPI2(DLL_FUNCTIONS *table, authum_i32_t *interface_version)
{
    if (lh_math_eq(table, authum_null) || lh_math_eq(interface_version, authum_null))
    {
        return 0;
    }
    return authum_gamedll::hooks_fill_entity_table(table, lh_ptr_deref(interface_version)) ? 1 : 0;
}

AUTHUM_ENTRY authum_i32_t
GetEntityAPI(DLL_FUNCTIONS *table, authum_i32_t interface_version)
{
    if (lh_math_eq(table, authum_null))
    {
        return 0;
    }
    return authum_gamedll::hooks_fill_entity_table(table, interface_version) ? 1 : 0;
}

AUTHUM_ENTRY authum_i32_t
GetNewDLLFunctions(NEW_DLL_FUNCTIONS *table, authum_i32_t *interface_version)
{
    if (lh_math_eq(table, authum_null) || lh_math_eq(interface_version, authum_null))
    {
        return 0;
    }
    return authum_gamedll::hooks_fill_new_table(table, interface_version) ? 1 : 0;
}

/* The map's first entity is spawned via dlsym(gamedll, "worldspawn"); that
 * symbol lives in the real game DLL, so forward it. (On HLDS this is the only
 * entity classname the engine resolves by symbol; the rest the game dispatches
 * through its own table.) */
AUTHUM_ENTRY void
worldspawn(entvars_t *pev)
{
    typedef void (*entity_fn_t)(entvars_t *);
    entity_fn_t fn = lh_cast_reinterpret(entity_fn_t, authum_gamedll::gamedll_symbol("worldspawn"));
    if (lh_math_ne(fn, authum_null))
    {
        fn(pev);
    }
}

/* The engine asks the game for its blending interface (studio models). Forward
 * verbatim; Authum has no interest in it. */
AUTHUM_ENTRY authum_i32_t
Server_GetBlendingInterface(authum_i32_t version, authum_ptr *ppinterface, authum_ptr pstudio,
                            authum_float_t (*rotationmatrix)[3][4],
                            authum_float_t (*bonetransform)[128][3][4])
{
    typedef authum_i32_t (*blend_fn_t)(authum_i32_t, authum_ptr *, authum_ptr,
                                       authum_float_t(*)[3][4], authum_float_t(*)[128][3][4]);
    blend_fn_t fn = lh_cast_reinterpret(
        blend_fn_t, authum_gamedll::gamedll_symbol("Server_GetBlendingInterface"));
    return lh_math_ne(fn, authum_null)
               ? fn(version, ppinterface, pstudio, rotationmatrix, bonetransform)
               : 0;
}
