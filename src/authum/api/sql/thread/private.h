/**
 * @file private.h
 * @brief Private to `thread.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_SQL_THREAD_PRIVATE_H
#define AUTHUM_API_SQL_THREAD_PRIVATE_H

#include <authum/sql/thread.h>
#include <lh/core/attribute/thread_local.h>
#include <lh/os/sql/keep.h>

typedef struct authum_sql_thread_job
{
    authum_sql_thread_work_fn work;
    authum_sql_thread_complete_fn complete;
    authum_task_fn destroy;
    authum_ptr user;
    /** No end-of-thread call on this host: the job closes its own connection. */
    authum_bool_t own_connection;
    authum_sql_thread_result_t result;
    /** The backend the module asked for, as the handle it gets names it. */
    authum_sql_driver_t driver;
    /** Where to connect, copied from the module's description (owned). */
    lh_sql_address_t address;
} authum_sql_thread_job_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Copy the module's connection description @p c into @p out, an
 *        ::lh_sql_address_init'ed address: the backend and every field, used
 *        or not, with Authum's defaults (MySQL host `127.0.0.1`, an absent
 *        text empty).
 *
 * @return ::authum_bool_false for an unknown backend or an SQLite description
 *         without a path; @p out is left cleared then.
 */
authum_bool_t
authum_sql_thread_try_copy_address(const authum_sql_connection_t *c, lh_sql_address_t *out);

/**
 * @brief The connection this task thread keeps, ::lh_os_sql_keep_init'ed on
 *        first use.
 */
lh_os_sql_keep_t *
authum_sql_thread_get_keep(void);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_SQL_THREAD_PRIVATE_H */
