#ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#endif
#include <authum/null.h>
#include <authum/host/http.h>
#include <authum/host/threads.h>
#include <authum/host/http/internal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

authum_i32_t
available_port(void)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        return -1;
    }
    struct sockaddr_in address = {.sin_family = AF_INET,
                                  .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)}};
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

void
close_in_log(const authum_char_t *line, authum_ptr user)
{
    (void)line;
    close_during_log_t *context = static_cast<close_during_log_t *>(user);
    ++context->calls;
    authum_http_close(context->service, context->server);
}

static authum_thread_owner_t thread_owner;
static authum_threads_t threads;
static authum_http_owner_t owner, foreign_owner;
static authum_http_service_t service, foreign_service;
static authum_http_options_t options;
static char directory[64], filename[256], error[512];
static int root_fd, client_fd;
static authum_http_server_t *server;

void
write_resource(const char *name, const char *text)
{
    char path[256];
    snprintf(path, sizeof path, "%s/%s", directory, name);
    FILE *file = fopen(path, "wb");
    ASSERT_NE(file, authum_null);
    EXPECT_EQ(fwrite(text, 1, strlen(text), file), strlen(text));
    EXPECT_EQ(fclose(file), 0);
}

namespace tests
{

class authum_http : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        authum_threads_host_init(&thread_owner, &threads);
        authum_http_host_init(&owner, &threads, &service);
        authum_http_host_init(&foreign_owner, &threads, &foreign_service);
        server = authum_null;
        root_fd = client_fd = -1;
        strcpy(directory, "/tmp/authum-http-XXXXXX");
        ASSERT_NE(mkdtemp(directory), authum_null);
        snprintf(filename, sizeof filename, "%s/shared.bsp", directory);
        ASSERT_NO_FATAL_FAILURE(write_resource("shared.bsp", "shared-http"));
        char path[256];
        snprintf(path, sizeof path, "%s/maps", directory);
        ASSERT_EQ(mkdir(path, 0700), 0);
        ASSERT_NO_FATAL_FAILURE(write_resource("maps/nested.bsp", "nested"));
        snprintf(path, sizeof path, "%s/link.bsp", directory);
        ASSERT_EQ(symlink("shared.bsp", path), 0);
        snprintf(path, sizeof path, "%s/alias", directory);
        ASSERT_EQ(symlink("maps", path), 0);
        snprintf(path, sizeof path, "%s/.hidden.bsp", directory);
        ASSERT_EQ(link(filename, path), 0);
        snprintf(path, sizeof path, "%s/fifo.bsp", directory);
        ASSERT_EQ(mkfifo(path, 0600), 0);
        root_fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        ASSERT_GE(root_fd, 0);
        authum_http_options_init(&options);
        ASSERT_TRUE(authum_http_source_add(&options, directory));
        strcpy(options.allow_extensions, "bsp");
        strcpy(options.root_extensions, "bsp");
        strcpy(options.allow_dirs, "maps alias");
        options.max_clients = 1;
    }

    void
    TearDown() override
    {
        if (client_fd >= 0)
        {
            close(client_fd);
        }
        authum_threads_host_stop_owner(&thread_owner);
        authum_http_host_drop_owner(&owner);
        authum_http_host_drop_owner(&foreign_owner);
        authum_threads_host_drop_owner(&thread_owner);
        if (root_fd >= 0)
        {
            close(root_fd);
        }
        const char *files[] = {"maps/nested.bsp", "link.bsp", "alias",
                               ".hidden.bsp",     "fifo.bsp", "shared.bsp"};
        char path[256];
        for (const char *file : files)
        {
            snprintf(path, sizeof path, "%s/%s", directory, file);
            unlink(path);
        }
        snprintf(path, sizeof path, "%s/maps", directory);
        rmdir(path);
        rmdir(directory);
    }
};

void
open_server(void)
{
    options.port = available_port();
    ASSERT_GT(options.port, 0);
    server = authum_http_open(&service, &options, error, sizeof error);
    ASSERT_NE(server, authum_null) << error;
}

void
request_resource(char *response, size_t capacity)
{
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(client_fd, 0);
    struct timeval timeout = {.tv_sec = 2};
    ASSERT_EQ(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout), 0);
    struct sockaddr_in address = {.sin_family = AF_INET,
                                  .sin_port = htons(options.port),
                                  .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)}};
    ASSERT_EQ(connect(client_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof address), 0);
    const char *request = "GET /shared.bsp HTTP/1.1\r\nHost: localhost\r\n\r\n";
    ASSERT_GT(send(client_fd, request, strlen(request), MSG_NOSIGNAL), 0);
    size_t received = 0;
    for (;;)
    {
        ASSERT_LT(received, capacity - 1);
        ssize_t size = recv(client_fd, response + received, capacity - 1 - received, 0);
        ASSERT_GE(size, 0);
        if (!size)
        {
            break;
        }
        received += static_cast<size_t>(size);
    }
    response[received] = 0;
    close(client_fd);
    client_fd = -1;
}

TEST_F(authum_http, rejects_relative_and_null_sources)
{
    EXPECT_FALSE(authum_http_source_add(&options, "relative/tree"));
    EXPECT_FALSE(authum_http_source_add(&options, authum_null));
}

TEST_F(authum_http, enforces_source_count_limit)
{
    for (authum_usize_t index = 1; index < AUTHUM_HTTP_MAX_SOURCES; ++index)
    {
        ASSERT_TRUE(authum_http_source_add(&options, directory));
    }
    EXPECT_FALSE(authum_http_source_add(&options, directory));
}

