#include <authum/loader.h>
#include <authum/plugin/info.h>

#include <extdll.h>
#include <dllapi.h>
#include <engine_api.h>
#include <meta_api.h>

#include <lh/char.h>
#include <lh/datetime.h>
#include <lh/index.h>
#include <lh/memory.h>
#include <lh/null.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/version.h>
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

static lh_char_t g_version_text[LH_VERSION_TEXT_MAX + 1];
static lh_char_t g_datetime_text[LH_DATETIME_TEXT_MAX + 1];

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Query(char * /* ifvers */, plugin_info_t **pPlugInfo,
                                             mutil_funcs_t *pMetaUtilFuncs)
{
    lh_usize_t n;

    if (lh_null_eq(lh_ptr_cast(lh_void, pPlugInfo)))
    {
        return FALSE;
    }

    n = lh_version_format(lh_addr_of(authum_module_info.version), g_version_text,
                          LH_VERSION_TEXT_MAX);
    g_version_text[n] = '\0';

    n = lh_datetime_format(lh_addr_of(authum_module_info.datetime), g_datetime_text,
                           LH_DATETIME_TEXT_MAX);
    g_datetime_text[n] = '\0';

    Plugin_info.ifvers = const_cast<char *>(META_INTERFACE_VERSION);
    Plugin_info.name = const_cast<char *>(authum_module_info.name);
    Plugin_info.version = g_version_text;
    Plugin_info.date = g_datetime_text;
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
        return FALSE;
    }
    (void)authum_loader_load_modules(&authum);
    {
        lh_usize_t loaded;
        lh_char_t version_text[LH_VERSION_TEXT_MAX + 1];
        authum_module_t *child;

        loaded = authum_loader_get_loaded(&authum);
        for (child = authum_loader_get_first(&authum); child != nullptr;
             child = authum_loader_get_next(&authum, child))
        {
            const authum_module_info_t *info;
            lh_usize_t n;

            info = authum_module_get_info(child);
            if (lh_null_eq(info))
            {
                continue;
            }
            n = lh_version_format(lh_addr_of(info->version), version_text, LH_VERSION_TEXT_MAX);
            version_text[n] = '\0';
            LOG_MESSAGE(PLID, "loaded %s %s", info->name, version_text);
        }
        LOG_MESSAGE(PLID, "modules loaded: %u", static_cast<unsigned>(loaded));
    }
    return TRUE;
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Detach(PLUG_LOADTIME /* now */, PL_UNLOAD_REASON /* reason */)
{
    authum_loader_deinit();
    return TRUE;
}
