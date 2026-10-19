#include <authum/gate.h>
#include <authum/loader.h>
#include <authum/log.h>
#include <authum/plugin/info.h>

#include <extdll.h>
#include <dllapi.h>
#include <engine_api.h>
#include <enginecallbacks.h>
#include <meta_api.h>

#include <cstdio>

#include <lh/char.h>
#include <lh/fs/path.h>
#include <lh/index.h>
#include <lh/memory.h>
#include <lh/null.h>
#include <lh/os/module.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/void.h>

static META_FUNCTIONS g_meta_functions = {
    nullptr,
    nullptr,
    GetEntityAPI2,
    nullptr,
    nullptr,
    nullptr,
    GetEngineFunctions,
    nullptr,
};

plugin_info_t Plugin_info;
meta_globals_t *gpMetaGlobals;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;

static char g_version_text[32];

static void
authum_format_version(const authum_version_t *v, char *buf, unsigned buf_size)
{
    std::snprintf(buf, buf_size, "%u.%u.%u", v->major, v->minor, v->patch);
}

static void
authum_cmd_list(void)
{
    authum_module_t *child;
    lh_usize_t loaded;
    char version_text[32];

    loaded = authum_loader_get_loaded(&authum);
    authum_log_info("=== Authum modules (%u) ===", static_cast<unsigned>(loaded));
    for (child = authum_loader_get_first(&authum); child != nullptr;
         child = authum_loader_get_next(&authum, child))
    {
        const authum_module_info_t *info;

        info = authum_module_get_info(child);
        if (lh_null_eq(info))
        {
            authum_log_info("  (unnamed module)");
            continue;
        }
        authum_format_version(&info->version, version_text, sizeof(version_text));
        authum_log_info("  OK  %s %s", info->name, version_text);
    }
    if (loaded == 0)
    {
        authum_log_info("  (none — check modules.list beside authum_mm.so)");
    }
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Query(char * /* ifvers */, plugin_info_t **pPlugInfo,
                                             mutil_funcs_t *pMetaUtilFuncs)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pPlugInfo)))
    {
        return FALSE;
    }

    authum_format_version(&authum_module_info.version, g_version_text, sizeof(g_version_text));

    Plugin_info.ifvers = const_cast<char *>(META_INTERFACE_VERSION);
    Plugin_info.name = const_cast<char *>(authum_module_info.name);
    Plugin_info.version = g_version_text;
    Plugin_info.date = const_cast<char *>(authum_module_info.date != nullptr
                                              ? authum_module_info.date
                                              : "");
    Plugin_info.author = const_cast<char *>(authum_module_info.author);
    Plugin_info.url = const_cast<char *>(authum_module_info.url);
    Plugin_info.logtag = const_cast<char *>(authum_module_info.logtag);
    Plugin_info.loadable = PT_ANYTIME;
    Plugin_info.unloadable = PT_ANYPAUSE;

    *pPlugInfo = &Plugin_info;
    gpMetaUtilFuncs = pMetaUtilFuncs;
    return TRUE;
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Attach(PLUG_LOADTIME /* now */, META_FUNCTIONS *pFunctionTable,
                                              meta_globals_t *pMGlobals,
                                              gamedll_funcs_t *pGamedllFuncs)
{
    if (lh_null_eq(lh_ptr_cast(lh_void, pMGlobals)) ||
        lh_null_eq(lh_ptr_cast(lh_void, pFunctionTable)))
    {
        return FALSE;
    }
    gpMetaGlobals = pMGlobals;
    lh_memory_copy(pFunctionTable, sizeof(META_FUNCTIONS), lh_addr_of(g_meta_functions),
                   sizeof(META_FUNCTIONS));
    gpGamedllFuncs = pGamedllFuncs;

    if (authum_loader_init() != lh_bool_true)
    {
        authum_log_error("loader init failed");
        return FALSE;
    }
    {
        const lh_fs_path_t *image;
        lh_fs_path_t parent;

        image = lh_os_module_get_path_as_const(&authum);
        lh_fs_path_init(lh_addr_of(parent));
        if (lh_null_ne(image) && lh_fs_path_parent(lh_addr_of(parent), image))
        {
            (void)authum_log_init(lh_addr_of(parent));
        }
        lh_fs_path_deinit(lh_addr_of(parent));
    }

    authum_log_info("Authum %s attaching", g_version_text);

    (void)authum_loader_load_modules(&authum);
    authum_cmd_list();

    if (authum_gate_init() != lh_bool_true)
    {
        authum_log_error("connect gate init failed");
        authum_loader_deinit();
        authum_log_deinit();
        return FALSE;
    }

    REG_SVR_COMMAND("authum", authum_cmd_list);
    authum_log_info("server command: authum  (list modules)");
    return TRUE;
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Detach(PLUG_LOADTIME /* now */, PL_UNLOAD_REASON /* reason */)
{
    authum_gate_deinit();
    authum_loader_deinit();
    authum_log_deinit();
    return TRUE;
}
