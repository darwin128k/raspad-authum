/*
 * Standalone entry: Authum as the HLDS game library, no Metamod.
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
#include "gamedll.h"
#include "hooks.h"

#include <authum/console.h>
#include <authum/export.h>
#include <authum/gate.h>
#include <authum/loader.h>
#include <authum/log.h>
#include <authum/module.h>

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
#    define AUTHUM_ENTRY \
        extern "C" AUTHUM_IMAGE_EXPORT __attribute__((force_align_arg_pointer))
#elif defined(_WIN32)
#    define AUTHUM_ENTRY extern "C" AUTHUM_IMAGE_EXPORT
#elif defined(__GNUC__)
#    define AUTHUM_ENTRY extern "C" AUTHUM_IMAGE_EXPORT
#else
#    define AUTHUM_ENTRY extern "C"
#endif

namespace
{
bool g_loader_up = false; /* the loader bound Authum's own image */
bool g_started = false;   /* modules are up */

void
standalone_command(void)
{
    const authum_char_t *argv[64];
    authum_usize_t argc = 0u;
    int engine_argc = g_engfuncs.pfnCmd_Argc();
    int i;

    for (i = 1; i < engine_argc && argc < 64u; ++i)
    {
        argv[argc++] = g_engfuncs.pfnCmd_Argv(i);
    }
    authum_console_run(argc, argv,
                       [](authum_ptr, const authum_char_t *text) {
                           if (text != nullptr && g_engfuncs.pfnServerPrint != nullptr)
                           {
                               g_engfuncs.pfnServerPrint(text);
                           }
                       },
                       nullptr);
}

/* Console-level lines also go to the server console (what `log_console` in
 * authum.cfg asks for): the engine's print, tagged with the module's name. */
void
standalone_log_mirror(authum_logger_level_t /* level */, const authum_char_t *name,
                      const authum_char_t *text)
{
    char line[1024];

    if (text == nullptr || g_engfuncs.pfnServerPrint == nullptr)
    {
        return;
    }
    if (name != nullptr && std::strcmp(name, "authum") != 0)
    {
        std::snprintf(line, sizeof line, "[%s] %s", name, text);
    }
    else
    {
        std::snprintf(line, sizeof line, "%s", text);
    }
    g_engfuncs.pfnServerPrint(line);
}

/* Authum's own folder (addons/authum) holds the logs, next to the image. */
void
standalone_log_init(void)
{
    const lh_fs_path_t *image = lh_os_module_get_path(&authum);
    lh_fs_path_t parent;

    lh_fs_path_init(&parent);
    if (image != nullptr && lh_fs_path_parent(&parent, image))
    {
        (void)authum_log_init_sink(&parent, standalone_log_mirror);
    }
    lh_fs_path_deinit(&parent);
}

void
standalone_start(void)
{
    if (g_started)
    {
        return;
    }
    authum_log_info("Authum standalone starting");
    (void)authum_loader_load_modules(&authum);
    if (authum_gate_init() != lh_bool_true)
    {
        authum_log_error("standalone: connect gate init failed");
    }
    g_engfuncs.pfnAddServerCommand(const_cast<char *>("authum"), standalone_command);
    authum_log_info("server command: authum  (authum help — module commands)");
    g_started = true;
}
} // namespace

namespace authum_standalone
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
} // namespace authum_standalone

AUTHUM_ENTRY void
GiveFnptrsToDll(enginefuncs_t *engine_funcs, globalvars_t *globals)
{
    g_engfuncs = *engine_funcs;
    gpGlobals = globals;

    /* The loader binds Authum's own image: its path says where gamedll.txt
     * and the logs live (addons/authum), whatever the engine's cwd is. */
    if (authum_loader_init() != lh_bool_true)
    {
        authum_log_error("standalone: loader init failed");
        return;
    }
    g_loader_up = true;

    standalone_log_init();

    /* The real game DLL sits behind us: its path is in gamedll.txt, beside
     * authum.so, because liblist.gam now names Authum instead. */
    if (!authum_standalone::gamedll_load(engine_funcs, globals))
    {
        return;
    }

    standalone_start();
}

AUTHUM_ENTRY int
GetEntityAPI2(DLL_FUNCTIONS *table, int *interface_version)
{
    if (table == nullptr || interface_version == nullptr)
    {
        return 0;
    }
    return authum_standalone::hooks_fill_entity_table(table, *interface_version) ? 1 : 0;
}

AUTHUM_ENTRY int
GetEntityAPI(DLL_FUNCTIONS *table, int interface_version)
{
    if (table == nullptr)
    {
        return 0;
    }
    return authum_standalone::hooks_fill_entity_table(table, interface_version) ? 1 : 0;
}

AUTHUM_ENTRY int
GetNewDLLFunctions(NEW_DLL_FUNCTIONS *table, int *interface_version)
{
    if (table == nullptr || interface_version == nullptr)
    {
        return 0;
    }
    return authum_standalone::hooks_fill_new_table(table, interface_version) ? 1 : 0;
}

/* The map's first entity is spawned via dlsym(gamedll, "worldspawn"); that
 * symbol lives in the real game DLL, so forward it. (On HLDS this is the only
 * entity classname the engine resolves by symbol; the rest the game dispatches
 * through its own table.) */
AUTHUM_ENTRY void
worldspawn(entvars_t *pev)
{
    typedef void (*entity_fn_t)(entvars_t *);
    entity_fn_t fn = reinterpret_cast<entity_fn_t>(authum_standalone::gamedll_symbol("worldspawn"));
    if (fn != nullptr)
    {
        fn(pev);
    }
}

/* The engine asks the game for its blending interface (studio models). Forward
 * verbatim; Authum has no interest in it. */
AUTHUM_ENTRY int
Server_GetBlendingInterface(int version, void **ppinterface, void *pstudio, float (*rotationmatrix)[3][4],
                            float (*bonetransform)[128][3][4])
{
    typedef int (*blend_fn_t)(int, void **, void *, float (*)[3][4], float (*)[128][3][4]);
    blend_fn_t fn =
        reinterpret_cast<blend_fn_t>(authum_standalone::gamedll_symbol("Server_GetBlendingInterface"));
    return fn != nullptr ? fn(version, ppinterface, pstudio, rotationmatrix, bonetransform) : 0;
}
