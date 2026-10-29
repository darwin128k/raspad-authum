/**
 * @file logs.c
 * @brief Host log channels: a file per module and day, levels from authum.cfg,
 *        warnings copied to the authum channel, console mirror, journals,
 *        buffering, old files pruned, cfg reload.
 */

#include <check.h>

#include <authum/cstr.h>
#include <authum/host.h>
#include <authum/host/clock.h>
#include <authum/logger.h>
#include <authum/host/logs.h>
#include <authum/path.h>
#include <authum/unused.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
#    include <windows.h>
#endif

#define TEST_ROOT "authum_logs_test_root"

/* Noon, so the next day is the next calendar day in any time zone. */
#define TEST_DAY ((time_t)86400)
#define TEST_DAY_ONE ((time_t)1792929600)
#define TEST_DAY_TWO (TEST_DAY_ONE + TEST_DAY)

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
    authum_unused(level);
    ++g_test_mirrored;
    authum_cstr_copy(g_test_mirror_name, sizeof g_test_mirror_name, name);
    authum_cstr_copy(g_test_mirror_text, sizeof g_test_mirror_text, text);
}

/**
 * The daily file of channel @p dir for @p when: `authum` is the host
 * (`<root>/logs/`), `qproto` a module (`<root>/modules/qproto/logs/`),
 * `qproto/bans` a journal (`<root>/modules/qproto/logs/bans/`).
 */
void
test_log_path(authum_char_t *out, authum_usize_t out_size, const char *dir, time_t when)
{
    char day[16];
    char module[64];
    const char *slash = strchr(dir, '/');
    struct tm *tm = localtime(&when);

    strftime(day, sizeof day, "%Y-%m-%d", tm);
    if (strcmp(dir, "authum") == 0)
    {
        snprintf(out, out_size, TEST_ROOT "/logs/%s.log", day);
    }
    else if (slash != NULL)
    {
        snprintf(module, sizeof module, "%.*s", (int)(slash - dir), dir);
        snprintf(out, out_size, TEST_ROOT "/modules/%s/logs/%s/%s.log", module, slash + 1, day);
    }
    else
    {
        snprintf(out, out_size, TEST_ROOT "/modules/%s/logs/%s.log", dir, day);
    }
}

/** Whether @p path is on disk now (without flushing anything). */
authum_bool_t
test_path_contains(const char *path, const char *needle)
{
    authum_char_t line[512];
    authum_bool_t found = authum_bool_false;
    FILE *file = fopen(path, "r");

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
test_on_disk(const char *dir, time_t when, const char *needle)
{
    authum_char_t path[256];

    test_log_path(path, sizeof path, dir, when);
    return test_path_contains(path, needle);
}

/** What a reader sees once the host has flushed (it does so every frame). */
authum_bool_t
test_file_contains(const char *dir, time_t when, const char *needle)
{
    CHECK(authum_logs_sync(5000));
    return test_on_disk(dir, when, needle);
}

authum_bool_t
test_exists(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL)
    {
        return authum_bool_false;
    }
    fclose(file);
    return authum_bool_true;
}

authum_bool_t
test_file_exists(const char *dir, time_t when)
{
    authum_char_t path[256];

    CHECK(authum_logs_sync(5000));
    test_log_path(path, sizeof path, dir, when);
    return test_exists(path);
}

void
test_write_cfg(const char *text)
{
    FILE *cfg = fopen(TEST_ROOT "/configs/authum.cfg", "w");

    CHECK(cfg != NULL);
    if (cfg != NULL)
    {
        fputs(text, cfg);
        fclose(cfg);
    }
}

void
test_touch(const char *path)
{
    FILE *file = fopen(path, "w");

    if (file != NULL)
    {
        fclose(file);
    }
}

void
test_mkdir(const char *text)
{
    authum_path_t dir;

    authum_path_set_cstr(&dir, text);
    CHECK(authum_path_ensure_dir(&dir));
}

const char g_test_cfg[] = "# test config\n"
                          "log_level = info\n"
                          "log_console = error\n"
                          "log_keep_days = 30\n"
                          "qproto.log_level = debug\n"
                          "quiet.log_level = off\n"
                          "hello.audit.log_level = debug\n";

