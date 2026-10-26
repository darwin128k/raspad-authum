#include <authum/client.h>
#include <authum/host/events.h>
#include <authum/host/tasks.h>
#include <check.h>
#include <stdatomic.h>
#include <string.h>
#if defined(_WIN32)
#    include <windows.h>
typedef DWORD thread_id_t;

thread_id_t
thread_id(void)
{
    return GetCurrentThreadId();
}

void
pause_ms(void)
{
    Sleep(1);
}
#else
#    include <pthread.h>
#    include <time.h>
typedef pthread_t thread_id_t;

thread_id_t
thread_id(void)
{
    return pthread_self();
}

void
pause_ms(void)
{
    struct timespec t = {0, 1000000};
    nanosleep(&t, NULL);
}
#endif
static thread_id_t main_thread;
static atomic_int written, enabled, wrong_thread, work_finished;
static int delivered;
static char last_line[authum_logger_line_max];
static int saw_auto_message;
static authum_i32_t current_user = 100;

void
write_log(authum_ptr owner, authum_logger_level_t level, const char *text)
{
    authum_unused(owner);
    authum_unused(level);
    if (thread_id() != main_thread)
    {
        atomic_store(&wrong_thread, 1);
    }
    atomic_fetch_add(&written, 1);
    authum_cstr_copy(last_line, sizeof last_line, text);
    if (!strcmp(text, "worker 42"))
    {
        saw_auto_message = 1;
    }
}

authum_bool_t
log_enabled(authum_ptr owner, authum_logger_level_t level)
{
    authum_unused(owner);
    authum_unused(level);
    if (thread_id() != main_thread)
    {
        atomic_store(&wrong_thread, 1);
    }
    atomic_fetch_add(&enabled, 1);
    return authum_bool_true;
}

typedef struct log_job
{
    authum_logger_buffer_t buffer;
    authum_logger_t logger;
    authum_host_t host;
} log_job_t;

void
log_work(void *user)
{
    log_job_t *job = user;
    authum_logger_buffer_t *previous = authum_logger_buffer_begin(&job->buffer);
    authum_host_log_info(&job->host, "worker %d", 42);
    authum_logger_error(&job->logger, "worker error");
    authum_logger_warning(&job->logger, "overflow");
    authum_logger_buffer_end(previous);
    atomic_store(&work_finished, 1);
}

void
log_complete(void *user)
{
    log_job_t *job = user;
    CHECK(thread_id() == main_thread);
    CHECK(authum_logger_buffer_dropped(&job->buffer) == 1);
    authum_logger_buffer_flush(&job->buffer);
    CHECK(saw_auto_message);
    ++delivered;
}

void
test_buffer(void)
{
    int owner;
    unsigned i;
    log_job_t job = {0};
    authum_logger_ops_t ops = {sizeof ops, write_log, log_enabled, 0, 0};
    authum_logger_channel_t channel = {&ops, NULL};
    authum_tasks_t tasks = {authum_tasks_host_ops(), &owner};
    authum_logger_buffer_t nested = {0}, *previous;
    job.logger.channel = &channel;
    job.host.size = sizeof job.host;
    job.host.logger = &channel;
    CHECK(!authum_logger_buffer_init(&job.buffer, &job.logger, 0));
    CHECK(authum_logger_buffer_init(&job.buffer, &job.logger, 2));
    CHECK(authum_tasks_submit(&tasks, log_work, log_complete, NULL, &job));
    for (i = 0; i < 5000 && !atomic_load(&work_finished); ++i)
    {
        pause_ms();
    }
    CHECK(atomic_load(&work_finished));
    CHECK(atomic_load(&written) == 0);
    CHECK(atomic_load(&enabled) == 0);
    for (i = 0; i < 5000 && !delivered; ++i)
    {
        authum_tasks_host_pump();
        pause_ms();
    }
    CHECK(delivered == 1);
    CHECK(atomic_load(&written) == 3);
    CHECK(!atomic_load(&wrong_thread));
    CHECK(strstr(last_line, "1 background log messages dropped"));
    CHECK(authum_logger_buffer_dropped(&job.buffer) == 0);
    CHECK(authum_logger_buffer_init(&nested, &job.logger, 1));
    previous = authum_logger_buffer_begin(&job.buffer);
    authum_logger_info(&job.logger, "outer");
    {
        authum_logger_buffer_t *outer = authum_logger_buffer_begin(&nested);
        authum_logger_info(&job.logger, "inner");
        authum_logger_buffer_end(outer);
    }
    authum_logger_buffer_end(previous);
    authum_logger_buffer_flush(&nested);
    CHECK_STR(last_line, "inner");
    authum_logger_buffer_flush(&job.buffer);
    CHECK_STR(last_line, "outer");
    authum_logger_buffer_destroy(&nested);
    authum_logger_buffer_destroy(&job.buffer);
    authum_logger_buffer_destroy(&job.buffer);
    authum_tasks_host_drop_owner(&owner);
}

authum_i32_t
userid(authum_i32_t index)
{
    return index == 1 ? current_user : 0;
}

