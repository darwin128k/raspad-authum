#include "bridge.h"
#include "data_dir.h"
#include "host_log.h"

#include <extdll.h>
#include <enginecallback.h>

#include "qproto/qproto_bans.h"
#include "qproto/qproto_cfg.h"
#include "qproto/qproto_hook.h"
#include "qproto/qproto_log.h"

#include <cstring>

extern enginefuncs_t g_engfuncs;
extern globalvars_t *gpGlobals;

namespace {

bool
bind_engine_from_host(const authum_host_t *host)
{
	if (!host || !host->engine_funcs || !host->globals)
		return false;

	auto *ef = static_cast<enginefuncs_t *>(host->engine_funcs());
	auto *gv = static_cast<globalvars_t *>(host->globals());
	if (!ef || !gv)
		return false;

	std::memcpy(&g_engfuncs, ef, sizeof(g_engfuncs));
	gpGlobals = gv;
	return true;
}

} // namespace

authum_bool_t
qproto_authum_start(authum_host_t *host)
{
	if (!host)
		return AUTHUM_FALSE;

	if (!bind_engine_from_host(host))
		return AUTHUM_FALSE;

	qproto_host_log_bind(host);
	if (host->module_dir && host->module_dir[0])
		Qproto_SetDataDir(host->module_dir);

	/* Open log first (default LoggingMode); cfg may raise/lower it after. */
	Qproto_Log_Init(g_qprotoConfig.loggingMode);
	Qproto_Cfg_Load();
	if (g_qprotoConfig.loggingMode != 0) {
		Qproto_Log_Close();
		Qproto_Log_Init(g_qprotoConfig.loggingMode);
	}

	Qproto_Log("Authum module start — data dir: %s",
		Qproto_GetDataDir() ? Qproto_GetDataDir() : "(unset)");
	Qproto_Log("qproto 1.0.0 under Authum ABI 0.2 — installing Steam_NotifyClientConnect hook");

	Qproto_Bans_Load();

	if (!Qproto_Hook_Install()) {
		Qproto_Log("Qproto_Hook_Install FAILED");
		Qproto_Log_Close();
		return AUTHUM_FALSE;
	}
	Qproto_Log("hook installed OK");
	return AUTHUM_TRUE;
}

void
qproto_authum_stop(void)
{
	Qproto_Hook_Remove();
	Qproto_Log("Authum module stop — hook removed");
	Qproto_Log_Close();
	qproto_host_log_bind(nullptr);
}