/** Start from nothing: no files from an earlier run. */
void
test_reset(void)
{
    const char *dirs[] = {"authum", "qproto", "quiet", "hello", "hello/audit", "qproto/bans"};
    authum_char_t path[256];
    authum_usize_t i;

    for (i = 0u; i < sizeof dirs / sizeof dirs[0]; ++i)
    {
        test_log_path(path, sizeof path, dirs[i], TEST_DAY_ONE);
        remove(path);
        test_log_path(path, sizeof path, dirs[i], TEST_DAY_TWO);
        remove(path);
    }
    test_mkdir(TEST_ROOT);
    test_mkdir(TEST_ROOT "/configs");
    test_write_cfg(g_test_cfg);
}

void
test_routing(void)
{
    CHECK(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    CHECK(authum_logs_open(&g_test_quiet, "quiet", authum_null));
    CHECK(authum_logs_open(&g_test_hello, "hello", authum_null));

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

    /* `off` disables the source in every destination, including errors. */
    authum_logs_write(&g_test_quiet, authum_logger_level_error, "quiet error line");
    CHECK(!test_file_exists("quiet", TEST_DAY_ONE));
    CHECK(!test_file_contains("authum", TEST_DAY_ONE, "quiet error line"));
    CHECK(g_test_mirrored == 0);
    CHECK(!authum_logs_is_enabled(&g_test_quiet, authum_logger_level_error));

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

/* Barriers publish accepted records; shutdown drains pending records. */
void
test_buffering(void)
{
    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_qproto, "qproto", authum_null));

    authum_logs_write(&g_test_qproto, authum_logger_level_info, "buffered line");

    authum_logs_write(&g_test_qproto, authum_logger_level_error, "urgent line");
    CHECK(authum_logs_sync(5000));
    CHECK(test_on_disk("qproto", TEST_DAY_ONE, "urgent line"));
    CHECK(test_on_disk("qproto", TEST_DAY_ONE, "buffered line")); /* went out with it */

    authum_logs_write(&g_test_qproto, authum_logger_level_info, "flushed line");

    CHECK(authum_logs_sync(5000));
    CHECK(test_on_disk("qproto", TEST_DAY_ONE, "flushed line"));

    authum_logs_write(&g_test_qproto, authum_logger_level_info, "closed line");
    authum_logs_deinit(); /* closing writes what is left */
    CHECK(test_on_disk("qproto", TEST_DAY_ONE, "closed line"));
}

/* Flush must progress without game frames, barriers, channel close or shutdown. */
void
test_periodic_flush(void)
{
    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    authum_char_t previous[256];
    test_log_path(previous, sizeof previous, "qproto", TEST_DAY_ONE);
    remove(previous);
    authum_logs_write(&g_test_qproto, authum_logger_level_info, "automatic flush line");
    authum_i64_t start = authum_clock_monotonic_ms(authum_clock_host());
    while (!test_on_disk("qproto", TEST_DAY_ONE, "automatic flush line") &&
           authum_clock_monotonic_ms(authum_clock_host()) - start < 5000)
    {
#if defined(_WIN32)
        Sleep(10);
#else
        struct timespec delay = {0, 10000000};
        nanosleep(&delay, NULL);
#endif
    }
    CHECK(test_on_disk("qproto", TEST_DAY_ONE, "automatic flush line"));
    authum_logs_deinit();
}

/* More journals than open file slots: eviction must flush and reopening appends. */
void
test_file_cache(void)
{
    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    for (authum_u32_t round = 0; round < 2; ++round)
    {
        for (authum_u32_t index = 0; index < 40; ++index)
        {
            authum_char_t name[32];
            snprintf(name, sizeof name, "async_cache_%u", (unsigned)index);
            authum_ptr journal = authum_logs_open_journal(&g_test_qproto, name);
            CHECK(journal != authum_null);
            authum_logs_write(journal, authum_logger_level_info,
                              round == 0 ? "before eviction" : "after reopening");
            authum_logs_close_journal(journal);
        }
    }
    authum_logs_close(&g_test_qproto);
    authum_logs_deinit();
    for (authum_u32_t index = 0; index < 40; ++index)
    {
        authum_char_t name[64];
        snprintf(name, sizeof name, "qproto/async_cache_%u", (unsigned)index);
        CHECK(test_on_disk(name, TEST_DAY_ONE, "before eviction"));
        CHECK(test_on_disk(name, TEST_DAY_ONE, "after reopening"));
        authum_char_t path[256];
        test_log_path(path, sizeof path, name, TEST_DAY_ONE);
        remove(path);
    }
}

