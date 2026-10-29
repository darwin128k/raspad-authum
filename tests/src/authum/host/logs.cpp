/**
 * @file logs.cpp
 * @brief Host log channels: a file per module and day, levels from authum.cfg,
 *        warnings copied to the authum channel, console mirror, journals,
 *        buffering, old files pruned, cfg reload.
 */

#include <gtest/gtest.h>

#include <authum/host/clock.h>
#include <authum/logger.h>
#include <authum/host/logs.h>
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
    else if (slash != authum_null)
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

    if (file == authum_null)
    {
        return authum_bool_false;
    }
    while (!found && fgets(line, sizeof line, file) != authum_null)
    {
        found = strstr(line, needle) != authum_null ? authum_bool_true : authum_bool_false;
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
    EXPECT_TRUE(authum_logs_sync(5000));
    return test_on_disk(dir, when, needle);
}

authum_bool_t
test_exists(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == authum_null)
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

    EXPECT_TRUE(authum_logs_sync(5000));
    test_log_path(path, sizeof path, dir, when);
    return test_exists(path);
}

void
test_write_cfg(const char *text)
{
    FILE *cfg = fopen(TEST_ROOT "/configs/authum.cfg", "w");

    EXPECT_NE(cfg, authum_null);
    if (cfg != authum_null)
    {
        fputs(text, cfg);
        fclose(cfg);
    }
}

void
test_touch(const char *path)
{
    FILE *file = fopen(path, "w");

    if (file != authum_null)
    {
        fclose(file);
    }
}

void
test_mkdir(const char *text)
{
    authum_path_t dir;

    authum_path_set_cstr(&dir, text);
    EXPECT_TRUE(authum_path_ensure_dir(&dir));
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
    g_test_now = TEST_DAY_ONE;
    g_test_mirrored = 0;
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

TEST(authum_logs, routing)
{
    test_reset();
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    EXPECT_TRUE(authum_logs_open(&g_test_quiet, "quiet", authum_null));
    EXPECT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));

    /* Per-module level from the cfg. */
    EXPECT_TRUE(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug));
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_hello, authum_logger_level_debug));
    EXPECT_TRUE(authum_logs_is_enabled(&g_test_hello, authum_logger_level_info));

    authum_logs_write(&g_test_qproto, authum_logger_level_debug, "qproto debug line");
    EXPECT_TRUE(test_file_contains("qproto", TEST_DAY_ONE, "[debug] qproto debug line"));
    EXPECT_FALSE(test_file_contains("authum", TEST_DAY_ONE, "qproto debug line"));

    authum_logs_write(&g_test_hello, authum_logger_level_debug, "hello debug line");
    EXPECT_FALSE(test_file_exists("hello", TEST_DAY_ONE));

    /* A module's warning: its own file and the authum file, not the console. */
    authum_logs_write(&g_test_hello, authum_logger_level_warning, "hello warning line");
    EXPECT_TRUE(test_file_contains("hello", TEST_DAY_ONE, "[warning] hello warning line"));
    EXPECT_TRUE(test_file_contains("authum", TEST_DAY_ONE, "[warning] [hello] hello warning line"));
    EXPECT_EQ(g_test_mirrored, 0);

    /* `off` disables the source in every destination, including errors. */
    authum_logs_write(&g_test_quiet, authum_logger_level_error, "quiet error line");
    EXPECT_FALSE(test_file_exists("quiet", TEST_DAY_ONE));
    EXPECT_FALSE(test_file_contains("authum", TEST_DAY_ONE, "quiet error line"));
    EXPECT_EQ(g_test_mirrored, 0);
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_quiet, authum_logger_level_error));

    /* The host's own lines (owner null). */
    authum_logs_write(authum_null, authum_logger_level_info, "host info line");
    EXPECT_TRUE(test_file_contains("authum", TEST_DAY_ONE, "[info] host info line"));

    /* Next day, next file. */
    g_test_now = TEST_DAY_TWO;
    authum_logs_write(&g_test_qproto, authum_logger_level_info, "qproto day two");
    EXPECT_TRUE(test_file_contains("qproto", TEST_DAY_TWO, "qproto day two"));
    EXPECT_FALSE(test_file_contains("qproto", TEST_DAY_ONE, "qproto day two"));
    g_test_now = TEST_DAY_ONE;

    /* A closed channel takes nothing. */
    authum_logs_close(&g_test_hello);
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_hello, authum_logger_level_error));
    authum_logs_write(&g_test_hello, authum_logger_level_error, "after close");
    EXPECT_FALSE(test_file_contains("hello", TEST_DAY_ONE, "after close"));
    EXPECT_FALSE(test_file_contains("authum", TEST_DAY_ONE, "after close"));

    authum_logs_deinit();
}

