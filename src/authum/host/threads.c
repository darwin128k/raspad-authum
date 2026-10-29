/**
 * @file threads.c
 * @brief A module's threads: the LH thread group, with Authum's limit and error texts.
 */

#include <authum/host/threads.h>
#include <authum/null.h>
#include <authum/unused.h>
#include <lh/assert/static.h>
#include <lh/os/system.h>
#include <lh/str/format/text.h>
#include <lh/util/return.h>

enum
{
    /** Threads one module may own at a time. */
    authum_threads_max = 64
};

lh_assert_static(AUTHUM_THREAD_BLOCK_SIGPIPE == lh_os_thread_block_sigpipe,
                 "authum and lh sigpipe flags must have the same value");

/** Write @p text as the refusal, when the caller gave room for one. */
static void
authum_threads_refuse(authum_char_t *error, authum_usize_t error_size, const authum_char_t *text)
{
    if (!authum_null_eq(error) && lh_math_ne(error_size, 0u))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "%s", text));
    }
}

/**
 * @brief Validate owner limits and register a worker only after LH startup succeeds.
 */
static authum_thread_t *
start_thread(authum_ptr context, authum_thread_fn run, authum_thread_fn wake, authum_ptr user,
             authum_i32_t cpu, authum_u32_t flags, authum_char_t *error, authum_usize_t error_size)
{
    authum_thread_owner_t *owner = context;
    lh_os_thread_group_member_t *member;

    if (authum_null_eq(owner) || lh_os_thread_group_is_closing(lh_addr_of(owner->group)) ||
        lh_math_ge(lh_os_thread_group_get_count(lh_addr_of(owner->group)), authum_threads_max) ||
        authum_null_eq(run) || authum_null_eq(wake) || lh_math_lt(cpu, -1) ||
        lh_math_bit_and(flags, ~AUTHUM_THREAD_BLOCK_SIGPIPE))
    {
        authum_threads_refuse(error, error_size,
                              "invalid thread options or owner stopping/limit reached");
        return authum_null;
    }
    member = lh_os_thread_group_try_start(lh_addr_of(owner->group), run, wake, user, cpu, flags);
    if (authum_null_eq(member) && !authum_null_eq(error) && lh_math_ne(error_size, 0u))
    {
        authum_unused(lh_str_ptr_format_cstr(
            error, error_size, "cannot create thread (cpu=%d): OS error %u", cpu,
            lh_cast_static(lh_uint_t, lh_os_system_get_last_error_code())));
    }
    /* The module's handle is the group member itself. */
    return lh_ptr_rcast(authum_thread_t, member);
}

/**
 * @brief Stop, join and release a handle belonging to the requesting owner.
 */
static authum_bool_t
stop_thread(authum_ptr context, authum_thread_t *thread)
{
    authum_thread_owner_t *owner = context;

    lh_return_if(authum_null_eq(owner), authum_bool_false);
    return lh_os_thread_group_stop(lh_addr_of(owner->group),
                                   lh_ptr_rcast(lh_os_thread_group_member_t, thread));
}

static const authum_thread_ops_t ops = {.start = start_thread, .stop = stop_thread};

/**
 * @brief Initialize an owner and bind its public thread facade.
 */
void
authum_threads_host_init(authum_thread_owner_t *owner, authum_threads_t *facade)
{
    lh_os_thread_group_init(lh_addr_of(owner->group));
    facade->owner = owner;
    facade->ops = lh_addr_of(ops);
}

/**
 * @brief Wake every owned worker before joining any, preventing unsafe module unload.
 */
void
authum_threads_host_stop_owner(authum_thread_owner_t *owner)
{
    lh_return_if(authum_null_eq(owner));
    lh_os_thread_group_stop_all(lh_addr_of(owner->group));
}

/**
 * @brief Join all owned workers and reclaim their remaining host handles.
 */
void
authum_threads_host_drop_owner(authum_thread_owner_t *owner)
{
    lh_return_if(authum_null_eq(owner));
    lh_os_thread_group_deinit(lh_addr_of(owner->group));
}
