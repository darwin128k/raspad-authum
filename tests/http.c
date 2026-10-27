#define _GNU_SOURCE
#include <authum/host/http.h>
#include <authum/host/threads.h>
#include <authum/host/clock.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REQUIRE(condition)                                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            fprintf(stderr, "http:%d: %s\n", __LINE__, #condition);                                \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static authum_i32_t
available_port(void)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        return -1;
    }
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    socklen_t size = sizeof address;
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof address) ||
        getsockname(socket_fd, (struct sockaddr *)&address, &size))
    {
        close(socket_fd);
        return -1;
    }
    close(socket_fd);
    return ntohs(address.sin_port);
}

typedef struct close_during_log
{
    const authum_http_service_t *service;
    authum_http_server_t *server;
    authum_usize_t calls;
} close_during_log_t;

static void
close_in_log(const authum_char_t *line, authum_ptr user)
{
    (void)line;
    close_during_log_t *context = user;
    ++context->calls;
    authum_http_close(context->service, context->server);
}

int
main(void)
{
    const authum_clock_t *clock = authum_clock_host();
    authum_i64_t before = authum_clock_monotonic_ms(clock);
    REQUIRE(before > 0);
    usleep(2000);
    REQUIRE(authum_clock_monotonic_ms(clock) >= before);
    REQUIRE(authum_clock_monotonic_ms(NULL) == 0);
    authum_clock_t short_clock = {.size = sizeof(authum_usize_t)};
    REQUIRE(authum_clock_monotonic_ms(&short_clock) == 0);

    authum_thread_owner_t thread_owner;
    authum_threads_t threads;
    authum_threads_host_init(&thread_owner, &threads);
    authum_http_owner_t owner, foreign_owner;
    authum_http_service_t service, foreign_service;
    authum_http_host_init(&owner, &threads, &service);
    authum_http_host_init(&foreign_owner, &threads, &foreign_service);

    authum_http_options_t options;
    authum_http_options_init(&options);
    REQUIRE(options.size == sizeof options);
    REQUIRE(!authum_http_source_add(&options, "relative/tree"));
    REQUIRE(!authum_http_source_add(&options, NULL));
    char directory[] = "/tmp/authum-http-XXXXXX";
    REQUIRE(mkdtemp(directory));
    char filename[256];
    snprintf(filename, sizeof filename, "%s/shared.bsp", directory);
    FILE *resource = fopen(filename, "wb");
    REQUIRE(resource);
    REQUIRE(fwrite("shared-http", 1, 11, resource) == 11);
    REQUIRE(!fclose(resource));
    for (authum_usize_t index = 0; index < AUTHUM_HTTP_MAX_SOURCES; ++index)
    {
        REQUIRE(authum_http_source_add(&options, directory));
    }
    REQUIRE(!authum_http_source_add(&options, directory));
    options.source_count = 1;
    strcpy(options.allow_extensions, "bsp");
    strcpy(options.root_extensions, "bsp");
    options.port = available_port();
    REQUIRE(options.port > 0);
    options.max_clients = 1;
    char error[512];
    REQUIRE(!authum_http_open(NULL, &options, error, sizeof error));
    REQUIRE(strstr(error, "update Authum"));

    options.size = 0;
    REQUIRE(!authum_http_open(&service, &options, error, sizeof error));
    options.size = sizeof options;
    memset(options.bind, 'x', sizeof options.bind);
    REQUIRE(!authum_http_open(&service, &options, error, sizeof error));
    strcpy(options.bind, "127.0.0.1");
    authum_http_server_t *server = authum_http_open(&service, &options, error, sizeof error);
    REQUIRE(server);
    authum_usize_t memory = authum_http_memory_usage(&service, server);
    REQUIRE(memory > 8192);
    REQUIRE(authum_http_memory_usage(&foreign_service, server) == 0);
    authum_http_shutdown(&foreign_service, server);
    authum_http_close(&foreign_service, server);
    REQUIRE(authum_http_memory_usage(&service, server) == memory);
    options.max_clients = 256;
    REQUIRE(authum_http_memory_usage(&service, server) == memory);

    int client = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(client >= 0);
    struct timeval timeout = {.tv_sec = 2};
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    struct sockaddr_in address = {.sin_family = AF_INET,
                                  .sin_port = htons(options.port),
                                  .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    REQUIRE(!connect(client, (struct sockaddr *)&address, sizeof address));
    const char *request = "GET /shared.bsp HTTP/1.1\r\nHost: localhost\r\n\r\n";
    REQUIRE(send(client, request, strlen(request), MSG_NOSIGNAL) > 0);
    char response[512] = {0};
    size_t received = 0;
    for (;;)
    {
        REQUIRE(received < sizeof response - 1);
        ssize_t size = recv(client, response + received, sizeof response - 1 - received, 0);
        REQUIRE(size >= 0);
        if (!size)
        {
            break;
        }
        received += (size_t)size;
    }
    REQUIRE(strstr(response, "200 OK"));
    REQUIRE(strstr(response, "\r\n\r\nshared-http"));
    close(client);

    /* A log callback may close its server: defer destruction until drain
     * returns and do not invoke further callbacks with the old user context. */
    close_during_log_t callback = {&service, server, 0};
    REQUIRE(authum_http_drain(&service, server, close_in_log, &callback, 1000) == 1);
    REQUIRE(callback.calls == 1);
    REQUIRE(owner.head == NULL);
    REQUIRE(thread_owner.count == 0);
    options.max_clients = 1;
    server = authum_http_open(&service, &options, error, sizeof error);
    REQUIRE(server);

    /* The plugin intentionally forgets its handle, as on failed start.
     * Host cleanup must join the worker and reclaim listener/source handles. */
    authum_threads_host_stop_owner(&thread_owner);
    authum_http_host_drop_owner(&owner);
    REQUIRE(owner.head == NULL);
    REQUIRE(thread_owner.count == 0);
    authum_threads_host_drop_owner(&thread_owner);
    authum_http_host_drop_owner(&foreign_owner);
    REQUIRE(!unlink(filename));
    REQUIRE(!rmdir(directory));
    puts("PASS: monotonic clock, constructor validation, source limits, owner isolation, "
         "immutable options, real HTTP response, forgotten-server cleanup");
    return 0;
}
