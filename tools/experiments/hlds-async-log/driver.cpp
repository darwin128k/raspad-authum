#ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#endif
#include <gtest/gtest.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern "C" int
__vfprintf_chk(FILE *, int, const char *, va_list);

/* Same libc entry point used by this HLDS filesystem's FS_vfprintf. */
int
emit(FILE *file, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = __vfprintf_chk(file, 1, fmt, args);
    va_end(args);
    return result;
}

double
now(void)
{
    struct timespec value;
    clock_gettime(CLOCK_MONOTONIC, &value);
    return value.tv_sec + value.tv_nsec / 1e9;
}

void
read_file(const char *path, char *data, size_t capacity, size_t *length)
{
    FILE *file = fopen(path, "rb");
    ASSERT_TRUE(file);
    *length = fread(data, 1, capacity, file);
    ASSERT_TRUE(!ferror(file));
    ASSERT_TRUE(fclose(file) == 0);
}

static char path[4096], other[4096], actual[32768], expected[32768];
static const char *mode;
static FILE *file;

namespace tests
{
class hlds_async_log : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        const char *directory = getenv("AUTHUM_ASYNC_LOG_TEST_DIR");
        mode = getenv("AUTHUM_ASYNC_LOG_TEST_MODE");
        ASSERT_NE(directory, nullptr);
        ASSERT_NE(mode, nullptr);
        snprintf(path, sizeof path, "%s/L1026000.log", directory);
        snprintf(other, sizeof other, "%s/unrelated.txt", directory);
        file = nullptr;
    }

    void
    TearDown() override
    {
        if (file)
        {
            fclose(file);
        }
        file = nullptr;
    }
};

TEST_F(hlds_async_log, visibility_and_exact_bytes_across_stdio_buffer_boundary)
{
    file = fopen(path, "wt");
    ASSERT_NE(file, nullptr);
    size_t length = 0;
    for (int index = 0; index < 100; ++index)
    {
        int count = snprintf(
            expected + length, sizeof expected - length,
            "L 10/26/2026 - 12:00:00: record %03d: abcdefghijklmnopqrstuvwxyz0123456789\n", index);
        ASSERT_EQ(emit(file, "%s", expected + length), count);
        length += static_cast<size_t>(count);
        if (index == 0)
        {
            usleep(100000);
            size_t visible;
            ASSERT_NO_FATAL_FAILURE(read_file(path, actual, sizeof actual, &visible));
            if (!strcmp(mode, "native"))
            {
                EXPECT_EQ(visible, 0u);
            }
            else
            {
                ASSERT_EQ(visible, length);
                EXPECT_EQ(memcmp(actual, expected, length), 0);
            }
        }
    }
    if (strcmp(mode, "native"))
    {
        usleep(100000);
        size_t visible;
        ASSERT_NO_FATAL_FAILURE(read_file(path, actual, sizeof actual, &visible));
        ASSERT_EQ(visible, length);
        EXPECT_EQ(memcmp(actual, expected, length), 0);
    }
    EXPECT_GT(length, 4096u);
    EXPECT_EQ(fclose(file), 0);
    file = nullptr;
    size_t visible;
    ASSERT_NO_FATAL_FAILURE(read_file(path, actual, sizeof actual, &visible));
    ASSERT_EQ(visible, length);
    EXPECT_EQ(memcmp(actual, expected, length), 0);
}

TEST_F(hlds_async_log, reopening_does_not_retain_previous_stream_bytes)
{
    file = fopen(path, "wt");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(emit(file, "%s", "old log\n"), 8);
    EXPECT_EQ(fclose(file), 0);
    file = fopen(path, "wt");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(emit(file, "%s", "new log\n"), 8);
    EXPECT_EQ(fclose(file), 0);
    file = nullptr;
    size_t visible;
    ASSERT_NO_FATAL_FAILURE(read_file(path, actual, sizeof actual, &visible));
    ASSERT_EQ(visible, 8u);
    EXPECT_EQ(memcmp(actual, "new log\n", 8), 0);
}

TEST_F(hlds_async_log, unrelated_file_keeps_native_buffering)
{
    file = fopen(other, "wt");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(emit(file, "%s", "ordinary file\n"), 14);
    usleep(100000);
    size_t visible;
    ASSERT_NO_FATAL_FAILURE(read_file(other, actual, sizeof actual, &visible));
    EXPECT_EQ(visible, 0u);
    EXPECT_EQ(fclose(file), 0);
    file = nullptr;
    ASSERT_NO_FATAL_FAILURE(read_file(other, actual, sizeof actual, &visible));
    EXPECT_EQ(visible, 14u);
}

TEST_F(hlds_async_log, overflow_retains_fifo_prefix_without_blocking_producer)
{
    file = fopen(path, "wt");
    ASSERT_NE(file, nullptr);
    double start = now();
    for (int index = 0; index < 2048; ++index)
    {
        ASSERT_EQ(emit(file, "record %04d\n", index), 12);
    }
    const double elapsed = now() - start;
    EXPECT_LT(elapsed, 0.5);
    EXPECT_EQ(fclose(file), 0);
    file = nullptr;
    size_t length;
    ASSERT_NO_FATAL_FAILURE(read_file(path, actual, sizeof actual, &length));
    ASSERT_EQ(length, 1024u * 12);
    for (int index = 0; index < 1024; ++index)
    {
        char line[32];
        snprintf(line, sizeof line, "record %04d\n", index);
        EXPECT_EQ(memcmp(actual + index * 12, line, 12), 0) << "record " << index;
    }
    printf("overflow: 2048 calls in %.6fs; 1024 FIFO records retained\n", elapsed);
}
} // namespace tests
