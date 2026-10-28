#define _GNU_SOURCE
#include <authum/host/placeholders.h>

#include "check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static authum_char_t game_dir[512];
static authum_char_t module_dir[512];

static void
setup(void)
{
    authum_char_t base[384];
    CHECK(getcwd(base, sizeof base) != NULL);
    snprintf(game_dir, sizeof game_dir, "%s/authum-placeholders-game", base);
    snprintf(module_dir, sizeof module_dir, "%s/authum-placeholders-module", base);
#if defined(_WIN32)
    mkdir(game_dir);
    mkdir(module_dir);
#else
    mkdir(game_dir, 0755);
    mkdir(module_dir, 0755);
#endif
    CHECK(authum_placeholder_canonical(game_dir, game_dir, sizeof game_dir));
    CHECK(authum_placeholder_canonical(module_dir, module_dir, sizeof module_dir));
}

static void
teardown(void)
{
    rmdir(game_dir);
    rmdir(module_dir);
}

int
main(void)
{
    authum_placeholder_owner_t owner;
    authum_placeholder_service_t service;
    authum_placeholder_owner_t other;
    authum_placeholder_service_t other_service;
    const authum_char_t *standard[authum_placeholder_standard_count] = {0};
    authum_char_t out[512];

    setup();
    /* Canonical paths are strict in LH; Authum retains unresolved names. */
    authum_char_t dotted[640];
    snprintf(dotted, sizeof dotted, "%s/.", game_dir);
    CHECK(authum_placeholder_canonical(dotted, out, sizeof out));
    CHECK_STR(out, game_dir);
    const authum_char_t *missing = "authum-placeholders-missing/../future";
    CHECK(authum_placeholder_canonical(missing, out, sizeof out));
    CHECK_STR(out, missing);
    CHECK(authum_placeholder_canonical(out, out, sizeof out));
    CHECK_STR(out, missing);
    strcpy(out, "sentinel");
    CHECK(!authum_placeholder_canonical(game_dir, out, 2));
    CHECK_STR(out, "sentinel");
    CHECK(!authum_placeholder_canonical(missing, out, 2));
    CHECK_STR(out, "sentinel");
    CHECK(!authum_placeholder_canonical(NULL, out, sizeof out));
    CHECK(!authum_placeholder_canonical(game_dir, NULL, sizeof out));
    CHECK(!authum_placeholder_canonical(game_dir, out, 0));

    standard[authum_placeholder_game] = game_dir;
    standard[authum_placeholder_module] = module_dir;
    authum_placeholder_host_init(&owner, standard, &service);

    /* Standard names resolve against the calling module. */
    CHECK(authum_placeholder_expand(&service, "{game}", out, sizeof out));
    CHECK_STR(out, game_dir);
    CHECK(authum_placeholder_expand(&service, "{module}", out, sizeof out));
    CHECK_STR(out, module_dir);
    CHECK(authum_placeholder_expand(&service, "{game}/maps", out, sizeof out));
    CHECK(strstr(out, "/maps") != 0);

    /* A relative template belongs to the calling module. */
    CHECK(authum_placeholder_expand(&service, "maps", out, sizeof out));
    CHECK(strstr(out, module_dir) != 0);

    /* Unknown names, stray braces and empty templates fail. */
    CHECK(!authum_placeholder_expand(&service, "{unknown}", out, sizeof out));
    CHECK(!authum_placeholder_expand(&service, "{game", out, sizeof out));
    CHECK(!authum_placeholder_expand(&service, "game}", out, sizeof out));
    CHECK(!authum_placeholder_expand(&service, "", out, sizeof out));
    CHECK(!authum_placeholder_expand(&service, "{game}", out, 4));

    /* A module publishes a name; every module may use it. */
    CHECK(authum_placeholder_register(&service, "shared", "/tmp/authum-placeholders-shared"));
    CHECK(authum_placeholder_expand(&service, "{shared}", out, sizeof out));
    CHECK(strstr(out, "authum-placeholders-shared") != 0);

    /* Standard names are reserved, and a taken name is not stolen. */
    CHECK(!authum_placeholder_register(&service, "game", "/tmp"));
    CHECK(!authum_placeholder_register(&service, "shared", "/tmp"));

    /* A second module sees the published name, but not the other's scope. */
    authum_placeholder_host_init(&other, standard, &other_service);
    CHECK(authum_placeholder_expand(&other_service, "{shared}", out, sizeof out));
    CHECK(strstr(out, "authum-placeholders-shared") != 0);

    /* Only the owner may remove a name. */
    CHECK(!authum_placeholder_unregister(&other_service, "shared"));
    CHECK(authum_placeholder_unregister(&service, "shared"));
    CHECK(!authum_placeholder_expand(&service, "{shared}", out, sizeof out));

    /* Unloading a module drops the names it published. */
    CHECK(authum_placeholder_register(&service, "gone", "/tmp/authum-placeholders-gone"));
    CHECK(authum_placeholder_expand(&other_service, "{gone}", out, sizeof out));
    authum_placeholder_host_drop_owner(&owner);
    CHECK(!authum_placeholder_expand(&other_service, "{gone}", out, sizeof out));

    teardown();
    return authum_check_report("placeholders");
}
