/**
 * @file logs.c
 * @brief Host log channels: a file per module and day, levels from authum.cfg,
 *        warnings copied to the authum channel, console mirror.
 */

#include <check.h>

#include <authum/cstr.h>
#include <authum/host.h>
#include <authum/logger.h>
#include <authum/logger/host.h>
#include <authum/path.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#define TEST_ROOT "authum_logs_test_root"

/* Noon, so the next day is the next calendar day in any time zone. */
#define TEST_DAY_ONE ((time_t)1792929600)
#define TEST_DAY_TWO (TEST_DAY_ONE + (time_t)86400)

time_t g_test_now = TEST_DAY_ONE;
int g_test_mirrored;
authum_char_t g_test_mirror_name[64];
authum_char_t g_test_mirror_text[256];

/* Owners are just distinct addresses. */
int g_test_qproto;
int g_test_quiet;
int g_test_hello;

time_t
test_clock(void)
{
    return g_test_now;
}

void
test_mirror(authum_logger_level_t level, const authum_char_t *name, const authum_char_t *text)
{
    (void)level;
    ++g_test_mirrored;
    authum_cstr_copy(g_test_mirror_name, sizeof g_test_mirror_name, name);
    authum_cstr_copy(g_test_mirror_text, sizeof g_test_mirror_text, text);
}

/** `<root>/logs/<name>/YYYY-MM-DD.log` for @p when. */
void
test_log_path(authum_char_t *out, authum_usize_t out_size, const char *name, time_t when)
{
    char day[16];
    struct tm *tm = localtime(&when);

    strftime(day, sizeof day, "%Y-%m-%d", tm);
    snprintf(out, out_size, TEST_ROOT "/logs/%s/%s.log", name, day);
}

authum_bool_t
test_file_contains(const char *name, time_t when, const char *needle)
{
    authum_char_t path[256];
    authum_char_t line[512];
    authum_bool_t found = authum_bool_false;
    FILE *file;

    test_log_path(path, sizeof path, name, when);
    file = fopen(path, "r");
    if (file == NULL)
    {
        return authum_bool_false;
    }
    while (!found && fgets(line, sizeof line, file) != NULL)
    {
        found = strstr(line, needle) != NULL ? authum_bool_true : authum_bool_false;
    }
    fclose(file);
    return found;
}

authum_bool_t
test_file_exists(const char *name, time_t when)
{
    authum_char_t path[256];
    FILE *file;

    test_log_path(path, sizeof path, name, when);
    file = fopen(path, "r");
    if (file == NULL)
    {
        return authum_bool_false;
    }
    fclose(file);
    return authum_bool_true;
}

/** Start from nothing: no files from an earlier run. */
void
test_reset(void)
{
    const char *names[] = {"authum", "qproto", "quiet", "hello"};
    authum_char_t path[256];
    authum_usize_t i;
    authum_path_t dir;
    FILE *cfg;

    for (i = 0u; i < sizeof names / sizeof names[0]; ++i)
    {
        test_log_path(path, sizeof path, names[i], TEST_DAY_ONE);
        remove(path);
        test_log_path(path, sizeof path, names[i], TEST_DAY_TWO);
        remove(path);
    }
    authum_path_set_cstr(&dir, TEST_ROOT);
    CHECK(authum_path_ensure_dir(&dir));
    cfg = fopen(TEST_ROOT "/authum.cfg", "w");
    CHECK(cfg != NULL);
    if (cfg != NULL)
    {
        fputs("# test config\n"
              "log_level = info\n"
              "log_console = error\n"
              "qproto.log_level = debug\n"
              "quiet.log_level = off\n",
              cfg);
        fclose(cfg);
    }
}

