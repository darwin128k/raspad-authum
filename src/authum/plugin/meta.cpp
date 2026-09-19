#include <authum/loader.h>
#include <authum/plugin/info.h>

#include <extdll.h>
#include <dllapi.h>
#include <engine_api.h>
#include <meta_api.h>

#include <lh/date.h>
#include <lh/datetime.h>
#include <lh/util/addr.h>
#include <lh/version.h>

#include <string.h>

static META_FUNCTIONS g_meta_functions = {
    NULL,
    NULL,
    GetEntityAPI2,
    NULL,
    NULL,
    NULL,
    GetEngineFunctions,
    NULL,
};

plugin_info_t Plugin_info;
meta_globals_t *gpMetaGlobals;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;

static char g_version_text[LH_VERSION_TEXT_MAX + 1];
static char g_date_text[LH_DATE_TEXT_MAX + 1];

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Query(char * /* ifvers */, plugin_info_t **pPlugInfo,
                                             mutil_funcs_t *pMetaUtilFuncs)
{
    lh_usize_t n;

    if (!pPlugInfo)
    {
        return FALSE;
    }

    n = lh_version_format(lh_addr_of(authum_plugin_info.version), g_version_text,
                          LH_VERSION_TEXT_MAX);
    g_version_text[n] = '\0';

    {
        const lh_date_t date = lh_datetime_get_date(lh_addr_of(authum_plugin_info.datetime));
        n = lh_date_format(lh_addr_of(date), g_date_text, LH_DATE_TEXT_MAX);
        g_date_text[n] = '\0';
    }

    Plugin_info.ifvers = const_cast<char *>(META_INTERFACE_VERSION);
    Plugin_info.name = const_cast<char *>(authum_plugin_info.name);
    Plugin_info.version = g_version_text;
    Plugin_info.date = g_date_text;
    Plugin_info.author = const_cast<char *>(authum_plugin_info.author);
    Plugin_info.url = const_cast<char *>(authum_plugin_info.url);
    Plugin_info.logtag = const_cast<char *>(authum_plugin_info.logtag);
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
    if (!pMGlobals || !pFunctionTable)
    {
        return FALSE;
    }
    gpMetaGlobals = pMGlobals;
    memcpy(pFunctionTable, &g_meta_functions, sizeof(META_FUNCTIONS));
    gpGamedllFuncs = pGamedllFuncs;
    authum_loader_init();
    return TRUE;
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Detach(PLUG_LOADTIME /* now */, PL_UNLOAD_REASON /* reason */)
{
    authum_loader_deinit();
    return TRUE;
}
