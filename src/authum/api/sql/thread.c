/**
 * @file thread.c
 * @brief Asynchronous SQL for modules: work on the module's task thread over one
 *        connection kept for that thread.
 *
 * The connection is opened by the first job on a task thread and kept for the
 * next ones; a job for another address closes it and opens its own. When the
 * module unloads, the host runs authum_sql_thread_exit on that thread (registered
 * with authum_tasks_at_exit): the connection is closed and the MySQL client lets
 * go of the thread (mysql_thread_end) on the thread that used it. A host without
 * that service gets the older behaviour: one connection per job.
 */

#include <authum/api/sql/thread/private.h>
#include <authum/memory.h>
#include <authum/crypto/ed25519.h>

#if !LH_LIBRARY_OPTION_THREAD_LOCAL
#    error                                                                                         \
        "authum_sql_thread_query keeps one connection per thread: needs LH_LIBRARY_OPTION_THREAD_LOCAL"
#endif

/** A connection description with its strings copied after it. */

static LH_ATTRIBUTE_THREAD_LOCAL authum_sql_thread_cache_t m_cache;

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

/** Bytes the strings of @p c take with their terminators; 0 on overflow past @p limit. */
authum_usize_t
authum_sql_thread_strings_size(const authum_sql_connection_t *c, authum_usize_t limit)
{
    const authum_char_t *values[] = {c->path, c->host, c->user, c->database, c->password};
    authum_usize_t size = 0;
    for (authum_usize_t i = 0; lh_math_lt(i, lh_math_div(sizeof values, sizeof values[0]));
         i = lh_math_add_one(i))
    {
        if (values[i])
        {
            const authum_u64_t length = lh_str_ptr_len(values[i]);
            if (lh_math_ge(length, lh_math_sub(limit, size)))
            {
                return 0;
            }
            size = lh_math_add(size, lh_math_add(lh_cast_static(authum_usize_t, length), 1u));
        }
    }
    return lh_math_is_zero(size) ? 1u : size;
}

/** Copy @p c into @p dst, its strings into @p strings (sized by authum_sql_thread_strings_size). */
void
authum_sql_thread_copy_init(authum_sql_connection_t *dst, authum_char_t *strings,
                            const authum_sql_connection_t *c)
{
    lh_ptr_deref(dst) = lh_ptr_deref(c);
    authum_char_t *next = strings;
    const authum_char_t *values[] = {c->path, c->host, c->user, c->database, c->password};
    const authum_char_t **fields[] = {lh_addr_of(dst->path), lh_addr_of(dst->host),
                                      lh_addr_of(dst->user), lh_addr_of(dst->database),
                                      lh_addr_of(dst->password)};
    for (authum_usize_t i = 0; lh_math_lt(i, lh_math_div(sizeof values, sizeof values[0]));
         i = lh_math_add_one(i))
    {
        if (values[i])
        {
            const authum_usize_t length = lh_math_add(lh_str_ptr_len(values[i]), 1u);
            lh_memory_std_copy(next, values[i], length);
            lh_ptr_deref(fields[i]) = next;
            next = lh_math_add(next, length);
        }
    }
}

/** Whether two optional strings are equal (both absent counts as equal). */
authum_bool_t
authum_sql_thread_same_text(const authum_char_t *a, const authum_char_t *b)
{
    if (!a || !b)
    {
        return !a && !b;
    }
    return authum_cstr_is_equal(a, b, authum_bool_false);
}

/** Whether @p a and @p b describe the same connection. */
authum_bool_t
authum_sql_thread_same(const authum_sql_connection_t *a, const authum_sql_connection_t *b)
{
    return lh_math_eq(a->driver, b->driver) && lh_math_eq(a->port, b->port) &&
           authum_sql_thread_same_text(a->path, b->path) &&
           authum_sql_thread_same_text(a->host, b->host) &&
           authum_sql_thread_same_text(a->user, b->user) &&
           authum_sql_thread_same_text(a->database, b->database) &&
           authum_sql_thread_same_text(a->password, b->password);
}

