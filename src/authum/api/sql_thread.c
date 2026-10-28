#include <authum/sql/thread.h>
#include <lh/sql/mysql/api.h>
#include <authum/crypto/ed25519.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct authum_sql_thread_job
{
    authum_sql_connection_t connection;
    authum_sql_thread_work_fn work;
    authum_sql_thread_complete_fn complete;
    authum_task_fn destroy;
    authum_ptr user;
    authum_sql_thread_result_t result;
    authum_usize_t strings_size;
    authum_char_t strings[];
} authum_sql_thread_job_t;

authum_bool_t
authum_sql_connect(authum_sql_t *db, const authum_sql_connection_t *c)
{
    if (!db || !c)
    {
        return authum_bool_false;
    }
    if (c->driver == authum_sql_driver_sqlite)
    {
        return c->path && authum_sql_open_sqlite(db, c->path);
    }
    if (c->driver == authum_sql_driver_mysql)
    {
        return authum_sql_open_mysql(db, c->host ? c->host : "127.0.0.1", c->user ? c->user : "",
                                     c->password ? c->password : "", c->database ? c->database : "",
                                     c->port);
    }
    return authum_bool_false;
}

void
authum_sql_thread_run(authum_ptr arg)
{
    authum_sql_thread_job_t *job = arg;
    authum_sql_t db;
    const lh_sql_mysql_api_t *api = job->connection.driver == authum_sql_driver_mysql
                                        ? lh_sql_mysql_api_get_default()
                                        : authum_null;
    authum_sql_init(&db);
    job->result.connected = authum_sql_connect(&db, &job->connection);
    if (job->result.connected)
    {
        job->result.success = job->work(&db, job->user);
        if (authum_sql_first_error(&db)[0])
        {
            job->result.success = authum_bool_false;
        }
    }
    if (!job->result.connected || !job->result.success)
    {
        snprintf(job->result.error, sizeof job->result.error, "%s",
                 authum_sql_first_error(&db)[0] ? authum_sql_first_error(&db) : "SQL task failed");
    }
    authum_sql_close(&db);
    if (api)
    {
        lh_sql_mysql_api_thread_end(api);
    }
}

void
authum_sql_thread_finish(authum_ptr arg)
{
    authum_sql_thread_job_t *job = arg;
    if (job->complete)
    {
        job->complete(&job->result, job->user);
    }
}

void
authum_sql_thread_free(authum_ptr arg)
{
    authum_sql_thread_job_t *job = arg;
    if (job->destroy)
    {
        job->destroy(job->user);
    }
    authum_crypto_wipe(job->strings, job->strings_size);
    free(job);
}

authum_bool_t
authum_sql_thread_query(const authum_host_t *host, const authum_sql_connection_t *c,
                        authum_sql_thread_work_fn work, authum_sql_thread_complete_fn complete,
                        authum_task_fn destroy, authum_ptr user)
{
    authum_sql_thread_job_t *job;
    authum_usize_t strings_size = 0;
    authum_char_t *next;
    authum_usize_t i;
    if (!c || !work || !authum_host_tasks(host))
    {
        return authum_bool_false;
    }
    /* Load the shared client on the game thread, before workers may use it.
     * dlopen does not connect to the database.
     */
    if (c->driver == authum_sql_driver_mysql && !authum_sql_mysql_available())
    {
        return authum_bool_false;
    }
    const authum_char_t *values[] = {c->path, c->host, c->user, c->database, c->password};
    authum_usize_t lengths[sizeof values / sizeof values[0]] = {0};
    for (i = 0; i < sizeof values / sizeof values[0]; ++i)
    {
        if (values[i])
        {
            authum_u64_t length = strlen(values[i]);
            if (length >= UINT32_MAX - sizeof *job - strings_size)
            {
                return authum_bool_false;
            }
            lengths[i] = (authum_usize_t)length + 1u;
            strings_size += lengths[i];
        }
    }
    job = calloc(1, sizeof *job + strings_size);
    if (!job)
    {
        return authum_bool_false;
    }
    job->connection = *c;
    job->strings_size = strings_size;
    next = job->strings;
    const authum_char_t **fields[] = {&job->connection.path, &job->connection.host,
                                      &job->connection.user, &job->connection.database,
                                      &job->connection.password};
    for (i = 0; i < sizeof values / sizeof values[0]; ++i)
    {
        if (values[i])
        {
            memcpy(next, values[i], lengths[i]);
            *fields[i] = next;
            next += lengths[i];
        }
    }
    job->work = work;
    job->complete = complete;
    job->user = user;
    if (!authum_tasks_submit(authum_host_tasks(host), authum_sql_thread_run,
                             authum_sql_thread_finish, authum_sql_thread_free, job))
    {
        authum_sql_thread_free(job);
        return authum_bool_false;
    }
    job->destroy = destroy;
    return authum_bool_true;
}
