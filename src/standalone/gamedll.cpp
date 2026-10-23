#include "gamedll.h"

#include <authum/log.h>
#include <authum/module.h>

#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/null.h>
#include <lh/os/module.h>
#include <lh/str.h>
#include <lh/util/addr.h>
#include <lh/util/str/ptr.h>

#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern enginefuncs_t g_engfuncs;

namespace authum_standalone
{
namespace
{
typedef void (*give_fn_t)(enginefuncs_t *, globalvars_t *);
typedef int (*get_entity_api_t)(DLL_FUNCTIONS *, int);  /* old: version by value */
typedef int (*get_entity_api2_t)(DLL_FUNCTIONS *, int *);
typedef int (*get_new_dll_funcs_t)(NEW_DLL_FUNCTIONS *, int *);

void *g_game = nullptr;

/* The real game DLL is named in Authum's own file, next to authum.so.
 *
 * The engine reads the game library path from `gamedll_linux` in liblist.gam;
 * that line now points at authum.so, so the original path ("dlls/cs.so") is
 * kept beside the host image — addons/authum/gamedll.txt. A relative path is
 * resolved against the game dir, the way liblist.gam resolves it; an
 * absolute one is taken as it is. $AUTHUM_GAMEDLL overrides it (tests).
 */
const char authum_gamedll_file[] = "gamedll.txt";
const char authum_gamedll_default[] = "dlls/cs.so";

/* Trim the line in place: no comment, no surrounding spaces. */
void
trim(char *line)
{
    char *start = line;
    char *end;
    char *comment;

    comment = std::strchr(line, '#');
    if (comment != nullptr)
    {
        *comment = '\0';
    }
    while (*start == ' ' || *start == '\t')
    {
        ++start;
    }
    if (start != line)
    {
        std::memmove(line, start, std::strlen(start) + 1u);
    }
    end = line + std::strlen(line);
    while (end > line &&
           (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
    {
        --end;
        *end = '\0';
    }
}

/* Authum's own folder — the parent of the host image (addons/authum). */
bool
host_dir(char *out, size_t out_size)
{
    const lh_fs_path_t *image;
    lh_fs_path_t parent;
    lh_str_t scratch;
    const lh_str_ptr text;
    bool ok;

    image = lh_os_module_get_path(&authum);
    lh_fs_path_init(lh_addr_of(parent));
    lh_str_init(lh_addr_of(scratch));
    text = lh_fs_path_to_cstr(image, lh_fs_path_style_posix, lh_addr_of(scratch));
    ok = lh_null_ne(text) && lh_fs_path_parent(lh_addr_of(parent), image);
    if (ok)
    {
        const lh_str_ptr dir =
            lh_fs_path_to_cstr(lh_addr_of(parent), lh_fs_path_style_posix, lh_addr_of(scratch));
        ok = lh_null_ne(dir);
        if (ok)
        {
            std::snprintf(out, out_size, "%s", dir);
        }
    }
    lh_str_deinit(lh_addr_of(scratch));
    lh_fs_path_deinit(lh_addr_of(parent));
    return ok;
}

/* The game DLL path as written in gamedll.txt (or $AUTHUM_GAMEDLL). */
bool
read_gamedll_name(const char *dir, char *out, size_t out_size)
{
    const char *env = std::getenv("AUTHUM_GAMEDLL");
    char path[600];
    char line[256];
    FILE *f;

    if (env != nullptr && env[0] != '\0')
    {
        std::snprintf(out, out_size, "%s", env);
        return true;
    }
    std::snprintf(path, sizeof path, "%s/%s", dir, authum_gamedll_file);
    f = std::fopen(path, "r");
    if (f == nullptr)
    {
        authum_log_error("standalone: no %s beside authum.so (looked at %s)",
                         authum_gamedll_file, path);
        std::snprintf(out, out_size, "%s", authum_gamedll_default);
        return false;
    }
    while (std::fgets(line, sizeof line, f) != nullptr)
    {
        trim(line);
        if (line[0] == '\0')
        {
            continue;
        }
        std::snprintf(out, out_size, "%s", line);
        std::fclose(f);
        return true;
    }
    std::fclose(f);
    authum_log_error("standalone: %s is empty", path);
    std::snprintf(out, out_size, "%s", authum_gamedll_default);
    return false;
}
/* Open the real game library.
 *
 * The build links it into authum.so (a stub carries its file name), so the
 * loader already has it in our dependency tree — that is what makes the
 * engine's class lookups (info_player_start, …) find the game's spawn
 * functions through our handle. Take *that* object: dlopen by file name with
 * RTLD_NOLOAD finds the loaded one instead of loading a second copy (two
 * copies would mean two sets of game globals). If it is not linked in, fall
 * back to the path from gamedll.txt. */
void *
open_game(const char *file_name, const char *path)
{
    void *handle = dlopen(file_name, RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);

    if (handle != nullptr)
    {
        return handle;
    }
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}
} // namespace

bool
gamedll_load(enginefuncs_t *engine_funcs, globalvars_t *globals)
{
    char game_dir[256] = "cstrike";
    char dir[512];
    char name[256];
    char file[128];
    char full[800];
    give_fn_t give;

    /* The engine runs from the HLDS root; the mod dir ("cstrike") is relative
     * to it, and so are the game DLL and the paths inside it. */
    if (g_engfuncs.pfnGetGameDir != nullptr)
    {
        g_engfuncs.pfnGetGameDir(game_dir);
    }
    if (!host_dir(dir, sizeof dir))
    {
        authum_log_error("standalone: cannot find Authum's own folder");
        return false;
    }
    (void)read_gamedll_name(dir, name, sizeof name); /* e.g. "dlls/cs.so" */

    /* The file name (cs.so) is how the loader knows the library; the path is
     * where it lives inside the game directory (dlls/cs.so). */
    {
        const char *slash = std::strrchr(name, '/');
        std::snprintf(file, sizeof file, "%s", slash != nullptr ? slash + 1 : name);
    }
    if (file[0] == '\0')
    {
        authum_log_error("standalone: empty game DLL name in gamedll.txt");
        return false;
    }
    if (name[0] == '/')
    {
        std::snprintf(full, sizeof full, "%s", name); /* absolute: as it is */
    }
    else
    {
        std::snprintf(full, sizeof full, "%s/%s", game_dir, name);
    }

    g_game = open_game(file, full);
    if (g_game == nullptr)
    {
        authum_log_error("standalone: cannot load game DLL %s: %s", full, dlerror());
        return false;
    }
    authum_log_info("standalone: game DLL %s", full);

    give = reinterpret_cast<give_fn_t>(gamedll_symbol("GiveFnptrsToDll"));
    if (give == nullptr)
    {
        authum_log_error("standalone: game DLL has no GiveFnptrsToDll");
        return false;
    }
    give(engine_funcs, globals);
    return true;
}

bool
gamedll_entity_api(DLL_FUNCTIONS *out, int interface_version)
{
    /* The game's *old* GetEntityAPI, on purpose: besides filling the table it
     * sets up the entity hash table (its bucket count). The newer
     * GetEntityAPI2 fills the same table but leaves the hash alone — and the
     * game then divides by zero on the first entity. */
    get_entity_api_t api1 = reinterpret_cast<get_entity_api_t>(gamedll_symbol("GetEntityAPI"));
    get_entity_api2_t api2;

    if (api1 != nullptr)
    {
        return api1(out, interface_version) != 0;
    }
    api2 = reinterpret_cast<get_entity_api2_t>(gamedll_symbol("GetEntityAPI2"));
    if (api2 == nullptr)
    {
        api2 = reinterpret_cast<get_entity_api2_t>(
            gamedll_symbol("_Z13GetEntityAPI2P13DLL_FUNCTIONSPi"));
    }
    if (api2 != nullptr)
    {
        int v = interface_version;
        return api2(out, &v) != 0;
    }
    authum_log_error("standalone: game DLL exports neither GetEntityAPI nor GetEntityAPI2");
    return false;
}

bool
gamedll_new_functions(NEW_DLL_FUNCTIONS *out, int *interface_version)
{
    get_new_dll_funcs_t fn =
        reinterpret_cast<get_new_dll_funcs_t>(gamedll_symbol("GetNewDLLFunctions"));

    return fn != nullptr && fn(out, interface_version) != 0;
}

void *
gamedll_symbol(const char *name)
{
    return g_game != nullptr ? dlsym(g_game, name) : nullptr;
}

} // namespace authum_standalone
