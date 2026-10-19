#include <cmath>
#include <extdll.h>
#include <enginecallback.h>

#include "data_dir.h"
#include "qproto/qproto_paths.h"

#include <cstdio>
#include <cstring>

FILE *
Qproto_OpenDataFile(const char *name, char *outPath, size_t outPathLen)
{
	if (!name || !outPath || outPathLen == 0)
		return nullptr;

	const char *modDir = Qproto_GetDataDir();
	if (modDir) {
		snprintf(outPath, outPathLen, "%s/%s", modDir, name);
		FILE *f = fopen(outPath, "r");
		if (f)
			return f;
	}

	char gameDir[256] = {};
	(*g_engfuncs.pfnGetGameDir)(gameDir);

	const char *patterns[] = {
		"%s/addons/authum/modules/qproto/%s",
		"%s/addons/qproto/%s",
		"%s/%s",
		"%s/../%s",
	};

	for (const char *pattern : patterns) {
		snprintf(outPath, outPathLen, pattern, gameDir, name);
		FILE *f = fopen(outPath, "r");
		if (f)
			return f;
	}

	if (modDir)
		snprintf(outPath, outPathLen, "%s/%s", modDir, name);
	else
		snprintf(outPath, outPathLen, "%s/addons/authum/modules/qproto/%s", gameDir, name);
	return nullptr;
}