TEST_F(authum_http, missing_service_reports_unsupported)
{
    EXPECT_EQ(authum_http_open(authum_null, &options, error, sizeof error), authum_null);
    EXPECT_NE(strstr(error, "on this platform"), authum_null);
}

TEST_F(authum_http, refuses_unterminated_bind_address)
{
    memset(options.bind, 'x', sizeof options.bind);
    EXPECT_EQ(authum_http_open(&service, &options, error, sizeof error), authum_null);
}

TEST_F(authum_http, foreign_owner_cannot_inspect_or_close_server)
{
    ASSERT_NO_FATAL_FAILURE(open_server());
    const auto memory = authum_http_get_memory_usage(&service, server);
    ASSERT_GT(memory, 8192u);
    EXPECT_EQ(authum_http_get_memory_usage(&foreign_service, server), 0u);
    authum_http_shutdown(&foreign_service, server);
    authum_http_close(&foreign_service, server);
    EXPECT_EQ(authum_http_get_memory_usage(&service, server), memory);
}

TEST_F(authum_http, opening_server_copies_options)
{
    ASSERT_NO_FATAL_FAILURE(open_server());
    const auto memory = authum_http_get_memory_usage(&service, server);
    options.max_clients = 256;
    EXPECT_EQ(authum_http_get_memory_usage(&service, server), memory);
}

TEST_F(authum_http, serves_complete_static_resource)
{
    ASSERT_NO_FATAL_FAILURE(open_server());
    char response[512] = {0};
    ASSERT_NO_FATAL_FAILURE(request_resource(response, sizeof response));
    EXPECT_NE(strstr(response, "200 OK"), authum_null);
    EXPECT_NE(strstr(response, "\r\n\r\nshared-http"), authum_null);
}

TEST_F(authum_http, callback_may_close_server_during_log_drain)
{
    ASSERT_NO_FATAL_FAILURE(open_server());
    char response[512] = {0};
    ASSERT_NO_FATAL_FAILURE(request_resource(response, sizeof response));
    close_during_log_t callback = {&service, server, 0};
    EXPECT_EQ(authum_http_drain(&service, server, close_in_log, &callback, 1000), 1u);
    EXPECT_EQ(callback.calls, 1u);
    EXPECT_EQ(owner.head, authum_null);
    EXPECT_EQ(thread_owner.count, 0u);
}

TEST_F(authum_http, owner_cleanup_reclaims_forgotten_server)
{
    ASSERT_NO_FATAL_FAILURE(open_server());
    authum_threads_host_stop_owner(&thread_owner);
    authum_http_host_drop_owner(&owner);
    EXPECT_EQ(owner.head, authum_null);
    EXPECT_EQ(thread_owner.count, 0u);
}

TEST_F(authum_http, opens_allowed_nested_resource)
{
    const char *url = "/maps/nested.bsp";
    off_t size = -1;
    int fd = authum_http_resource_open(root_fd, &options, url, strlen(url), &size);
    ASSERT_GE(fd, 0);
    EXPECT_EQ(size, 6);
    EXPECT_EQ(close(fd), 0);
}

class authum_http_denied_path : public authum_http,
                                public ::testing::WithParamInterface<const char *>
{
};

TEST_P(authum_http_denied_path, refuses_unsafe_path_without_changing_size)
{
    off_t size = -1;
    const char *url = GetParam();
    int fd = authum_http_resource_open(root_fd, &options, url, strlen(url), &size);
    EXPECT_LT(fd, 0);
    EXPECT_EQ(size, -1);
    if (fd >= 0)
    {
        close(fd);
    }
}

INSTANTIATE_TEST_SUITE_P(unsafe_paths, authum_http_denied_path,
                         ::testing::Values("/link.bsp", "/alias/nested.bsp", "/.hidden.bsp",
                                           "/fifo.bsp", "/maps/../shared.bsp", "/maps//nested.bsp",
                                           "/maps/.nested.bsp", "/maps/nested.txt",
                                           "/maps/nested.bsp/", "/shared.bsp?x=1"));

TEST_F(authum_http, file_size_limit_accepts_exact_boundary)
{
    const char *url = "/maps/nested.bsp";
    off_t size = -1;
    options.max_file_bytes = 5;
    EXPECT_LT(authum_http_resource_open(root_fd, &options, url, strlen(url), &size), 0);
    options.max_file_bytes = 6;
    int fd = authum_http_resource_open(root_fd, &options, url, strlen(url), &size);
    ASSERT_GE(fd, 0);
    EXPECT_EQ(size, 6);
    close(fd);
}

TEST_F(authum_http, deny_prefix_overrides_allowed_directory)
{
    strcpy(options.deny_prefixes, "maps");
    const char *url = "/maps/nested.bsp";
    off_t size = -1;
    EXPECT_LT(authum_http_resource_open(root_fd, &options, url, strlen(url), &size), 0);
}

TEST_F(authum_http, empty_directory_allow_list_refuses_nested_file)
{
    options.allow_dirs[0] = 0;
    const char *url = "/maps/nested.bsp";
    off_t size = -1;
    EXPECT_LT(authum_http_resource_open(root_fd, &options, url, strlen(url), &size), 0);
}

TEST_F(authum_http, root_file_requires_root_extension_permission)
{
    options.root_extensions[0] = 0;
    const char *url = "/shared.bsp";
    off_t size = -1;
    EXPECT_LT(authum_http_resource_open(root_fd, &options, url, strlen(url), &size), 0);
}

} // namespace tests