/** Close the connection this thread keeps, if any, and forget its description. */
void
authum_sql_thread_drop(authum_sql_thread_cache_t *cache)
{
    if (!cache->key)
    {
        return;
    }
    authum_sql_close(lh_addr_of(cache->db));
    authum_crypto_wipe(cache->key,
                       lh_math_add(sizeof lh_ptr_deref(cache->key), cache->key->strings_size));
    authum_memory_free(cache->key);
    cache->key = authum_null;
}

/** The connection this thread keeps for @p c: kept, reopened, or opened now. */
authum_sql_t *
authum_sql_thread_connection(authum_sql_thread_cache_t *cache, const authum_sql_connection_t *c,
                             authum_usize_t strings_size)
{
    if (cache->key && (!authum_sql_thread_same(lh_addr_of(cache->key->connection), c) ||
                       !authum_sql_is_open(lh_addr_of(cache->db))))
    {
        authum_sql_thread_drop(cache);
    }
    if (cache->key)
    {
        authum_sql_clear_errors(lh_addr_of(cache->db));
        return lh_addr_of(cache->db);
    }
    if (lh_math_eq(c->driver, authum_sql_driver_mysql))
    {
        cache->used_mysql = authum_bool_true;
    }
    authum_sql_init(lh_addr_of(cache->db));
    if (!authum_sql_connect(lh_addr_of(cache->db), c))
    {
        return authum_null;
    }
    const authum_usize_t size = lh_math_add(sizeof lh_ptr_deref(cache->key), strings_size);
    cache->key = authum_memory_alloc_zero(1, size);
    if (cache->key)
    {
        cache->key->strings_size = strings_size;
        authum_sql_thread_copy_init(lh_addr_of(cache->key->connection), cache->key->strings, c);
    }
    /* Without the copy the connection is used once and closed by the caller. */
    return lh_addr_of(cache->db);
}

void
authum_sql_thread_exit(authum_ptr arg)
{
    authum_sql_thread_cache_t *cache = lh_addr_of(m_cache);
    (void)arg;
    authum_sql_thread_drop(cache);
    if (cache->used_mysql)
    {
        lh_os_sql_mysql_api_thread_end(lh_os_sql_mysql_api_get_default());
        cache->used_mysql = authum_bool_false;
    }
}

void
authum_sql_thread_run(authum_ptr arg)
{
    authum_sql_thread_job_t *job = arg;
    authum_sql_thread_cache_t *cache = lh_addr_of(m_cache);
    authum_sql_t *db =
        authum_sql_thread_connection(cache, lh_addr_of(job->connection), job->strings_size);
    job->result.connected = lh_null_ne(db);
    if (job->result.connected)
    {
        job->result.success = job->work(db, job->user);
        if (authum_sql_first_error(db)[0])
        {
            job->result.success = authum_bool_false;
        }
    }
    if (!job->result.connected || !job->result.success)
    {
        const authum_sql_t *source = lh_addr_of(cache->db);
        authum_cstr_copy(job->result.error, sizeof job->result.error,
                         authum_sql_first_error(source)[0] ? authum_sql_first_error(source)
                                                           : "SQL task failed");
    }
    if (!job->result.connected || !cache->key)
    {
        /* Not kept: a failed connect, or no room to remember it. */
        authum_sql_close(lh_addr_of(cache->db));
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
    const authum_tasks_t *tasks = authum_host_get_tasks(host);
    if (!c || !work || !tasks)
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
    const authum_usize_t strings_size = authum_sql_thread_strings_size(
        c, lh_math_sub(LH_U32_T_MAX, sizeof(authum_sql_thread_job_t)));
    if (lh_math_is_zero(strings_size))
    {
        return authum_bool_false;
    }
    authum_sql_thread_job_t *job =
        authum_memory_alloc_zero(1, lh_math_add(sizeof(authum_sql_thread_job_t), strings_size));
    if (!job)
    {
        return authum_bool_false;
    }
    job->strings_size = strings_size;
    authum_sql_thread_copy_init(lh_addr_of(job->connection), job->strings, c);
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