/* Barriers publish accepted records; shutdown drains pending records. */
TEST(authum_logs, buffering)
{
    test_reset();
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));

    authum_logs_write(&g_test_qproto, authum_logger_level_info, "buffered line");

    authum_logs_write(&g_test_qproto, authum_logger_level_error, "urgent line");
    EXPECT_TRUE(authum_logs_sync(5000));
    EXPECT_TRUE(test_on_disk("qproto", TEST_DAY_ONE, "urgent line"));
    EXPECT_TRUE(test_on_disk("qproto", TEST_DAY_ONE, "buffered line")); /* went out with it */

    authum_logs_write(&g_test_qproto, authum_logger_level_info, "flushed line");

    EXPECT_TRUE(authum_logs_sync(5000));
    EXPECT_TRUE(test_on_disk("qproto", TEST_DAY_ONE, "flushed line"));

    authum_logs_write(&g_test_qproto, authum_logger_level_info, "closed line");
    authum_logs_deinit(); /* closing writes what is left */
    EXPECT_TRUE(test_on_disk("qproto", TEST_DAY_ONE, "closed line"));
}

/* Flush must progress without game frames, barriers, channel close or shutdown. */
TEST(authum_logs, periodic_flush)
{
    test_reset();
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    authum_char_t previous[256];
    test_log_path(previous, sizeof previous, "qproto", TEST_DAY_ONE);
    remove(previous);
    authum_logs_write(&g_test_qproto, authum_logger_level_info, "automatic flush line");
    authum_i64_t start = authum_clock_get_monotonic_ms(authum_clock_host());
    while (!test_on_disk("qproto", TEST_DAY_ONE, "automatic flush line") &&
           authum_clock_get_monotonic_ms(authum_clock_host()) - start < 5000)
    {
#if defined(_WIN32)
        Sleep(10);
#else
        struct timespec delay = {0, 10000000};
        nanosleep(&delay, authum_null);
#endif
    }
    EXPECT_TRUE(test_on_disk("qproto", TEST_DAY_ONE, "automatic flush line"));
    authum_logs_deinit();
}

/* More journals than open file slots: eviction must flush and reopening appends. */
TEST(authum_logs, file_cache)
{
    test_reset();
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    for (authum_u32_t round = 0; round < 2; ++round)
    {
        for (authum_u32_t index = 0; index < 40; ++index)
        {
            authum_char_t name[32];
            snprintf(name, sizeof name, "async_cache_%u", (unsigned)index);
            authum_ptr journal = authum_logs_open_journal(&g_test_qproto, name);
            EXPECT_NE(journal, authum_null);
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
        EXPECT_TRUE(test_on_disk(name, TEST_DAY_ONE, "before eviction"));
        EXPECT_TRUE(test_on_disk(name, TEST_DAY_ONE, "after reopening"));
        authum_char_t path[256];
        test_log_path(path, sizeof path, name, TEST_DAY_ONE);
        remove(path);
    }
}

/* Journals: a module's own extra files, with their own level. */
TEST(authum_logs, journals)
{
    test_reset();
    authum_ptr audit;
    authum_ptr again;
    authum_ptr bans;
    authum_char_t path[256];

    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));
    EXPECT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));

    audit = authum_logs_open_journal(&g_test_hello, "audit");
    EXPECT_NE(audit, authum_null);
    EXPECT_EQ(authum_logs_open_journal(authum_null, "x"), authum_null);
    EXPECT_EQ(authum_logs_open_journal(&g_test_hello, ""), authum_null);
    EXPECT_EQ(authum_logs_open_journal(audit, "nested"),
              authum_null); /* journals of journals: no */

    /* hello is at info, its audit journal at debug (hello.audit.log_level). */
    authum_logs_write(audit, authum_logger_level_debug, "audit debug line");
    EXPECT_TRUE(test_file_contains("hello/audit", TEST_DAY_ONE, "[debug] audit debug line"));
    EXPECT_FALSE(test_file_contains("hello", TEST_DAY_ONE, "audit debug line"));

    authum_logs_write(audit, authum_logger_level_warning, "audit warning line");
    EXPECT_TRUE(
        test_file_contains("authum", TEST_DAY_ONE, "[warning] [hello/audit] audit warning line"));

    /* Opening the same name again shares it; it lives until the last close. */
    again = authum_logs_open_journal(&g_test_hello, "audit");
    EXPECT_EQ(again, audit);
    authum_logs_close_journal(again);
    EXPECT_TRUE(authum_logs_is_enabled(audit, authum_logger_level_info));
    authum_logs_close_journal(audit);
    EXPECT_FALSE(authum_logs_is_enabled(audit, authum_logger_level_error));

    /* A module that forgets its journal: it goes when the module does. */
    bans = authum_logs_open_journal(&g_test_qproto, "bans");
    authum_logs_write(bans, authum_logger_level_info, "ban line");
    test_log_path(path, sizeof path, "qproto/bans", TEST_DAY_ONE);
    authum_logs_close(&g_test_qproto);
    EXPECT_TRUE(authum_logs_sync(5000));
    EXPECT_TRUE(test_path_contains(path, "ban line"));
    EXPECT_FALSE(authum_logs_is_enabled(bans, authum_logger_level_error));

    authum_logs_deinit();
}

