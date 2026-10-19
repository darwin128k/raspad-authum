#include "data_dir.h"

#include <cstring>

namespace {
char g_dataDir[512];
}

void
Qproto_SetDataDir(const char *dir)
{
	g_dataDir[0] = '\0';
	if (!dir || !dir[0])
		return;
	strncpy(g_dataDir, dir, sizeof(g_dataDir) - 1);
	g_dataDir[sizeof(g_dataDir) - 1] = '\0';
	size_t n = strlen(g_dataDir);
	while (n > 1 && (g_dataDir[n - 1] == '/' || g_dataDir[n - 1] == '\\')) {
		g_dataDir[--n] = '\0';
	}
}

const char *
Qproto_GetDataDir(void)
{
	return g_dataDir[0] ? g_dataDir : nullptr;
}
