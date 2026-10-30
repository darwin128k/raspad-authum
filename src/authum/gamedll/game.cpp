#include <authum/gamedll/game.h>
#include <authum/unused.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>

#include <authum/cstr.h>
#include <authum/host/log.h>
#include <authum/host/module.h>


#include <cstdio>
#include <cstdlib>
#include <cstring>

extern enginefuncs_t g_engfuncs;

namespace authum_gamedll
{
namespace
{
/* The game's GiveFnptrsToDll is __stdcall on Windows (HLSDK's DLLEXPORT): it
 * pops its own arguments, so calling it as cdecl unbalances our stack. */
#if defined(_WIN32)
typedef void(__stdcall *give_fn_t)(enginefuncs_t *, globalvars_t *);
#else
typedef void (*give_fn_t)(enginefuncs_t *, globalvars_t *);
#endif
typedef authum_i32_t (*get_entity_api_t)(DLL_FUNCTIONS *, authum_i32_t); /* old: version by value */
typedef authum_i32_t (*get_entity_api2_t)(DLL_FUNCTIONS *, authum_i32_t *);
typedef authum_i32_t (*get_new_dll_funcs_t)(NEW_DLL_FUNCTIONS *, authum_i32_t *);

/* The real game library, open through lh's OS layer (dlopen / LoadLibrary). */
lh_os_shared_t g_game;
authum_bool_t g_game_open = false;

/* The real game DLL is named in Authum's own file, next to authum.so.
 *
 * The engine reads the game library path from `gamedll_linux` in liblist.gam;
 * that line now points at authum.so, so the original path ("dlls/cs.so") is
 * kept beside the host image — addons/authum/gamedll.txt. A relative path is
 * resolved against the game dir, the way liblist.gam resolves it; an
 * absolute one is taken as it is. $AUTHUM_GAMEDLL overrides it (tests).
 */
const authum_char_t authum_gamedll_file[] = "gamedll.txt";
const authum_char_t authum_gamedll_default[] = "dlls/cs.so";

/* Trim the line in place: no comment, no surrounding spaces. */
void
trim(authum_char_t *line)
{
    authum_char_t *start = line;
    authum_char_t *end;
    authum_char_t *comment;

    comment = std::strchr(line, lh_char_map_hash);
    if (lh_math_ne(comment, authum_null))
    {
        lh_ptr_deref(comment) = lh_char_map_nul;
    }
    while (lh_math_eq(lh_ptr_deref(start), lh_char_map_space) ||
           lh_math_eq(lh_ptr_deref(start), lh_char_map_ht))
    {
        start = lh_math_add_one(start);
    }
    if (lh_math_ne(start, line))
    {
        lh_memory_std_move(line, start, lh_math_add(std::strlen(start), 1u));
    }
    end = lh_math_add(line, std::strlen(line));
    while (lh_math_gt(end, line) &&
           (lh_math_eq(end[-1], lh_char_map_lf) || lh_math_eq(end[-1], lh_char_map_cr) ||
            lh_math_eq(end[-1], lh_char_map_space) || lh_math_eq(end[-1], lh_char_map_ht)))
    {
        end = lh_math_sub_one(end);
        lh_ptr_deref(end) = lh_char_map_nul;
    }
}

/* Authum's own folder — the parent of the host image (addons/authum). */
authum_bool_t
host_dir(authum_char_t *out, authum_usize_t out_size)
{
    const lh_fs_path_t *image;
    lh_fs_path_t parent;
    lh_str_t scratch;
    const lh_str_ptr text;
    authum_bool_t ok;

    image = lh_os_module_get_path(lh_addr_of(authum));
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
            authum_cstr_copy(out, out_size, dir);
        }
    }
    lh_str_deinit(lh_addr_of(scratch));
    lh_fs_path_deinit(lh_addr_of(parent));
    return ok;
}

/* The game DLL path as written in gamedll.txt (or $AUTHUM_GAMEDLL). */
authum_bool_t
read_gamedll_name(const authum_char_t *dir, authum_char_t *out, authum_usize_t out_size)
{
    const authum_char_t *env = std::getenv("AUTHUM_GAMEDLL");
    authum_char_t path[600];
    authum_char_t line[256];
    FILE *f;

    if (lh_math_ne(env, authum_null) && lh_math_ne(env[0], lh_char_map_nul))
    {
        authum_cstr_copy(out, out_size, env);
        return true;
    }
    lh_str_ptr_format_cstr(path, sizeof path, "%s/%s", dir, authum_gamedll_file);
    f = std::fopen(path, "r");
    if (lh_math_eq(f, authum_null))
    {
        authum_log_error("gamedll: no %s beside authum.so (looked at %s)", authum_gamedll_file,
                         path);
        authum_cstr_copy(out, out_size, authum_gamedll_default);
        return false;
    }
    while (lh_math_ne(std::fgets(line, sizeof line, f), authum_null))
    {
        trim(line);
        if (lh_math_eq(line[0], lh_char_map_nul))
        {
            continue;
        }
        authum_cstr_copy(out, out_size, line);
        std::fclose(f);
        return true;
    }
    std::fclose(f);
    authum_log_error("gamedll: %s is empty", path);
    authum_cstr_copy(out, out_size, authum_gamedll_default);
    return false;
}