/* authum.cfg edits apply on reload (the host reloads on every map change). */
TEST(authum_logs, reload)
{
    test_reset();
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    EXPECT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    EXPECT_TRUE(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug));

    test_write_cfg("qproto.log_level = warning\n");
    EXPECT_TRUE(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug)); /* not yet */
    authum_logs_reload();
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_info));
    EXPECT_TRUE(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_warning));

    test_write_cfg(g_test_cfg);
    authum_logs_deinit();
}

/* log_keep_days = 30: older daily files go when a channel opens a new day. */
TEST(authum_logs, prune)
{
    test_reset();
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
    EXPECT_TRUE(test_exists(old_path));

    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));
    authum_logs_write(&g_test_hello, authum_logger_level_info, "prune trigger");
    authum_logs_deinit();

    EXPECT_FALSE(test_exists(old_path));
    EXPECT_TRUE(test_exists(recent_path));
    EXPECT_TRUE(
        test_exists(TEST_ROOT "/modules/hello/logs/notes.txt")); /* not a daily file: kept */
    remove(recent_path);
    remove(TEST_ROOT "/modules/hello/logs/notes.txt");
}

/* What a module does: authum_logger_* through the host's channel. */
TEST(authum_logs, module_logger)
{
    test_reset();
    authum_logger_channel_t channel;
    authum_host_t host;
    authum_logger_t log;
    authum_logger_t audit;

    EXPECT_TRUE(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));

    channel.ops = authum_logs_ops();
    channel.owner = &g_test_hello;
    memset(&host, 0, sizeof host);
    host.logger = &channel;

    authum_logger_init(&log, &host);
    authum_logger_info(&log, "seen %d players", 3);
    EXPECT_TRUE(test_file_contains("hello", TEST_DAY_ONE, "[info] seen 3 players"));
    EXPECT_FALSE(authum_logger_is_enabled(&log, authum_logger_level_debug));
    authum_host_log_line(&host, authum_bool_true, "via host_log_line");
    EXPECT_TRUE(test_file_contains("hello", TEST_DAY_ONE, "[error] via host_log_line"));

    EXPECT_TRUE(authum_logger_open(&audit, &host, "audit"));
    EXPECT_TRUE(authum_logger_is_enabled(&audit, authum_logger_level_debug));
    authum_logger_debug(&audit, "admin %s did %s", "vega", "kick");
    EXPECT_TRUE(test_file_contains("hello/audit", TEST_DAY_ONE, "[debug] admin vega did kick"));
    authum_logger_close(&audit);
    authum_logger_info(&audit, "after close");
    EXPECT_FALSE(test_file_contains("hello/audit", TEST_DAY_ONE, "after close"));

    authum_logger_close(&log);
    authum_logger_info(&log, "after close");
    EXPECT_FALSE(test_file_contains("hello", TEST_DAY_ONE, "after close"));

    EXPECT_FALSE(authum_logger_open(&audit, authum_null, "audit"));

    authum_logs_deinit();
}

TEST(authum_logs, level_names)
{
    test_reset();
    authum_logger_level_t level = authum_logger_level_info;

    EXPECT_TRUE(authum_logger_level_parse("debug", &level) && level == authum_logger_level_debug);
    EXPECT_TRUE(authum_logger_level_parse("WARN", &level) && level == authum_logger_level_warning);
    EXPECT_FALSE(authum_logger_level_parse("loud", &level));
    EXPECT_STREQ(authum_logger_level_get_name(authum_logger_level_critical), "critical");
    EXPECT_STREQ(authum_logger_level_get_name(42), "?");
}

