#define _GNU_SOURCE
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern int __vfprintf_chk(FILE *, int, const char *, va_list);

/* Same libc entry point used by this HLDS filesystem's FS_vfprintf. */
static int emit(FILE *file, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = __vfprintf_chk(file, 1, fmt, args);
    va_end(args);
    return result;
}

static double now(void)
{
    struct timespec value;
    clock_gettime(CLOCK_MONOTONIC, &value);
    return value.tv_sec + value.tv_nsec / 1e9;
}

static size_t read_file(const char *path, char *data, size_t capacity)
{
    FILE *file = fopen(path, "rb");
    assert(file);
    size_t length = fread(data, 1, capacity, file);
    assert(!ferror(file));
    assert(fclose(file) == 0);
    return length;
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    char path[4096], other[4096], actual[32768], expected[32768];
    snprintf(path, sizeof path, "%s/L1026000.log", argv[1]);
    snprintf(other, sizeof other, "%s/unrelated.txt", argv[1]);
    FILE *file = fopen(path, "wt");
    assert(file);
    if (!strcmp(argv[2], "overflow")) {
        double start = now();
        for (int i = 0; i < 2048; ++i)
            assert(emit(file, "record %04d\n", i) == 12);
        double elapsed = now() - start;
        /* The worker is stalled for 1000ms. Producer must finish well before it. */
        assert(elapsed < 0.5);
        assert(fclose(file) == 0);
        size_t length = read_file(path, actual, sizeof actual);
        assert(length == 1024 * 12);
        for (int i = 0; i < 1024; ++i) {
            char line[32];
            snprintf(line, sizeof line, "record %04d\n", i);
            assert(memcmp(actual + i * 12, line, 12) == 0);
        }
        printf("overflow: 2048 calls in %.6fs; 1024 FIFO records retained\n", elapsed);
        return 0;
    }
    size_t length = 0;
    for (int i = 0; i < 100; ++i) {
        int count = snprintf(expected + length, sizeof expected - length,
            "L 10/26/2026 - 12:00:00: record %03d: abcdefghijklmnopqrstuvwxyz0123456789\n", i);
        assert(emit(file, "%s", expected + length) == count);
        length += (size_t)count;
        if (i == 0) {
            usleep(100000);
            size_t visible = read_file(path, actual, sizeof actual);
            if (!strcmp(argv[2], "native")) assert(visible == 0);
            else assert(visible == length && !memcmp(actual, expected, length));
        }
    }
    if (strcmp(argv[2], "native")) {
        usleep(100000);
        assert(read_file(path, actual, sizeof actual) == length);
        assert(!memcmp(actual, expected, length));
    }
    assert(length > 4096);
    assert(fclose(file) == 0);
    assert(read_file(path, actual, sizeof actual) == length);
    assert(!memcmp(actual, expected, length));

    /* Close/reopen same path; no queued data may leak to the new stream. */
    file = fopen(path, "wt");
    assert(file);
    assert(emit(file, "%s", "new log\n") == 8);
    assert(fclose(file) == 0);
    assert(read_file(path, actual, sizeof actual) == 8);
    assert(!memcmp(actual, "new log\n", 8));

    /* Files outside the filename filter retain native buffering. */
    file = fopen(other, "wt");
    assert(file);
    assert(emit(file, "%s", "ordinary file\n") == 14);
    usleep(100000);
    assert(read_file(other, actual, sizeof actual) == 0);
    assert(fclose(file) == 0);
    assert(read_file(other, actual, sizeof actual) == 14);
    printf("%s: visibility, exact bytes across 4096, close/reopen and isolation OK\n", argv[2]);
    return 0;
}
