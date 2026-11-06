#include <authum/null.h>
#include <authum/host/http.h>
#include <authum/host/threads.h>
#include <authum/host/http/internal.h>
#include <gtest/gtest.h>
#include <lh/os/net.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#    include <direct.h>
#    define test_mkdir(path) _mkdir(path)
#    define test_rmdir(path) _rmdir(path)
#else
#    include <sys/stat.h>
#    include <unistd.h>
#    define test_mkdir(path) mkdir(path, 0700)
#    define test_rmdir(path) rmdir(path)
#endif

/* A size no resource has: what a refused open must leave untouched. */
static const lh_u64_t k_unset = ~0ULL;

/* What the worker does with a request target: decode and check it, then open
 * the checked path. @p reason (optional) receives why it was refused. */
static bool
resource_open(const lh_os_fs_file_t *root, const authum_http_options_t *options, const char *url,
              size_t length, lh_u64_t *size, const char **reason = nullptr)
{
    char path[2048];
    const char *why = nullptr;
    lh_os_fs_file_t file;
    lh_os_fs_file_init(&file);
    bool opened = false;
    if (authum_http_resource_check(options, url, length, path, sizeof path, &why))
    {
        opened = authum_http_resource_open(&file, root, options, path, size, &why);
    }
    if (reason)
    {
        *reason = why;
    }
    lh_os_fs_file_deinit(&file);
    return opened;
}

/* 127.0.0.1:port. */
static lh_net_ip4_socket_addr_t
loopback(lh_net_port_t port)
{
    lh_net_ip4_t ip;
    lh_net_ip4_socket_addr_t address;

    EXPECT_TRUE(lh_net_ip4_parse("127.0.0.1", 9, &ip));
    lh_net_ip4_socket_addr_init(&address, &ip, port);
    return address;
}

