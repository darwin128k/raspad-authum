/* Linux/glibc research prototype, NOT an Authum runtime component.
 * Interpose only formatted writes to LMMDDNNN.log in an explicit directory.
 * Open/close/explicit flush remain synchronous. One producer thread only.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

enum { CAPACITY = 1024, TEXT_SIZE = 2048 };
struct record { unsigned length; char text[TEXT_SIZE]; };
static struct record queue[CAPACITY];
static atomic_uint head, tail, requested, completed, dropped, io_errors;
static atomic_bool stop;
static FILE *(*real_open)(const char *, const char *);
static FILE *(*real_open64)(const char *, const char *);
static int (*real_close)(FILE *);
static int (*real_flush)(FILE *);
static int (*real_vprint)(FILE *, const char *, va_list);
static int (*real_checked)(FILE *, int, const char *, va_list);
static _Atomic(FILE *) tracked;
static const char *directory;
static char canonical_directory[PATH_MAX];
static pthread_t producer, worker;
static atomic_bool running;
static unsigned stall_ms;

static void resolve(void)
{
    if (real_open) return;
    real_open = dlsym(RTLD_NEXT, "fopen");
    real_open64 = dlsym(RTLD_NEXT, "fopen64");
    real_close = dlsym(RTLD_NEXT, "fclose");
    real_flush = dlsym(RTLD_NEXT, "fflush");
    real_vprint = dlsym(RTLD_NEXT, "vfprintf");
    real_checked = dlsym(RTLD_NEXT, "__vfprintf_chk");
}

static void pause_ms(unsigned ms)
{
    struct timespec delay = { ms / 1000, (long)(ms % 1000) * 1000000 };
    nanosleep(&delay, NULL);
}

static void flush_stream(void)
{
    if (real_flush(tracked) != 0) atomic_fetch_add(&io_errors, 1);
}

static void *consume(void *unused)
{
    (void)unused;
    /* Deterministic slow-storage simulation; this is never a producer wait. */
    if (stall_ms) pause_ms(stall_ms);
    bool dirty = false;
    for (;;) {
        unsigned request = atomic_load_explicit(&requested, memory_order_acquire);
        unsigned read = atomic_load_explicit(&tail, memory_order_relaxed);
        for (unsigned count = 0; count < CAPACITY; ++count) {
            if (read == atomic_load_explicit(&head, memory_order_acquire)) break;
            struct record *record = &queue[read % CAPACITY];
            if (fwrite(record->text, 1, record->length, tracked) != record->length)
                atomic_fetch_add(&io_errors, 1);
            dirty = true;
            atomic_store_explicit(&tail, ++read, memory_order_release);
        }
        if (dirty || request != atomic_load_explicit(&completed, memory_order_relaxed)) {
            flush_stream();
            dirty = false;
            atomic_store_explicit(&completed, request, memory_order_release);
        }
        if (atomic_load_explicit(&stop, memory_order_acquire) &&
            read == atomic_load_explicit(&head, memory_order_acquire)) break;
        pause_ms(10);
    }
    flush_stream();
    return NULL;
}

static bool matches(const char *path, const char *mode)
{
    if (!directory || mode[0] != 'w') return false;
    char absolute[PATH_MAX];
    if (!realpath(path, absolute)) return false;
    path = absolute;
    size_t size = strlen(directory);
    if (strncmp(path, directory, size) || path[size] != '/') return false;
    const char *name = path + size + 1;
    if (strlen(name) != 12 || name[0] != 'L' || strcmp(name + 8, ".log")) return false;
    for (int i = 1; i <= 7; ++i)
        if (name[i] < '0' || name[i] > '9') return false;
    return true;
}

static FILE *opened(FILE *file, const char *path, const char *mode)
{
    if (!file || running || !matches(path, mode)) return file;
    tracked = file;
    producer = pthread_self();
    atomic_store(&head, 0);
    atomic_store(&tail, 0);
    atomic_store(&requested, 0);
    atomic_store(&completed, 0);
    atomic_store(&stop, false);
    if (pthread_create(&worker, NULL, consume, NULL) == 0) running = true;
    else tracked = NULL; /* Prototype falls back to original behavior on startup failure. */
    return file;
}

FILE *fopen(const char *path, const char *mode)
{
    resolve();
    return opened(real_open(path, mode), path, mode);
}

FILE *fopen64(const char *path, const char *mode)
{
    resolve();
    return opened(real_open64(path, mode), path, mode);
}

static int enqueue(const char *fmt, va_list args)
{
    /* Multiple producers are outside the prototype's contract; no sync fallback. */
    if (!pthread_equal(pthread_self(), producer)) {
        atomic_fetch_add(&dropped, 1);
        return 0;
    }
    char text[TEXT_SIZE];
    int length = vsnprintf(text, sizeof text, fmt, args);
    if (length < 0) return length;
    if ((unsigned)length >= sizeof text) {
        atomic_fetch_add(&dropped, 1); /* Drop whole records, never silently truncate. */
        return length;
    }
    unsigned write = atomic_load_explicit(&head, memory_order_relaxed);
    if (write - atomic_load_explicit(&tail, memory_order_acquire) >= CAPACITY) {
        atomic_fetch_add(&dropped, 1);
        return length;
    }
    struct record *record = &queue[write % CAPACITY];
    record->length = (unsigned)length;
    memcpy(record->text, text, (size_t)length);
    atomic_store_explicit(&head, write + 1, memory_order_release);
    return length;
}

int vfprintf(FILE *file, const char *fmt, va_list args)
{
    resolve();
    if (running && file == tracked) return enqueue(fmt, args);
    return real_vprint(file, fmt, args);
}

int __vfprintf_chk(FILE *file, int flag, const char *fmt, va_list args)
{
    resolve();
    if (running && file == tracked) return enqueue(fmt, args);
    return real_checked(file, flag, fmt, args);
}

int fflush(FILE *file)
{
    resolve();
    if (running && (file == tracked || !file)) {
        unsigned request = atomic_fetch_add_explicit(&requested, 1, memory_order_release) + 1;
        while (atomic_load_explicit(&completed, memory_order_acquire) != request) pause_ms(1);
        if (file) return 0;
    }
    return real_flush(file);
}

static void drain(void)
{
    if (!running) return;
    atomic_store_explicit(&stop, true, memory_order_release);
    pthread_join(worker, NULL);
    running = false;
    tracked = NULL;
}

int fclose(FILE *file)
{
    resolve();
    if (running && file == tracked) drain();
    return real_close(file);
}

__attribute__((constructor)) static void init(void)
{
    resolve();
    const char *configured = getenv("AUTHUM_ASYNC_LOG_PROBE_DIR");
    if (configured && realpath(configured, canonical_directory)) directory = canonical_directory;
    const char *stall = getenv("AUTHUM_ASYNC_LOG_PROBE_STALL_MS");
    if (stall) stall_ms = (unsigned)strtoul(stall, NULL, 10);
    if (!atomic_is_lock_free(&head)) directory = NULL;
}

__attribute__((destructor)) static void finish(void)
{
    drain();
    if (directory)
        fprintf(stderr, "async-log-probe: dropped=%u io_errors=%u\n",
                atomic_load(&dropped), atomic_load(&io_errors));
}