TEST(authum_logs, local_config)
{
    test_reset();
    test_mkdir(TEST_ROOT "/modules");
    test_mkdir(TEST_ROOT "/modules/hello");
    test_mkdir(TEST_ROOT "/modules/hello/configs");
    const char *local_path = TEST_ROOT "/modules/hello/configs/hello.cfg";
    FILE *cfg = fopen(local_path, "w");
    EXPECT_NE(cfg, authum_null);
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
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, test_mirror));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));
    EXPECT_TRUE(authum_logs_is_enabled(&g_test_hello, authum_logger_level_debug));
    EXPECT_TRUE(authum_logs_sync(5000));
    /* Registration cleans an idle module; no write was required. */
    EXPECT_FALSE(test_exists(old));
    EXPECT_TRUE(test_exists(recent));
    EXPECT_TRUE(test_exists(TEST_ROOT "/modules/hello/logs/2020-99-99.log"));

    authum_ptr audit = authum_logs_open_journal(&g_test_hello, "audit");
    EXPECT_NE(audit, authum_null);
    EXPECT_TRUE(authum_logs_is_enabled(audit, authum_logger_level_debug));
    test_write_cfg("log_level = warning\nhello.log_level = error\n");
    authum_logs_reload();
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_hello, authum_logger_level_debug));
    EXPECT_FALSE(authum_logs_is_enabled(audit, authum_logger_level_debug));
    test_write_cfg("hello.log_level = off\nhello.audit.log_level = debug\n");
    authum_logs_reload();
    EXPECT_FALSE(authum_logs_is_enabled(audit, authum_logger_level_error));
    /* File destination off is independent of console and shared-host routing. */
    test_write_cfg("hello.log_level = info\nhello.log_file = off\nhello.log_host = warning\n"
                   "hello.log_console = error\n");
    authum_logs_reload();
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_hello, authum_logger_level_info));
    int mirrored = g_test_mirrored;
    authum_logs_write(&g_test_hello, authum_logger_level_error, "destination independence");
    EXPECT_EQ(g_test_mirrored, mirrored + 1);
    EXPECT_TRUE(test_file_contains("authum", TEST_DAY_ONE, "destination independence"));
    EXPECT_FALSE(test_file_contains("hello", TEST_DAY_ONE, "destination independence"));
    authum_logs_deinit();
    remove(local_path);
    remove(recent);
    remove(TEST_ROOT "/modules/hello/logs/2020-99-99.log");
    test_write_cfg(g_test_cfg);
}

TEST(authum_logs, constructor)
{
    test_reset();
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    EXPECT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));
    authum_logger_channel_t channel = {authum_logs_ops(), &g_test_hello};
    authum_host_t host = {0};
    host.logger = &channel;
    authum_host_log_info(&host, "automatic channel %d", 42);
    EXPECT_TRUE(test_file_contains("hello", TEST_DAY_ONE, "automatic channel 42"));
    authum_logger_t raw, conflict;
    authum_logger_options_t options = {"custom", authum_logger_format_raw};
    EXPECT_TRUE(authum_logger_create(&raw, &host, &options));
    authum_logger_info(&raw, "original message");
    char path[256];
    test_log_path(path, sizeof path, "hello/custom", TEST_DAY_ONE);
    EXPECT_TRUE(authum_logs_sync(5000));
    FILE *file = fopen(path, "r");
    EXPECT_NE(file, authum_null);
    char text[256] = {0};
    EXPECT_NE(fgets(text, sizeof text, file), authum_null);
    EXPECT_STREQ(text, "original message\n");
    fclose(file);
    options.format = authum_logger_format_standard;
    EXPECT_FALSE(authum_logger_create(&conflict, &host, &options));
    /* Admin format overrides the constructor and survives reload. */
    test_write_cfg("hello.custom.log_format = standard\n");
    authum_logs_reload();
    authum_logger_info(&raw, "admin format");
    EXPECT_TRUE(test_file_contains("hello/custom", TEST_DAY_ONE, "[info] admin format"));
    authum_logger_close(&raw);
    authum_logs_deinit();
    remove(path);
    test_write_cfg(g_test_cfg);
}

