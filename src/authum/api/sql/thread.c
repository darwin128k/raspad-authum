/**
 * @file thread.c
 * @brief Asynchronous SQL for modules: work on the module's task thread over one
 *        connection kept for that thread.
 *
 * The connection is kept by lh (::lh_os_sql_keep_t, one per task thread): the
 * first job opens it, the next ones use it again, a job for another address
 * closes it and opens its own. When the module unloads, the host runs
 * authum_sql_thread_exit on that thread (registered with authum_tasks_at_exit):
 * the connection is closed and the MySQL client lets go of the thread
 * (mysql_thread_end) on the thread that used it. A host without that service
 * gets the older behaviour: one connection per job.
 */

#include <authum/api/sql/thread/private.h>
#include <authum/api/sql/private.h>
#include <authum/memory.h>
#include <authum/unused.h>

#if !LH_LIBRARY_OPTION_THREAD_LOCAL
#    error                                                                                         \
        "authum_sql_thread_query keeps one connection per thread: needs LH_LIBRARY_OPTION_THREAD_LOCAL"
#endif

/* The connection this task thread keeps, and whether it was set up yet. */
static LH_ATTRIBUTE_THREAD_LOCAL lh_os_sql_keep_t m_keep;
static LH_ATTRIBUTE_THREAD_LOCAL lh_bool_t m_keep_ready;

authum_bool_t
authum_sql_thread_try_copy_address(const authum_sql_connection_t *c, lh_sql_address_t *out)
{
    lh_sql_address_clear(out);

    /* Every field is copied, the unused ones too: two descriptions that differ
     * anywhere are two connections. */
    lh_str_assign_view(lh_sql_address_get_path_mut(out), authum_sql_view(c->path));
    lh_str_assign_view(lh_sql_address_get_user_mut(out), authum_sql_view(c->user));
    lh_str_assign_view(lh_sql_address_get_password_mut(out), authum_sql_view(c->password));
    lh_str_assign_view(lh_sql_address_get_database_mut(out), authum_sql_view(c->database));
    lh_sql_address_set_port(out, lh_cast_static(lh_uint_t, c->port));
    if (lh_math_eq(c->driver, authum_sql_driver_sqlite) && lh_null_ne(c->path))
    {
        lh_str_assign_view(lh_sql_address_get_host_mut(out), authum_sql_view(c->host));
        lh_sql_address_set_driver(out, lh_os_sql_sqlite_get_driver());
        return authum_bool_true;
    }
    if (lh_math_eq(c->driver, authum_sql_driver_mysql))
    {
        lh_str_assign_view(lh_sql_address_get_host_mut(out),
                           authum_sql_view(lh_null_ne(c->host) ? c->host : "127.0.0.1"));
        lh_sql_address_set_driver(out, lh_os_sql_mysql_get_driver());
        return authum_bool_true;
    }
    lh_sql_address_clear(out);
    return authum_bool_false;
}

authum_bool_t
authum_sql_connect(authum_sql_t *db, const authum_sql_connection_t *c)
{
    if (authum_null_eq(db) || authum_null_eq(c))
    {
        return authum_bool_false;
    }

    lh_sql_address_t address;

    lh_sql_address_init(lh_addr_of(address));

    authum_bool_t ok = authum_bool_false;

    if (authum_sql_thread_try_copy_address(c, lh_addr_of(address)))
    {
        lh_sql_db_t *opened = authum_sql_begin_open(db, c->driver);

        ok = lh_null_ne(opened) &&
             authum_sql_end_open(db, lh_os_sql_address_try_open(lh_addr_of(address), opened));
    }
    lh_sql_address_deinit(lh_addr_of(address));
    return ok;
}

lh_os_sql_keep_t *
authum_sql_thread_get_keep(void)
{
    if (!m_keep_ready)
    {
        lh_os_sql_keep_init(lh_addr_of(m_keep));
        m_keep_ready = lh_bool_true;
    }
    return lh_addr_of(m_keep);
}

void
authum_sql_thread_exit(authum_ptr arg)
{
    authum_unused(arg);
    if (m_keep_ready)
    {
        lh_os_sql_keep_deinit(lh_addr_of(m_keep));
        m_keep_ready = lh_bool_false;
    }
}

void
authum_sql_thread_run(authum_ptr arg)
{
    authum_sql_thread_job_t *job = arg;
    lh_os_sql_keep_t *keep = authum_sql_thread_get_keep();
    const lh_bool_t opened = lh_null_ne(lh_os_sql_keep_try_get_db(keep, lh_addr_of(job->address)));

    /* The module works on a handle over the kept connection; on a failed open
     * the handle still carries the reason. */
    authum_sql_t handle;

    authum_sql_init(lh_addr_of(handle));
    const authum_bool_t borrowed =
        authum_sql_borrow(lh_addr_of(handle), job->driver, lh_os_sql_keep_get_db_mut(keep));

    job->result.connected = opened && borrowed;
    if (job->result.connected)
    {
        job->result.success = job->work(lh_addr_of(handle), job->user);
        if (authum_sql_first_error(lh_addr_of(handle))[0])
        {
            job->result.success = authum_bool_false;
        }
    }
    if (!job->result.connected || !job->result.success)
    {
        const authum_char_t *error = authum_sql_first_error(lh_addr_of(handle));

        authum_cstr_copy(job->result.error, sizeof job->result.error,
                         error[0] ? error : "SQL task failed");
    }
    authum_sql_close(lh_addr_of(handle));
    if (!job->result.connected)
    {
        lh_os_sql_keep_close(keep);
    }
    if (job->own_connection)
    {
        authum_sql_thread_exit(authum_null);
    }
}

void
authum_sql_thread_finish(authum_ptr arg)
{
    authum_sql_thread_job_t *job = arg;
    if (!authum_null_eq(job->complete))
    {
        job->complete(lh_addr_of(job->result), job->user);
    }
}

void
authum_sql_thread_free(authum_ptr arg)
{
    authum_sql_thread_job_t *job = arg;
    if (!authum_null_eq(job->destroy))
    {
        job->destroy(job->user);
    }
    lh_sql_address_deinit(lh_addr_of(job->address));
    authum_memory_free(job);
}

authum_bool_t
authum_sql_thread_query(const authum_host_t *host, const authum_sql_connection_t *c,
                        authum_sql_thread_work_fn work, authum_sql_thread_complete_fn complete,
                        authum_task_fn destroy, authum_ptr user)
{
    const authum_tasks_t *tasks = authum_host_get_tasks(host);
    if (authum_null_eq(c) || authum_null_eq(work) || authum_null_eq(tasks))
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
    authum_sql_thread_job_t *job = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(job));
    if (authum_null_eq(job))
    {
        return authum_bool_false;
    }
    lh_sql_address_init(lh_addr_of(job->address));
    /* A description that names nothing is the job's to fail: it reports that it
     * could not connect, and the payload is disposed of as always. */
    authum_unused(authum_sql_thread_try_copy_address(c, lh_addr_of(job->address)));
    job->driver = c->driver;
    job->work = work;
    job->complete = complete;
    job->user = user;
    /* Close the connection the module's task thread keeps when that thread ends. */
    job->own_connection = !authum_tasks_at_exit(tasks, authum_sql_thread_exit, authum_null);
    if (!authum_tasks_submit(tasks, authum_sql_thread_run, authum_sql_thread_finish,
                             authum_sql_thread_free, job))
    {
        authum_sql_thread_free(job);
        return authum_bool_false;
    }
    job->destroy = destroy;
    return authum_bool_true;
}