/* Journals: a module's own extra files, with their own level. */
void
test_journals(void)
{
    authum_ptr audit;
    authum_ptr again;
    authum_ptr bans;
    authum_char_t path[256];

    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_hello, "hello", authum_null));
    CHECK(authum_logs_open(&g_test_qproto, "qproto", authum_null));

    audit = authum_logs_open_journal(&g_test_hello, "audit");
    CHECK(audit != authum_null);
    CHECK(authum_logs_open_journal(authum_null, "x") == authum_null);
    CHECK(authum_logs_open_journal(&g_test_hello, "") == authum_null);
    CHECK(authum_logs_open_journal(audit, "nested") == authum_null); /* journals of journals: no */

    /* hello is at info, its audit journal at debug (hello.audit.log_level). */
    authum_logs_write(audit, authum_logger_level_debug, "audit debug line");
    CHECK(test_file_contains("hello/audit", TEST_DAY_ONE, "[debug] audit debug line"));
    CHECK(!test_file_contains("hello", TEST_DAY_ONE, "audit debug line"));

    authum_logs_write(audit, authum_logger_level_warning, "audit warning line");
    CHECK(test_file_contains("authum", TEST_DAY_ONE, "[warning] [hello/audit] audit warning line"));

    /* Opening the same name again shares it; it lives until the last close. */
    again = authum_logs_open_journal(&g_test_hello, "audit");
    CHECK(again == audit);
    authum_logs_close_journal(again);
    CHECK(authum_logs_is_enabled(audit, authum_logger_level_info));
    authum_logs_close_journal(audit);
    CHECK(!authum_logs_is_enabled(audit, authum_logger_level_error));

    /* A module that forgets its journal: it goes when the module does. */
    bans = authum_logs_open_journal(&g_test_qproto, "bans");
    authum_logs_write(bans, authum_logger_level_info, "ban line");
    test_log_path(path, sizeof path, "qproto/bans", TEST_DAY_ONE);
    authum_logs_close(&g_test_qproto);
    CHECK(authum_logs_sync(5000));
    CHECK(test_path_contains(path, "ban line"));
    CHECK(!authum_logs_is_enabled(bans, authum_logger_level_error));

    authum_logs_deinit();
}

/* authum.cfg edits apply on reload (the host reloads on every map change). */
void
test_reload(void)
{
    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    CHECK(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    CHECK(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug));

    test_write_cfg("qproto.log_level = warning\n");
    CHECK(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug)); /* not yet */
    authum_logs_reload();
    CHECK(!authum_logs_is_enabled(&g_test_qproto, authum_logger_level_info));
    CHECK(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_warning));

    test_write_cfg(g_test_cfg);
    authum_logs_deinit();
}

/* log_keep_days = 30: older daily files go when a channel opens a new day. */
void
test_prune(void)
{
    authum_char_t old_path[256];
    authum_char_t recent_path[256];

    test_mkdir(TEST_ROOT "/modules");
    test_mkdir(TEST_ROOT "/modules/hello");
    test_mkdir(TEST_ROOT "/modules/hello/logs");
    test_log_path(old_path, sizeof old_path, "hello", TEST_DAY_ONE - 40 * TEST_DAY);
    test_log_path(recent_path, sizeof recent_path, "hello", TEST_DAY_ONE - 5 * TEST_DAY);
    test_touch(old_path);
    test_touch(recent_path);
    test_touch(TEST_ROOT "/modules/hello/logs/notes.txt");
    CHECK(test_exists(old_path));

    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_hello, "hello", authum_null));
    authum_logs_write(&g_test_hello, authum_logger_level_info, "prune trigger");
    authum_logs_deinit();

    CHECK(!test_exists(old_path));
    CHECK(test_exists(recent_path));
    CHECK(test_exists(TEST_ROOT "/modules/hello/logs/notes.txt")); /* not a daily file: kept */
    remove(recent_path);
    remove(TEST_ROOT "/modules/hello/logs/notes.txt");
}

