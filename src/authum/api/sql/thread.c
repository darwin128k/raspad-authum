#include <authum/sql/thread.h>
#include <authum/memory.h>
#include <lh/memory/std.h>
#include <lh/sql/mysql/api.h>
#include <authum/crypto/ed25519.h>
#include <stdlib.h>

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
    if (lh_math_eq(c->driver, authum_sql_driver_sqlite))
    {
        return c->path && authum_sql_open_sqlite(db, c->path);
    }
    if (lh_math_eq(c->driver, authum_sql_driver_mysql))
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
    const lh_sql_mysql_api_t *api = lh_math_eq(job->connection.driver, authum_sql_driver_mysql)
                                        ? lh_sql_mysql_api_get_default()
                                        : authum_null;
    authum_sql_init(lh_addr_of(db));
    job->result.connected = authum_sql_connect(lh_addr_of(db), lh_addr_of(job->connection));
    if (job->result.connected)
    {
        job->result.success = job->work(lh_addr_of(db), job->user);
        if (authum_sql_first_error(lh_addr_of(db))[0])
        {
            job->result.success = authum_bool_false;
        }
    }
    if (!job->result.connected || !job->result.success)
    {
        authum_cstr_copy(job->result.error, sizeof job->result.error,
                         authum_sql_first_error(lh_addr_of(db))[0]
                             ? authum_sql_first_error(lh_addr_of(db))
                             : "SQL task failed");
    }
    authum_sql_close(lh_addr_of(db));
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
        job->complete(lh_addr_of(job->result), job->user);
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
    authum_memory_free(job);
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
    if (!c || !work || !authum_host_get_tasks(host))
    {
        return authum_bool_false;
    }
    /* Load the shared client on the game thread, before workers may use it.
     * dlopen does not connect to the database.
     */
    if (lh_math_eq(c->driver, authum_sql_driver_mysql) && !authum_sql_mysql_available())
    {
        return authum_bool_false;
    }
    const authum_char_t *values[] = {c->path, c->host, c->user, c->database, c->password};
    authum_usize_t lengths[lh_math_div(sizeof values, sizeof values[0])] = {0};
    for (i = 0; lh_math_lt(i, lh_math_div(sizeof values, sizeof values[0])); i = lh_math_add_one(i))
    {
        if (values[i])
        {
            authum_u64_t length = lh_str_ptr_len(values[i]);
            if (lh_math_ge(length, lh_math_sub(lh_math_sub(LH_U32_T_MAX, sizeof lh_ptr_deref(job)),
                                               strings_size)))
            {
                return authum_bool_false;
            }
            lengths[i] = lh_math_add(lh_cast_static(authum_usize_t, length), 1u);
            strings_size = lh_math_add(strings_size, lengths[i]);
        }
    }
    job = authum_memory_alloc_zero(1, lh_math_add(sizeof lh_ptr_deref(job), strings_size));
    if (!job)
    {
        return authum_bool_false;
    }
    job->connection = lh_ptr_deref(c);
    job->strings_size = strings_size;
    next = job->strings;
    const authum_char_t **fields[] = {
        lh_addr_of(job->connection.path), lh_addr_of(job->connection.host),
        lh_addr_of(job->connection.user), lh_addr_of(job->connection.database),
        lh_addr_of(job->connection.password)};
    for (i = 0; lh_math_lt(i, lh_math_div(sizeof values, sizeof values[0])); i = lh_math_add_one(i))
    {
        if (values[i])
        {
            lh_memory_std_copy(next, values[i], lengths[i]);
            lh_ptr_deref(fields[i]) = next;
            next = lh_math_add(next, lengths[i]);
        }
    }
    job->work = work;
    job->complete = complete;
    job->user = user;
    if (!authum_tasks_submit(authum_host_get_tasks(host), authum_sql_thread_run,
                             authum_sql_thread_finish, authum_sql_thread_free, job))
    {
        authum_sql_thread_free(job);
        return authum_bool_false;
    }
    job->destroy = destroy;
    return authum_bool_true;
}