authum_i32_t
available_port(void)
{
    lh_net_ip4_socket_addr_t address = loopback(0);
    lh_os_net_socket_t socket;

    lh_os_net_socket_init(&socket);
    if (!lh_os_net_socket_open(&socket, lh_net_socket_type_tcp))
    {
        return -1;
    }
    if (!lh_os_net_socket_bind(&socket, &address) ||
        !lh_os_net_socket_get_local_addr(&socket, &address))
    {
        lh_os_net_socket_close(&socket);
        return -1;
    }
    lh_os_net_socket_close(&socket);
    return lh_net_ip4_socket_addr_get_port(&address);
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

typedef struct collected_log
{
    authum_bool_t named;
} collected_log_t;

void
collect_line(const authum_char_t *line, authum_ptr user)
{
    collected_log_t *context = static_cast<collected_log_t *>(user);
    if (strstr(line, "module=TESTMOD"))
    {
        context->named = authum_bool_true;
    }
}

static authum_thread_owner_t thread_owner;
static authum_threads_t threads;
static authum_http_owner_t owner, foreign_owner;
static authum_http_service_t service, foreign_service;
static authum_http_options_t options;
static char directory[200], filename[256], error[512];
static lh_os_fs_file_t source_root;
static lh_os_net_socket_t client_fd;
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
        authum_http_host_init(&owner, &service);
        authum_http_host_init(&foreign_owner, &foreign_service);
        ASSERT_TRUE(lh_os_net_init());
        server = authum_null;
        lh_os_fs_file_init(&source_root);
        lh_os_net_socket_init(&client_fd);
#if defined(_WIN32)
        /* An absolute directory of this run: sources must be absolute. */
        test_mkdir("authum-http-test");
        ASSERT_NE(_fullpath(directory, "authum-http-test", sizeof directory), authum_null);
        for (char *p = directory; *p; ++p)
        {
            *p = *p == '\\' ? '/' : *p;
        }
#else
        strcpy(directory, "/tmp/authum-http-XXXXXX");
        ASSERT_NE(mkdtemp(directory), authum_null);
#endif
        snprintf(filename, sizeof filename, "%s/shared.bsp", directory);
        ASSERT_NO_FATAL_FAILURE(write_resource("shared.bsp", "shared-http"));
        char path[256];
        snprintf(path, sizeof path, "%s/maps", directory);
        ASSERT_EQ(test_mkdir(path), 0);
        ASSERT_NO_FATAL_FAILURE(write_resource("maps/nested.bsp", "nested"));
#if !defined(_WIN32)
        /* Links and FIFOs need privileges on Windows: the refusals hold there unseen. */
        snprintf(path, sizeof path, "%s/link.bsp", directory);
        ASSERT_EQ(symlink("shared.bsp", path), 0);
        snprintf(path, sizeof path, "%s/alias", directory);
        ASSERT_EQ(symlink("maps", path), 0);
        snprintf(path, sizeof path, "%s/.hidden.bsp", directory);
        ASSERT_EQ(link(filename, path), 0);
        snprintf(path, sizeof path, "%s/fifo.bsp", directory);
        ASSERT_EQ(mkfifo(path, 0600), 0);
#endif
        ASSERT_TRUE(lh_os_fs_file_try_open_root_cstr(&source_root, directory));
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
        lh_os_net_socket_close(&client_fd);
        authum_threads_host_stop_owner(&thread_owner);
        authum_http_host_drop_owner(&owner);
        authum_http_host_drop_owner(&foreign_owner);
        authum_threads_host_drop_owner(&thread_owner);
        lh_os_fs_file_deinit(&source_root);
        const char *files[] = {"maps/nested.bsp", "link.bsp", "alias",
                               ".hidden.bsp",     "fifo.bsp", "shared.bsp"};
        char path[256];
        for (const char *file : files)
        {
            snprintf(path, sizeof path, "%s/%s", directory, file);
            remove(path);
        }
        snprintf(path, sizeof path, "%s/maps", directory);
        test_rmdir(path);
        test_rmdir(directory);
        lh_os_net_deinit();
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
    const lh_net_ip4_socket_addr_t address =
        loopback(static_cast<lh_net_port_t>(options.port));
    ASSERT_TRUE(lh_os_net_socket_open(&client_fd, lh_net_socket_type_tcp));
    ASSERT_TRUE(lh_os_net_socket_connect(&client_fd, &address));
    const char *request = "GET /shared.bsp HTTP/1.1\r\nHost: localhost\r\n\r\n";
    ASSERT_GT(lh_os_net_socket_send(&client_fd, request, strlen(request)), 0);
    lh_os_net_socket_t *const readable[1] = {&client_fd};
    size_t received = 0;
    for (;;)
    {
        ASSERT_LT(received, capacity - 1);
        /* Two seconds for each piece of the answer. */
        ASSERT_GT(lh_os_net_socket_wait(readable, 1U, nullptr, 0U, 2000U), 0);
        lh_ssize_t size =
            lh_os_net_socket_recv(&client_fd, response + received, capacity - 1 - received);
        ASSERT_GE(size, 0);
        if (!size)
        {
            break;
        }
        received += static_cast<size_t>(size);
    }
    response[received] = 0;
    lh_os_net_socket_close(&client_fd);
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

TEST_F(authum_http, access_log_names_the_owner)
{
    /* The module that opened the server is named in every access line. */
    owner.tag = "TESTMOD";
    ASSERT_NO_FATAL_FAILURE(open_server());
    char response[512] = {0};
    ASSERT_NO_FATAL_FAILURE(request_resource(response, sizeof response));
    collected_log_t collected = {authum_bool_false};
    EXPECT_GE(authum_http_drain(&service, server, collect_line, &collected, 1000), 1u);
    EXPECT_TRUE(collected.named);
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
    EXPECT_EQ(lh_os_thread_group_get_count(&thread_owner.group), 0u);
}

/* A module built before struct_size: its struct starts with the bind text. */
TEST_F(authum_http, old_layout_options_are_read_with_defaults)
{
    options.port = available_port();
    ASSERT_GT(options.port, 0);
    /* Exactly the old size on the heap: reading past it is an ASan error. */
    auto *old = static_cast<authum_http_options_v0_t *>(malloc(sizeof(authum_http_options_v0_t)));
    ASSERT_NE(old, authum_null);
    memset(old, 0, sizeof *old);
    memcpy(old->bind, options.bind, sizeof old->bind);
    old->port = options.port;
    memcpy(old->sources, options.sources, sizeof old->sources);
    old->source_count = options.source_count;
    strcpy(old->allow_dirs, options.allow_dirs);
    strcpy(old->allow_extensions, options.allow_extensions);
    strcpy(old->root_extensions, options.root_extensions);
    old->max_clients = 3;
    old->per_ip = 5;
    old->requests_per_second = 7;
    old->timeout_seconds = 11;
    old->cpu = -1;
    old->access_log = 1;
    old->bytes_per_second = 123456;
    old->max_file_bytes = 654321;
    server = authum_http_open(&service, reinterpret_cast<const authum_http_options_t *>(old),
                              error, sizeof error);
    free(old);
    ASSERT_NE(server, authum_null) << error;
    EXPECT_EQ(server->options.port, options.port);
    EXPECT_EQ(server->options.source_count, 1u);
    EXPECT_STREQ(server->options.allow_dirs, "maps alias");
    EXPECT_EQ(server->options.max_clients, 3);
    EXPECT_EQ(server->options.per_ip, 5);
    EXPECT_EQ(server->options.requests_per_second, 7);
    EXPECT_EQ(server->options.timeout_seconds, 11);
    EXPECT_EQ(server->options.access_log, 1);
    EXPECT_EQ(server->options.bytes_per_second, 123456u);
    EXPECT_EQ(server->options.max_file_bytes, 654321u);
    EXPECT_EQ(server->options.per_ip_pending, 4);
    EXPECT_EQ(server->options.trusted_proxies[0], 0);
    char response[512] = {0};
    ASSERT_NO_FATAL_FAILURE(request_resource(response, sizeof response));
    EXPECT_NE(strstr(response, "\r\n\r\nshared-http"), authum_null);
}

TEST_F(authum_http, smaller_sized_options_use_defaults_for_missing_fields)
{
    options.port = available_port();
    options.per_ip_pending = 9;
    strcpy(options.trusted_proxies, "not an address");
    const size_t size = AUTHUM_HTTP_OPTIONS_MIN_SIZE;
    auto *small = static_cast<unsigned char *>(malloc(size));
    ASSERT_NE(small, authum_null);
    memcpy(small, &options, size);
    const authum_u32_t declared = static_cast<authum_u32_t>(size);
    memcpy(small, &declared, sizeof declared);
    server = authum_http_open(&service, reinterpret_cast<const authum_http_options_t *>(small),
                              error, sizeof error);
    free(small);
    ASSERT_NE(server, authum_null) << error;
    EXPECT_EQ(server->options.port, options.port);
    EXPECT_EQ(server->options.per_ip_pending, 4);
    EXPECT_EQ(server->options.trusted_proxies[0], 0);
}

TEST_F(authum_http, larger_options_from_newer_module_are_accepted)
{
    options.port = available_port();
    options.per_ip_pending = 9;
    const size_t size = sizeof options + 64;
    auto *large = static_cast<unsigned char *>(malloc(size));
    ASSERT_NE(large, authum_null);
    memcpy(large, &options, sizeof options);
    memset(large + sizeof options, 0xA5, 64);
    const authum_u32_t declared = static_cast<authum_u32_t>(size);
    memcpy(large, &declared, sizeof declared);
    server = authum_http_open(&service, reinterpret_cast<const authum_http_options_t *>(large),
                              error, sizeof error);
    free(large);
    ASSERT_NE(server, authum_null) << error;
    EXPECT_EQ(server->options.per_ip_pending, 9);
}

TEST_F(authum_http, options_init_sets_size_and_defaults)
{
    EXPECT_EQ(options.struct_size, sizeof options);
    EXPECT_EQ(options.per_ip_pending, 4);
}

TEST_F(authum_http, too_small_struct_size_is_refused)
{
    options.port = available_port();
    for (authum_u32_t size : {0u, 16u, static_cast<authum_u32_t>(AUTHUM_HTTP_OPTIONS_MIN_SIZE - 1)})
    {
        options.struct_size = size;
        EXPECT_EQ(authum_http_open(&service, &options, error, sizeof error), authum_null);
        EXPECT_NE(strstr(error, "struct_size"), authum_null) << error;
    }
}

static unsigned stall_on_stop_ms;

static void
stall_on_stop(authum_http_server_t *server)
{
    if (stall_on_stop_ms && server->loop && lh_rail_loop_is_stopping(server->loop))
    {
        lh_os_thread_sleep(stall_on_stop_ms);
    }
}

/* A worker stuck past the stop timeout frees its server when the call
 * returns; the reaper then joins it without waiting. */
TEST_F(authum_http, detached_worker_frees_itself_and_is_reaped)
{
    for (unsigned owner_waits : {0u, 1u})
    {
        ASSERT_NO_FATAL_FAILURE(open_server());
        stall_on_stop_ms = 1500;
        authum_http_test_step_hook = stall_on_stop;
        const authum_i64_t start = authum_http_now_ms();
        authum_http_shutdown(&service, server);
        const authum_i64_t waited = authum_http_now_ms() - start;
        EXPECT_GE(waited, 900);
        EXPECT_LT(waited, 1400);
        if (owner_waits)
        {
            /* The worker finishes first: the owner frees the server. */
            lh_os_thread_sleep(800);
        }
        authum_http_close(&service, server);
        server = authum_null;
        EXPECT_EQ(owner.head, authum_null);
        /* A worker that ended while the owner slept is reaped at once. */
        authum_usize_t left = authum_http_host_reap();
        EXPECT_EQ(left, owner_waits ? 0u : 1u);
        while (left && authum_http_now_ms() - start < 4000)
        {
            const authum_i64_t before = authum_http_now_ms();
            left = authum_http_host_reap();
            /* Never waits: only threads that said they are done are joined. */
            EXPECT_LT(authum_http_now_ms() - before, 50);
            lh_os_thread_sleep(1);
        }
        EXPECT_EQ(left, 0u);
        authum_http_test_step_hook = authum_null;
        stall_on_stop_ms = 0;
    }
}

TEST_F(authum_http, owner_cleanup_reclaims_forgotten_server)
{
    ASSERT_NO_FATAL_FAILURE(open_server());
    authum_threads_host_stop_owner(&thread_owner);
    authum_http_host_drop_owner(&owner);
    EXPECT_EQ(owner.head, authum_null);
    EXPECT_EQ(lh_os_thread_group_get_count(&thread_owner.group), 0u);
}

TEST_F(authum_http, opens_allowed_nested_resource)
{
    const char *url = "/maps/nested.bsp";
    lh_u64_t size = k_unset;
    ASSERT_TRUE(resource_open(&source_root, &options, url, strlen(url), &size));
    EXPECT_EQ(size, 6U);
}

class authum_http_denied_path : public authum_http,
                                public ::testing::WithParamInterface<const char *>
{
};

TEST_P(authum_http_denied_path, refuses_unsafe_path_without_changing_size)
{
    lh_u64_t size = k_unset;
    const char *url = GetParam();
    EXPECT_FALSE(resource_open(&source_root, &options, url, strlen(url), &size));
    EXPECT_EQ(size, k_unset);
}

INSTANTIATE_TEST_SUITE_P(unsafe_paths, authum_http_denied_path,
                         ::testing::Values("/link.bsp", "/alias/nested.bsp", "/.hidden.bsp",
                                           "/fifo.bsp", "/maps/../shared.bsp", "/maps//nested.bsp",
                                           "/maps/.nested.bsp", "/maps/nested.txt",
                                           "/maps/nested.bsp/", "/maps/nested.bsp%00",
                                           "/maps%2fnested.bsp", "/maps/%2e%2e/shared.bsp",
                                           "/maps/nested.bsp.", "/maps/nested.bsp%20",
                                           "/maps/NESTED~1.BSP", "/maps/nested.bsp::$DATA",
                                           "/maps/a%5cnested.bsp", "/maps/nested.bsp%",
                                           "/maps/nested%zz.bsp", "/maps/%252e%252e/shared.bsp"));

TEST_F(authum_http, target_is_decoded_once_and_query_ignored)
{
    for (const char *url : {"/shared.bsp?x=1", "/maps/nested.bsp#f", "/maps/%6eested%2Ebsp",
                            "/maps/nested.bsp?..%2f..%2f"})
    {
        lh_u64_t size = k_unset;
        ASSERT_TRUE(resource_open(&source_root, &options, url, strlen(url), &size)) << url;
    }
}

TEST_F(authum_http, policy_lists_compare_without_case)
{
    /* The deny list must hold however the request spells the name: the
     * Windows opener resolves names without case. */
    strcpy(options.deny_prefixes, "maps/nested.bsp");
    lh_u64_t size = k_unset;
    const char *reason = nullptr;
    for (const char *url : {"/MAPS/nested.bsp", "/maps/NESTED.BSP", "/Maps/Nested.Bsp"})
    {
        EXPECT_FALSE(resource_open(&source_root, &options, url, strlen(url), &size, &reason))
            << url;
        EXPECT_STREQ(reason, "denied_prefix") << url;
    }
    /* Allow lists match without case too; the file name itself is left to
     * the filesystem. */
    options.deny_prefixes[0] = 0;
    const char *url = "/MAPS/nested.BSP";
    char path[64];
    EXPECT_NE(authum_http_resource_check(&options, url, strlen(url), path, sizeof path, &reason), 0u);
}

TEST_F(authum_http, refusals_name_their_reason)
{
    const struct
    {
        const char *url;
        const char *reason;
    } cases[] = {{"/maps/missing.bsp", "not_found"},    {"/maps/nested.txt", "ext_not_allowed"},
                 {"/other/x.bsp", "dir_not_allowed"},   {"/maps/.x.bsp", "hidden_or_dot"},
                 {"/maps/a%5cb.bsp", "bad_char"},       {"/maps/x.bsp%", "bad_encoding"},
                 {"/maps%2fx.bsp", "encoded_separator"}, {"/maps//x.bsp", "empty_component"}};
    for (const auto &c : cases)
    {
        lh_u64_t size = k_unset;
        const char *reason = nullptr;
        EXPECT_FALSE(resource_open(&source_root, &options, c.url, strlen(c.url), &size, &reason));
        EXPECT_STREQ(reason, c.reason) << c.url;
    }
}

TEST_F(authum_http, file_size_limit_accepts_exact_boundary)
{
    const char *url = "/maps/nested.bsp";
    lh_u64_t size = k_unset;
    options.max_file_bytes = 5;
    EXPECT_FALSE(resource_open(&source_root, &options, url, strlen(url), &size));
    options.max_file_bytes = 6;
    ASSERT_TRUE(resource_open(&source_root, &options, url, strlen(url), &size));
    EXPECT_EQ(size, 6U);
}

TEST_F(authum_http, deny_prefix_overrides_allowed_directory)
{
    strcpy(options.deny_prefixes, "maps");
    const char *url = "/maps/nested.bsp";
    lh_u64_t size = k_unset;
    EXPECT_FALSE(resource_open(&source_root, &options, url, strlen(url), &size));
}

TEST_F(authum_http, empty_directory_allow_list_refuses_nested_file)
{
    options.allow_dirs[0] = 0;
    const char *url = "/maps/nested.bsp";
    lh_u64_t size = k_unset;
    EXPECT_FALSE(resource_open(&source_root, &options, url, strlen(url), &size));
}

TEST_F(authum_http, root_file_requires_root_extension_permission)
{
    options.root_extensions[0] = 0;
    const char *url = "/shared.bsp";
    lh_u64_t size = k_unset;
    EXPECT_FALSE(resource_open(&source_root, &options, url, strlen(url), &size));
}

} // namespace tests