/* What a module does: authum_logger_* through the host's channel. */
void
test_module_logger(void)
{
    authum_logger_channel_t channel;
    authum_host_t host;
    authum_logger_t log;
    authum_logger_t audit;

    CHECK(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_hello, "hello", authum_null));

    channel.ops = authum_logs_ops();
    channel.owner = &g_test_hello;
    memset(&host, 0, sizeof host);
    host.logger = &channel;

    authum_logger_init(&log, &host);
    authum_logger_info(&log, "seen %d players", 3);
    CHECK(test_file_contains("hello", TEST_DAY_ONE, "[info] seen 3 players"));
    CHECK(!authum_logger_is_enabled(&log, authum_logger_level_debug));
    authum_host_log_line(&host, authum_bool_true, "via host_log_line");
    CHECK(test_file_contains("hello", TEST_DAY_ONE, "[error] via host_log_line"));

    CHECK(authum_logger_open(&audit, &host, "audit"));
    CHECK(authum_logger_is_enabled(&audit, authum_logger_level_debug));
    authum_logger_debug(&audit, "admin %s did %s", "vega", "kick");
    CHECK(test_file_contains("hello/audit", TEST_DAY_ONE, "[debug] admin vega did kick"));
    authum_logger_close(&audit);
    authum_logger_info(&audit, "after close");
    CHECK(!test_file_contains("hello/audit", TEST_DAY_ONE, "after close"));

    authum_logger_close(&log);
    authum_logger_info(&log, "after close");
    CHECK(!test_file_contains("hello", TEST_DAY_ONE, "after close"));

    CHECK(!authum_logger_open(&audit, authum_null, "audit"));

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

static void
test_local_config(void)
{
    test_mkdir(TEST_ROOT "/modules");
    test_mkdir(TEST_ROOT "/modules/hello");
    test_mkdir(TEST_ROOT "/modules/hello/configs");
    const char *local_path = TEST_ROOT "/modules/hello/configs/hello.cfg";
    FILE *cfg = fopen(local_path, "w");
    CHECK(cfg != NULL);
    fputs("log_level = debug\nlog_keep_days = 3\nlog_host = off\nlog_console = off\n"
          "audit.log_level = debug\n",
          cfg);
    fclose(cfg);
    char old[256], recent[256];
    test_log_path(old, sizeof old, "hello", TEST_DAY_ONE - 5 * TEST_DAY);
    test_log_path(recent, sizeof recent, "hello", TEST_DAY_ONE - 2 * TEST_DAY);
    test_mkdir(TEST_ROOT "/modules/hello/logs");
    test_touch(old);
    test_touch(recent);
    test_touch(TEST_ROOT "/modules/hello/logs/2020-99-99.log");
    test_write_cfg("log_level = warning\nlog_keep_days = 30\n");
    CHECK(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_hello, "hello", authum_null));
    CHECK(authum_logs_is_enabled(&g_test_hello, authum_logger_level_debug));
    CHECK(authum_logs_sync(5000));
    /* Registration cleans an idle module; no write was required. */
    CHECK(!test_exists(old));
    CHECK(test_exists(recent));
    CHECK(test_exists(TEST_ROOT "/modules/hello/logs/2020-99-99.log"));

    authum_ptr audit = authum_logs_open_journal(&g_test_hello, "audit");
    CHECK(audit != authum_null);
    CHECK(authum_logs_is_enabled(audit, authum_logger_level_debug));
    test_write_cfg("log_level = warning\nhello.log_level = error\n");
    authum_logs_reload();
    CHECK(!authum_logs_is_enabled(&g_test_hello, authum_logger_level_debug));
    CHECK(!authum_logs_is_enabled(audit, authum_logger_level_debug));
    test_write_cfg("hello.log_level = off\nhello.audit.log_level = debug\n");
    authum_logs_reload();
    CHECK(!authum_logs_is_enabled(audit, authum_logger_level_error));
    /* File destination off is independent of console and shared-host routing. */
    test_write_cfg("hello.log_level = info\nhello.log_file = off\nhello.log_host = warning\n"
                   "hello.log_console = error\n");
    authum_logs_reload();
    CHECK(!authum_logs_is_enabled(&g_test_hello, authum_logger_level_info));
    int mirrored = g_test_mirrored;
    authum_logs_write(&g_test_hello, authum_logger_level_error, "destination independence");
    CHECK(g_test_mirrored == mirrored + 1);
    CHECK(test_file_contains("authum", TEST_DAY_ONE, "destination independence"));
    CHECK(!test_file_contains("hello", TEST_DAY_ONE, "destination independence"));
    authum_logs_deinit();
    remove(local_path);
    remove(recent);
    remove(TEST_ROOT "/modules/hello/logs/2020-99-99.log");
    test_write_cfg(g_test_cfg);
}

