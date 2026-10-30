/**
 * @file hooks.cpp
 * @brief The chain of module handlers: order, changes, refusal, removal.
 */

#include <authum/cstr.h>
#include <authum/host/hooks.h>

#include <gtest/gtest.h>

#include <lh/os/thread.h>
#include <lh/util/str/ptr.h>

static char g_trace[64];
static char g_a[] = "a";

static void
trace(char letter)
{
    const authum_usize_t length = lh_str_ptr_len(g_trace);

    if (length + 1u < sizeof g_trace)
    {
        g_trace[length] = letter;
        g_trace[length + 1u] = '\0';
    }
}

static authum_bool_t
on_first(authum_ptr user, authum_hook_call_t *call)
{
    (void)call;
    trace(*(const char *)user);
    return authum_bool_true;
}

static authum_bool_t
on_second(authum_ptr user, authum_hook_call_t *call)
{
    (void)user;
    trace('b');
    /* A handler may change the call: the next one and the game see it. */
    authum_cstr_copy(call->name, sizeof call->name, "changed");
    return authum_bool_true;
}

static authum_bool_t
on_stop(authum_ptr user, authum_hook_call_t *call)
{
    (void)user;
    trace('s');
    call->proceed = authum_bool_false;
    authum_cstr_copy(call->reject, sizeof call->reject, "no entry");
    return authum_bool_false;
}

static authum_bool_t
on_slow(authum_ptr user, authum_hook_call_t *call)
{
    (void)call;
    trace(*(const char *)user);
    lh_os_thread_sleep(20u);
    return authum_bool_true;
}

namespace tests
{

class authum_hooks : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        g_trace[0] = '\0';
        authum_hook_registry_init(authum_hook_registry());
    }

    void
    TearDown() override
    {
        authum_hook_registry_init(authum_hook_registry());
    }
};

TEST_F(authum_hooks, the_last_hooked_is_called_first)
{
    int owner;

    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_first, g_a));
    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_second, authum_null));
    authum_hook_call_t call = {};

    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    EXPECT_STREQ(g_trace, "ba");
    EXPECT_STREQ(call.name, "changed");
}

TEST_F(authum_hooks, a_handler_may_stop_the_call)
{
    int owner;
    authum_hook_call_t call = {};

    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_stop, authum_null));
    call.proceed = authum_bool_true;
    EXPECT_FALSE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                           &call, authum_null));
    EXPECT_STREQ(call.reject, "no entry");
    /* The call names the module that stopped it: the log can say who decided. */
    EXPECT_STREQ(call.decided_by, "GUARD");
}

TEST_F(authum_hooks, a_call_nobody_stopped_has_no_author)
{
    int owner;
    authum_hook_call_t call = {};

    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_first, g_a));
    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    EXPECT_EQ(call.decided_by, authum_null);
}

TEST_F(authum_hooks, a_removed_handler_is_not_called)
{
    int owner;

    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_first, g_a));
    EXPECT_TRUE(authum_hook_registry_remove(authum_hook_registry(), &owner,
                                            authum_hook_point_client_connect, on_first, g_a));
    authum_hook_call_t call = {};

    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    EXPECT_STREQ(g_trace, "");
}

TEST_F(authum_hooks, unloading_a_module_drops_its_handlers)
{
    int owner;
    int other;

    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_first, g_a));
    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &other, "OTHER",
                                         authum_hook_point_client_connect, on_second, authum_null));
    authum_hook_registry_drop_owner(authum_hook_registry(), &owner);
    authum_hook_call_t call = {};

    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    EXPECT_STREQ(g_trace, "b");
}

TEST_F(authum_hooks, a_handler_that_keeps_to_its_budget_stays_in_the_chain)
{
    int owner;
    authum_hook_call_t call = {};

    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_first, g_a));
    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    /* It is still in the chain: the next call reaches it again. */
    g_trace[0] = '\0';
    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    EXPECT_STREQ(g_trace, "a");
}

TEST_F(authum_hooks, a_handler_that_overruns_its_budget_is_dropped)
{
    int owner;
    authum_hook_call_t call = {};

    EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                         authum_hook_point_client_connect, on_slow, g_a));
    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    EXPECT_STREQ(g_trace, "a");
    /* The handler was dropped, so it does not run again. */
    g_trace[0] = '\0';
    call.proceed = authum_bool_true;
    EXPECT_TRUE(authum_hook_registry_call(authum_hook_registry(), authum_hook_point_client_connect,
                                          &call, authum_null));
    EXPECT_STREQ(g_trace, "");
}

TEST_F(authum_hooks, a_point_holds_a_limited_number_of_handlers)
{
    int owner;
    int index;

    for (index = 0; index < authum_hooks_max; index++)
    {
        EXPECT_TRUE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                             authum_hook_point_client_connect, on_first, g_a));
    }
    EXPECT_FALSE(authum_hook_registry_add(authum_hook_registry(), &owner, "GUARD",
                                          authum_hook_point_client_connect, on_first, g_a));
}

} // namespace tests
