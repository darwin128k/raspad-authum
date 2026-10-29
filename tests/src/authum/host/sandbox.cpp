/**
 * @file sandbox.cpp
 * @brief Host sandbox (lh_memory_tree) + client sandbox/array API.
 */

#include <gtest/gtest.h>

#include <authum/array.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/host/sandbox.h>
#include <authum/unused.h>

#include <string.h>

int g_destroyed;

void
test_sandbox_count_destroy(authum_ptr ptr)
{
    authum_unused(ptr);
    ++g_destroyed;
}

authum_sandbox_t
test_sandbox_root(void)
{
    authum_sandbox_t sb;

    sb.ops = authum_sandbox_host_ops();
    sb.root = authum_sandbox_host_root_create();
    return sb;
}

TEST(authum_sandbox, sandbox_alloc_and_unload)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_byte_t *a;
    authum_byte_t *b;
    authum_byte_t *child;
    authum_usize_t i;
    authum_bool_t zero = authum_bool_true;

    EXPECT_TRUE(authum_sandbox_is_valid(&sb));
    a = static_cast<authum_byte_t *>(authum_sandbox_alloc(&sb, 64u));
    EXPECT_NE(a, authum_null);
    for (i = 0u; i < 64u; ++i)
    {
        if (a[i] != 0u)
        {
            zero = authum_bool_false;
        }
    }
    EXPECT_TRUE(zero);
    b = static_cast<authum_byte_t *>(authum_sandbox_alloc(&sb, 8u));
    child = static_cast<authum_byte_t *>(authum_sandbox_alloc_under(&sb, b, 8u));
    authum_sandbox_set_destructor(&sb, a, test_sandbox_count_destroy);
    authum_sandbox_set_destructor(&sb, child, test_sandbox_count_destroy);

    g_destroyed = 0;
    authum_sandbox_free(&sb, b);
    EXPECT_EQ(g_destroyed, 1);

    a = static_cast<authum_byte_t *>(authum_sandbox_realloc(&sb, a, 256u));
    EXPECT_NE(a, authum_null);

    authum_sandbox_host_root_destroy(sb.root);
    EXPECT_EQ(g_destroyed, 2);
}

TEST(authum_sandbox, sandbox_nested)
{
    authum_sandbox_t root = test_sandbox_root();
    authum_sandbox_t nested;
    authum_ptr p;

    EXPECT_TRUE(authum_sandbox_open(&nested, &root));
    p = authum_sandbox_alloc(&nested, 16u);
    authum_sandbox_set_destructor(&nested, p, test_sandbox_count_destroy);
    g_destroyed = 0;
    authum_sandbox_close(&nested);
    EXPECT_EQ(g_destroyed, 1);
    EXPECT_FALSE(authum_sandbox_is_valid(&nested));

    EXPECT_TRUE(authum_sandbox_open(&nested, &root));
    p = authum_sandbox_alloc(&nested, 16u);
    authum_sandbox_set_destructor(&nested, p, test_sandbox_count_destroy);
    g_destroyed = 0;
    authum_sandbox_host_root_destroy(root.root);
    EXPECT_EQ(g_destroyed, 1);

    EXPECT_FALSE(authum_sandbox_open(&nested, authum_null));
    EXPECT_EQ(authum_sandbox_alloc(authum_null, 4u), authum_null);
}

/* A block that moves on realloc takes its children along: freeing the tree
 * afterwards must reach them through the new address (GCC 11 + LTO once left
 * them pointing at the freed block). */
TEST(authum_sandbox, sandbox_realloc_with_children)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_ptr parent = authum_sandbox_alloc(&sb, 16u);
    authum_ptr moved;
    int i;

    for (i = 0; i < 3; ++i)
    {
        authum_ptr child = authum_sandbox_alloc_under(&sb, parent, 8u);
        authum_sandbox_set_destructor(&sb, child, test_sandbox_count_destroy);
    }
    moved = authum_sandbox_realloc(&sb, parent, 1u << 16);
    EXPECT_NE(moved, authum_null);

    g_destroyed = 0;
    authum_sandbox_free(&sb, moved);
    EXPECT_EQ(g_destroyed, 3);
    authum_sandbox_host_root_destroy(sb.root);
}

/* A size no heap can give is refused, not fatal. */
TEST(authum_sandbox, sandbox_absurd_size)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_ptr p = authum_sandbox_alloc(&sb, 8u);

    EXPECT_EQ(authum_sandbox_alloc(&sb, (authum_usize_t)-1), authum_null);
    EXPECT_EQ(authum_sandbox_alloc(&sb, (authum_usize_t)-1 / 2u), authum_null);
    EXPECT_EQ(authum_sandbox_realloc(&sb, p, (authum_usize_t)-1 - 8u), authum_null);
    EXPECT_NE(authum_sandbox_alloc(&sb, 8u), authum_null);
    authum_sandbox_host_root_destroy(sb.root);
}

struct test_item
{
    authum_i32_t id;
    authum_char_t name[12];
};

TEST(authum_sandbox, array)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_array_t arr;
    struct test_item item;
    struct test_item *at;
    authum_i32_t i;

    EXPECT_FALSE(authum_array_init(&arr, &sb, 0u));
    EXPECT_TRUE(authum_array_init(&arr, &sb, sizeof(struct test_item)));
    EXPECT_TRUE(authum_array_is_empty(&arr));
    for (i = 0; i < 100; ++i)
    {
        memset(&item, 0, sizeof item);
        item.id = i;
        EXPECT_NE(authum_array_push(&arr, &item), authum_null);
    }
    EXPECT_EQ(authum_array_get_size(&arr), 100u);
    at = static_cast<struct test_item *>(authum_array_get_at(&arr, 42u));
    EXPECT_TRUE(at != authum_null && at->id == 42);
    EXPECT_EQ(authum_array_get_at(&arr, 100u), authum_null);

    EXPECT_TRUE(authum_array_remove_at(&arr, 0u));
    at = static_cast<struct test_item *>(authum_array_get_at(&arr, 0u));
    EXPECT_TRUE(at != authum_null && at->id == 1);
    EXPECT_EQ(authum_array_get_size(&arr), 99u);
    EXPECT_FALSE(authum_array_remove_at(&arr, 99u));

    at = static_cast<struct test_item *>(authum_array_push(&arr, authum_null));
    EXPECT_TRUE(at != authum_null && at->id == 0 && at->name[0] == '\0');

    authum_array_clear(&arr);
    EXPECT_TRUE(authum_array_is_empty(&arr));
    EXPECT_TRUE(authum_array_reserve(&arr, 500u));
    EXPECT_GE(arr.capacity, 500u);

    /* Not deinit'ed on purpose: the sandbox root owns the storage. */
    authum_sandbox_host_root_destroy(sb.root);
}
