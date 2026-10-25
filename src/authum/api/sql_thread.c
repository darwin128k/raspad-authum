#include <authum/sql/thread.h>
#include <lh/sql/mysql/api.h>
#include <lh/os/shared.h>
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
    char *strings[6];
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
    void (*thread_end)(void) = NULL;
    /* mysql_init initializes the client's per-thread state. Release it before
     * returning to the reusable host worker (library stays loaded by client).
     */
    if (job->connection.driver == authum_sql_driver_mysql)
    {
        const lh_sql_mysql_api_t *api = lh_sql_mysql_api_get_default();
        if (api)
        {
            thread_end = (void (*)(void))lh_os_shared_get_fn(&api->image, "mysql_thread_end");
        }
    }
    authum_sql_init(&db);
    job->result.connected = authum_sql_connect(&db, &job->connection);
    if (job->result.connected)
    {
        job->result.success = job->work(&db, job->user);
    }
    if (!job->result.connected || !job->result.success)
    {
        snprintf(job->result.error, sizeof job->result.error, "%s",
                 authum_sql_errmsg(&db)[0] ? authum_sql_errmsg(&db) : "SQL task failed");
    }
    authum_sql_close(&db);
    if (thread_end)
    {
        thread_end();
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
    unsigned i;
    if (job->destroy)
    {
        job->destroy(job->user);
    }
    for (i = 0; i < 6; ++i)
    {
        if (i == 4 && job->strings[i])
        {
            volatile char *p = job->strings[i];
            size_t n = strlen(job->strings[i]);
            while (n--)
            {
                *p++ = 0;
            }
        }
        free(job->strings[i]);
    }
    free(job);
}

authum_bool_t
authum_sql_thread_query(const authum_host_t *host, const authum_sql_connection_t *c,
                        authum_sql_thread_work_fn work, authum_sql_thread_complete_fn complete,
                        authum_task_fn destroy, authum_ptr user)
{
    authum_sql_thread_job_t *job;
    const char *values[6];
    const char **fields[6];
    unsigned i;
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
    job = calloc(1, sizeof *job);
    if (!job)
    {
        return authum_bool_false;
    }
    job->connection = *c;
    values[0] = c->path;
    values[1] = c->host;
    values[2] = c->user;
    values[3] = c->database;
    values[4] = c->password;
    values[5] = NULL;
    fields[0] = &job->connection.path;
    fields[1] = &job->connection.host;
    fields[2] = &job->connection.user;
    fields[3] = &job->connection.database;
    fields[4] = &job->connection.password;
    fields[5] = NULL;
    for (i = 0; i < 5; ++i)
    {
        if (values[i])
        {
            size_t n = strlen(values[i]) + 1;
            job->strings[i] = malloc(n);
            if (!job->strings[i])
            {
                authum_sql_thread_free(job);
                return authum_bool_false;
            }
            memcpy(job->strings[i], values[i], n);
            *fields[i] = job->strings[i];
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