void
publish_client(const authum_host_t *host, const char *event)
{
    authum_event_client_t ev = {0};
    ev.size = sizeof ev;
    ev.index = 1;
    ev.userid = current_user;
    authum_host_publish(host, event, &ev);
}

void
test_refs(void)
{
    authum_clients_t clients = {0}, other = {0};
    authum_client_ref_t ref;
    authum_host_t host = {0};
    authum_engine_t engine = {0};
    authum_events_t events = {authum_events_host_ops(), &host};
    host.size = sizeof host;
    host.events = &events;
    host.engine = &engine;
    engine.size = sizeof engine;
    engine.client_userid = userid;
    CHECK(authum_clients_init(&clients, &host, 32));
    CHECK(authum_clients_capture(&clients, 1, &ref)); /* already connected at module load */
    CHECK(authum_client_ref_is_current(&clients, &ref));
    CHECK(authum_clients_init(&other, &host, 32));
    CHECK(!authum_client_ref_is_current(&other, &ref));
    authum_clients_destroy(&other);
    publish_client(&host, AUTHUM_EVENT_CLIENT_DISCONNECT);
    CHECK(!authum_client_ref_is_current(&clients, &ref));
    publish_client(&host, AUTHUM_EVENT_CLIENT_CONNECT);
    publish_client(&host, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER);
    CHECK(!authum_client_ref_is_current(&clients, &ref)); /* same userid, new generation */
    CHECK(authum_clients_capture(&clients, 1, &ref));
    current_user = 101;
    CHECK(!authum_client_ref_is_current(&clients, &ref));
    current_user = 100;
    authum_host_publish(&host, AUTHUM_EVENT_SERVER_DEACTIVATE, NULL);
    CHECK(!authum_client_ref_is_current(&clients, &ref));
    CHECK(!authum_clients_capture(&clients, 0, &ref));
    CHECK(!authum_clients_capture(&clients, 33, &ref));
    publish_client(&host, AUTHUM_EVENT_CLIENT_CONNECT);
    publish_client(&host, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER);
    CHECK(authum_clients_capture(&clients, 1, &ref));
    authum_clients_destroy(&clients);
    CHECK(authum_clients_init(&clients, &host, 32));
    CHECK(!authum_client_ref_is_current(&clients, &ref));
    authum_clients_destroy(&clients);
    authum_clients_destroy(&clients);
    authum_events_host_drop_owner(&host);
}

void
test_error(void)
{
    authum_sql_t db;
    authum_sql_stmt_t stmt;
    char first[authum_sql_errmsg_max];
    authum_sql_init(&db);
    CHECK(authum_sql_open_sqlite(&db, ":memory:"));
    CHECK(!authum_sql_exec(&db, "SELECT * FROM missing_first"));
    authum_cstr_copy(first, sizeof first, authum_sql_first_error(&db));
    CHECK(first[0]);
    CHECK(authum_sql_exec(&db, "SELECT 1"));
    CHECK_STR(authum_sql_errmsg(&db), "");
    CHECK_STR(authum_sql_first_error(&db), first);
    CHECK(!authum_sql_exec(&db, "SELECT * FROM missing_second"));
    CHECK_STR(authum_sql_first_error(&db), first);
    authum_sql_clear_errors(&db);
    CHECK_STR(authum_sql_first_error(&db), "");
    CHECK(authum_sql_prepare(&db, &stmt, "SELECT ?"));
    CHECK(!authum_sql_stmt_bind_i32(&stmt, 0, 1));
    CHECK(authum_sql_first_error(&db)[0]);
    authum_sql_stmt_finalize(&stmt);
    authum_sql_clear_errors(&db);
    CHECK(authum_sql_ping(&db));
    CHECK_STR(authum_sql_first_error(&db), "");
    authum_sql_close(&db);
}

void
test_address(void)
{
    char out[64], inplace[64] = "[2001:db8::1]:27015";
    CHECK(authum_net_address_host("192.0.2.10:27015", out, sizeof out));
    CHECK_STR(out, "192.0.2.10");
    CHECK(authum_net_address_host("example.org", out, sizeof out));
    CHECK_STR(out, "example.org");
    CHECK(authum_net_address_host("2001:db8::1", out, sizeof out));
    CHECK_STR(out, "2001:db8::1");
    CHECK(authum_net_address_host(inplace, inplace, sizeof inplace));
    CHECK_STR(inplace, "2001:db8::1");
    CHECK(!authum_net_address_host("[::1", out, sizeof out));
    CHECK_STR(out, "");
    CHECK(!authum_net_address_host("a:65536", out, sizeof out));
    CHECK_STR(out, "");
    CHECK(!authum_net_address_host("a:abc", out, sizeof out));
    CHECK(!authum_net_address_host("host:1", out, 2));
    CHECK_STR(out, "");
    CHECK(!authum_net_address_host("a\r\nb", out, sizeof out));
    CHECK(!authum_net_address_host(NULL, out, sizeof out));
}

int
main(void)
{
    main_thread = thread_id();
    test_buffer();
    test_refs();
    test_error();
    test_address();
    return authum_check_report("shared module helpers");
}