void
test_routing(void)
{
    CHECK(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_qproto, "qproto"));
    CHECK(authum_logs_open(&g_test_quiet, "quiet"));
    CHECK(authum_logs_open(&g_test_hello, "hello"));

    /* Per-module level from the cfg. */
    CHECK(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug));
    CHECK(!authum_logs_is_enabled(&g_test_hello, authum_logger_level_debug));
    CHECK(authum_logs_is_enabled(&g_test_hello, authum_logger_level_info));

    authum_logs_write(&g_test_qproto, authum_logger_level_debug, "qproto debug line");
    CHECK(test_file_contains("qproto", TEST_DAY_ONE, "[debug] qproto debug line"));
    CHECK(!test_file_contains("authum", TEST_DAY_ONE, "qproto debug line"));

    authum_logs_write(&g_test_hello, authum_logger_level_debug, "hello debug line");
    CHECK(!test_file_exists("hello", TEST_DAY_ONE));

    /* A module's warning: its own file and the authum file, not the console. */
    authum_logs_write(&g_test_hello, authum_logger_level_warning, "hello warning line");
    CHECK(test_file_contains("hello", TEST_DAY_ONE, "[warning] hello warning line"));
    CHECK(test_file_contains("authum", TEST_DAY_ONE, "[warning] [hello] hello warning line"));
    CHECK(g_test_mirrored == 0);

    /* `off` keeps the module's own file away, but its errors still reach
     * authum.log and the console. */
    authum_logs_write(&g_test_quiet, authum_logger_level_error, "quiet error line");
    CHECK(!test_file_exists("quiet", TEST_DAY_ONE));
    CHECK(test_file_contains("authum", TEST_DAY_ONE, "[error] [quiet] quiet error line"));
    CHECK(g_test_mirrored == 1);
    CHECK_STR(g_test_mirror_name, "quiet");
    CHECK_STR(g_test_mirror_text, "quiet error line");

    /* The host's own lines (owner null). */
    authum_logs_write(authum_null, authum_logger_level_info, "host info line");
    CHECK(test_file_contains("authum", TEST_DAY_ONE, "[info] host info line"));

    /* Next day, next file. */
    g_test_now = TEST_DAY_TWO;
    authum_logs_write(&g_test_qproto, authum_logger_level_info, "qproto day two");
    CHECK(test_file_contains("qproto", TEST_DAY_TWO, "qproto day two"));
    CHECK(!test_file_contains("qproto", TEST_DAY_ONE, "qproto day two"));
    g_test_now = TEST_DAY_ONE;

    /* A closed channel takes nothing. */
    authum_logs_close(&g_test_hello);
    CHECK(!authum_logs_is_enabled(&g_test_hello, authum_logger_level_error));
    authum_logs_write(&g_test_hello, authum_logger_level_error, "after close");
    CHECK(!test_file_contains("hello", TEST_DAY_ONE, "after close"));
    CHECK(!test_file_contains("authum", TEST_DAY_ONE, "after close"));

    authum_logs_deinit();
}

/* What a module does: authum_logger_* through the host's channel. */
void
test_module_logger(void)
{
    authum_logger_channel_t channel;
    authum_host_t host;
    authum_logger_t log;

    CHECK(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_hello, "hello"));

    channel.ops = authum_logs_ops();
    channel.owner = &g_test_hello;
    memset(&host, 0, sizeof host);
    host.size = sizeof host;
    host.logger = &channel;

    authum_logger_init(&log, &host);
    authum_logger_info(&log, "seen %d players", 3);
    CHECK(test_file_contains("hello", TEST_DAY_ONE, "[info] seen 3 players"));
    CHECK(!authum_logger_is_enabled(&log, authum_logger_level_debug));
    authum_host_log_line(&host, 1, "via host_log_line");
    CHECK(test_file_contains("hello", TEST_DAY_ONE, "[error] via host_log_line"));
    authum_logger_close(&log);
    authum_logger_info(&log, "after close");
    CHECK(!test_file_contains("hello", TEST_DAY_ONE, "after close"));

    authum_logs_deinit();
}

void
test_level_names(void)
{
    authum_logger_level_t level = authum_logger_level_info;

    CHECK(authum_logger_level_parse("debug", &level) && level == authum_logger_level_debug);
    CHECK(authum_logger_level_parse("WARN", &level) && level == authum_logger_level_warning);
    CHECK(!authum_logger_level_parse("loud", &level));
    CHECK_STR(authum_logger_level_name(authum_logger_level_critical), "critical");
    CHECK_STR(authum_logger_level_name(42), "?");
}

int
main(void)
{
    test_reset();
    test_level_names();
    test_routing();
    test_module_logger();
    return authum_check_report("logs");
}