TEST(authum_logs, custom_sandbox)
{
    test_reset();
    const char *base = TEST_ROOT "/custom_sandbox";
    test_mkdir(base);
    test_mkdir(TEST_ROOT "/custom_sandbox/configs");
    FILE *cfg = fopen(TEST_ROOT "/custom_sandbox/configs/private.cfg", "w");
    EXPECT_NE(cfg, authum_null);
    fputs("log_level = debug\n", cfg);
    fclose(cfg);
    test_write_cfg("log_level = off\n");
    EXPECT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
    authum_logs_set_clock(test_clock);
    int owner;
    EXPECT_TRUE(authum_logs_open(&owner, "private", base));
    EXPECT_TRUE(authum_logs_is_enabled(&owner, authum_logger_level_debug));
    authum_logs_write(&owner, authum_logger_level_debug, "custom sandbox");
    EXPECT_TRUE(authum_logs_sync(5000));
    char day[32], path[256];
    struct tm *tm = localtime(&g_test_now);
    strftime(day, sizeof day, "%Y-%m-%d.log", tm);
    snprintf(path, sizeof path, "%s/logs/%s", base, day);
    EXPECT_TRUE(test_path_contains(path, "custom sandbox"));
    authum_logs_deinit();
    remove(path);
    remove(TEST_ROOT "/custom_sandbox/configs/private.cfg");
    test_write_cfg(g_test_cfg);
}

TEST(authum_logs_environment, disabled)
{
    test_reset();

    EXPECT_TRUE(authum_logs_init(TEST_ROOT, test_mirror));
    EXPECT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_error));
    EXPECT_FALSE(authum_logs_is_enabled(authum_null, authum_logger_level_error));
    authum_logs_write(&g_test_qproto, authum_logger_level_error, "environment disabled");
    EXPECT_EQ(g_test_mirrored, 0);
#if defined(_WIN32)
    _putenv_s("AUTHUM_LOG_LEVEL", "debug");
#else
    setenv("AUTHUM_LOG_LEVEL", "debug", 1);
#endif
    authum_logs_reload();
    EXPECT_FALSE(authum_logs_is_enabled(&g_test_qproto, authum_logger_level_debug));
    authum_logs_deinit();
#if defined(_WIN32)
    _putenv_s("AUTHUM_LOG_LEVEL", "off");
#else
    setenv("AUTHUM_LOG_LEVEL", "off", 1);
#endif
}

namespace tests
{
class authum_log_line : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        test_reset();
        struct tm calendar = {};
        calendar.tm_year = 126;
        calendar.tm_mon = 9;
        calendar.tm_mday = 26;
        calendar.tm_hour = 8;
        calendar.tm_min = 9;
        calendar.tm_sec = 18;
        calendar.tm_isdst = -1;
        g_test_now = mktime(&calendar);
        ASSERT_NE(g_test_now, static_cast<time_t>(-1));
        ASSERT_TRUE(authum_logs_init(TEST_ROOT, authum_null));
        authum_logs_set_clock(test_clock);
    }

    void
    TearDown() override
    {
        authum_logs_deinit();
    }
};

TEST_F(authum_log_line, module_file_uses_hlds_timestamp_without_repeating_module_name)
{
    ASSERT_TRUE(authum_logs_open(&g_test_qproto, "qproto", authum_null));
    authum_logs_write(&g_test_qproto, authum_logger_level_info, "module line");
    EXPECT_TRUE(
        test_file_contains("qproto", g_test_now, "L 10/26/2026 - 08:09:18: [info] module line\n"));
}

TEST_F(authum_log_line, host_copy_includes_source_module)
{
    ASSERT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));
    authum_logs_write(&g_test_hello, authum_logger_level_warning, "module warning");
    EXPECT_TRUE(test_file_contains("authum", g_test_now,
                                   "L 10/26/2026 - 08:09:18: [warning] [hello] module warning\n"));
}

TEST_F(authum_log_line, host_own_line_has_no_redundant_module_tag)
{
    authum_logs_write(authum_null, authum_logger_level_info, "host line");
    EXPECT_TRUE(
        test_file_contains("authum", g_test_now, "L 10/26/2026 - 08:09:18: [info] host line\n"));
}

TEST_F(authum_log_line, journal_tag_is_only_added_to_host_copy)
{
    ASSERT_TRUE(authum_logs_open(&g_test_hello, "hello", authum_null));
    auto *journal = authum_logs_open_journal(&g_test_hello, "audit");
    ASSERT_NE(journal, authum_null);
    authum_logs_write(journal, authum_logger_level_warning, "journal line");
    EXPECT_TRUE(test_file_contains("hello/audit", g_test_now,
                                   "L 10/26/2026 - 08:09:18: [warning] journal line\n"));
    EXPECT_TRUE(test_file_contains(
        "authum", g_test_now, "L 10/26/2026 - 08:09:18: [warning] [hello/audit] journal line\n"));
}
} // namespace tests