static void
test_constructor(void)
{
    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    CHECK(authum_logs_open(&g_test_hello, "hello", authum_null));
    authum_logger_channel_t channel = {authum_logs_ops(), &g_test_hello};
    authum_host_t host = {0};
    host.logger = &channel;
    authum_host_log_info(&host, "automatic channel %d", 42);
    CHECK(test_file_contains("hello", TEST_DAY_ONE, "automatic channel 42"));
    authum_logger_t raw, conflict;
    authum_logger_options_t options = {"custom", authum_logger_format_raw};
    CHECK(authum_logger_create(&raw, &host, &options));
    authum_logger_info(&raw, "original message");
    char path[256];
    test_log_path(path, sizeof path, "hello/custom", TEST_DAY_ONE);
    CHECK(authum_logs_sync(5000));
    FILE *file = fopen(path, "r");
    CHECK(file != NULL);
    char text[256] = {0};
    CHECK(fgets(text, sizeof text, file) != NULL);
    CHECK_STR(text, "original message\n");
    fclose(file);
    options.format = authum_logger_format_standard;
    CHECK(!authum_logger_create(&conflict, &host, &options));
    /* Admin format overrides the constructor and survives reload. */
    test_write_cfg("hello.custom.log_format = standard\n");
    authum_logs_reload();
    authum_logger_info(&raw, "admin format");
    CHECK(test_file_contains("hello/custom", TEST_DAY_ONE, "[info] admin format"));
    authum_logger_close(&raw);
    authum_logs_deinit();
    remove(path);
    test_write_cfg(g_test_cfg);
}

static void
test_custom_sandbox(void)
{
    const char *base = TEST_ROOT "/custom_sandbox";
    test_mkdir(base);
    test_mkdir(TEST_ROOT "/custom_sandbox/configs");
    FILE *cfg = fopen(TEST_ROOT "/custom_sandbox/configs/private.cfg", "w");
    CHECK(cfg != NULL);
    fputs("log_level = debug\n", cfg);
    fclose(cfg);
    test_write_cfg("log_level = off\n");
    CHECK(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    int owner;
    CHECK(authum_logs_open(&owner, "private", base));
    CHECK(authum_logs_is_enabled(&owner, authum_logger_level_debug));
    authum_logs_write(&owner, authum_logger_level_debug, "custom sandbox");
    CHECK(authum_logs_sync(5000));
    char day[32], path[256];
    struct tm *tm = localtime(&g_test_now);
    strftime(day, sizeof day, "%Y-%m-%d.log", tm);
    snprintf(path, sizeof path, "%s/logs/%s", base, day);
    CHECK(test_path_contains(path, "custom sandbox"));
    authum_logs_deinit();
    remove(path);
    remove(TEST_ROOT "/custom_sandbox/configs/private.cfg");
    test_write_cfg(g_test_cfg);
}

int
main(int argc, char **argv)
{
    test_reset();
    if (argc == 2 && strcmp(argv[1], "--environment-off") == 0)
    {
        CHECK(authum_logs_init(TEST_ROOT, test_mirror));
        CHECK(authum_logs_open(&g_test_qproto, "qproto", authum_null));
        CHECK(!authum_logs_is_enabled(&g_test_qproto, authum_logger_level_error));
        CHECK(!authum_logs_is_enabled(authum_null, authum_logger_level_error));
        authum_logs_write(&g_test_qproto, authum_logger_level_error, "environment disabled");
        CHECK(g_test_mirrored == 0);
#if defined(_WIN32)
        _putenv_s("AUTHUM_LOG_LEVEL", "debug");
#else
        setenv("AUTHUM_LOG_LEVEL", "debug", 1);
#endif
        authum_logs_reload();
        CHECK(!authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug));
        authum_logs_deinit();
        return authum_check_report("logs_environment");
    }
    test_level_names();
    test_routing();
    test_buffering();
    test_periodic_flush();
    test_file_cache();
    test_journals();
    test_reload();
    test_prune();
    test_module_logger();
    test_local_config();
    test_constructor();
    test_custom_sandbox();
    return authum_check_report("logs");
}