/* Open the real game library, through lh's OS layer — the same call on both
 * platforms (dlopen / LoadLibrary).
 *
 * The engine looks an entity's spawn function up by class name
 * (info_player_start, …) *in the game module* — the one it loaded, which is
 * us. So the class names have to be found there: we export them ourselves
 * (src/authum/gamedll/game_classes.cpp) and each trampoline forwards to the game.
 */
authum_bool_t
open_game(const authum_char_t *path)
{
    lh_fs_path_t image;
    lh_str_t text;
    lh_str_view_t view;
    lh_bool_t ok;

    lh_os_shared_init(lh_addr_of(g_game));
    lh_fs_path_init(lh_addr_of(image));
    lh_str_init(lh_addr_of(text));
    lh_str_append(lh_addr_of(text), path, std::strlen(path));
    lh_str_as_view(lh_addr_of(text), lh_addr_of(view));
    lh_fs_path_set(lh_addr_of(image), view, lh_fs_path_style_posix);
    ok = lh_os_shared_open(lh_addr_of(g_game), lh_addr_of(image));
    if (ok)
    {
        g_game_open = true;
    }
    lh_str_deinit(lh_addr_of(text));
    lh_fs_path_deinit(lh_addr_of(image));
    return lh_math_eq(ok, lh_bool_true);
}
} // namespace

authum_bool_t
gamedll_load(enginefuncs_t *engine_funcs, globalvars_t *globals)
{
    authum_char_t game_dir[256] = "cstrike";
    authum_char_t dir[512];
    authum_char_t name[256];
    authum_char_t full[800];
    give_fn_t give;

    /* The engine runs from the HLDS root; the mod dir ("cstrike") is relative
     * to it, and so are the game DLL and the paths inside it. */
    if (lh_math_ne(g_engfuncs.pfnGetGameDir, authum_null))
    {
        g_engfuncs.pfnGetGameDir(game_dir);
    }
    if (!host_dir(dir, sizeof dir))
    {
        authum_log_error("gamedll: cannot find Authum's own folder");
        return false;
    }
    authum_unused(read_gamedll_name(dir, name, sizeof name)); /* e.g. "dlls/cs.so" */

    /* The path is relative to the game dir, the way liblist.gam resolves it. */
    if (lh_math_eq(name[0], lh_char_map_slash))
    {
        authum_cstr_copy(full, sizeof full, name); /* absolute: as it is */
    }
    else
    {
        lh_str_ptr_format_cstr(full, sizeof full, "%s/%s", game_dir, name);
    }

    if (!open_game(full))
    {
        authum_log_error("gamedll: cannot load game DLL %s", full);
        return false;
    }
    authum_log_info("gamedll: game DLL %s", full);

    give = lh_cast_reinterpret(give_fn_t, gamedll_symbol("GiveFnptrsToDll"));
    if (lh_math_eq(give, authum_null))
    {
        authum_log_error("gamedll: game DLL has no GiveFnptrsToDll");
        return false;
    }
    give(engine_funcs, globals);
    return true;
}

authum_bool_t
gamedll_entity_api(DLL_FUNCTIONS *out, authum_i32_t interface_version)
{
    /* The game's *old* GetEntityAPI, on purpose: besides filling the table it
     * sets up the entity hash table (its bucket count). The newer
     * GetEntityAPI2 fills the same table but leaves the hash alone — and the
     * game then divides by zero on the first entity. */
    get_entity_api_t api1 = lh_cast_reinterpret(get_entity_api_t, gamedll_symbol("GetEntityAPI"));
    get_entity_api2_t api2;

    if (lh_math_ne(api1, authum_null))
    {
        return lh_math_ne(api1(out, interface_version), 0);
    }
    api2 = lh_cast_reinterpret(get_entity_api2_t, gamedll_symbol("GetEntityAPI2"));
    if (lh_math_eq(api2, authum_null))
    {
        api2 = lh_cast_reinterpret(get_entity_api2_t,
                                   gamedll_symbol("_Z13GetEntityAPI2P13DLL_FUNCTIONSPi"));
    }
    if (lh_math_ne(api2, authum_null))
    {
        authum_i32_t v = interface_version;
        return lh_math_ne(api2(out, lh_addr_of(v)), 0);
    }
    authum_log_error("gamedll: game DLL exports neither GetEntityAPI nor GetEntityAPI2");
    return false;
}

authum_bool_t
gamedll_new_functions(NEW_DLL_FUNCTIONS *out, authum_i32_t *interface_version)
{
    get_new_dll_funcs_t fn =
        lh_cast_reinterpret(get_new_dll_funcs_t, gamedll_symbol("GetNewDLLFunctions"));

    return lh_math_ne(fn, authum_null) && lh_math_ne(fn(out, interface_version), 0);
}

authum_ptr
gamedll_symbol(const authum_char_t *name)
{
    return g_game_open ? lh_os_shared_get_sym(lh_addr_of(g_game), name) : authum_null;
}

} // namespace authum_gamedll
