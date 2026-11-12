/**
 * @file region.cpp
 * @brief Host region (lh_memory_tree) + client region/array API.
 */

#include <gtest/gtest.h>

#include <authum/array.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/host/region.h>
#include <authum/unused.h>

#include <string.h>

int g_destroyed;

void
test_region_count_destroy(authum_ptr ptr)
{
    authum_unused(ptr);
    ++g_destroyed;
}

authum_region_t
test_region_root(void)
{
    authum_region_t region;

    region.ops = authum_region_host_ops();
    region.root = authum_region_host_root_create();
    return region;
}

TEST(authum_region, region_alloc_and_unload)
{
    authum_region_t region = test_region_root();
    authum_byte_t *a;
    authum_byte_t *b;
    authum_byte_t *child;
    authum_bool_t zero = authum_bool_true;

    EXPECT_TRUE(authum_region_is_valid(&region));
    a = static_cast<authum_byte_t *>(authum_region_alloc(&region, 64u));
    EXPECT_NE(a, authum_null);
    for (authum_usize_t i = 0u; i < 64u; ++i)
    {
        if (a[i] != 0u)
        {
            zero = authum_bool_false;
        }
    }
    EXPECT_TRUE(zero);
    b = static_cast<authum_byte_t *>(authum_region_alloc(&region, 8u));
    child = static_cast<authum_byte_t *>(authum_region_alloc_under(&region, b, 8u));
    authum_region_set_destructor(&region, a, test_region_count_destroy);
    authum_region_set_destructor(&region, child, test_region_count_destroy);

    g_destroyed = 0;
    authum_region_free(&region, b);
    EXPECT_EQ(g_destroyed, 1);

    a = static_cast<authum_byte_t *>(authum_region_realloc(&region, a, 256u));
    EXPECT_NE(a, authum_null);

    authum_region_host_root_destroy(region.root);
    EXPECT_EQ(g_destroyed, 2);
}

TEST(authum_region, region_nested)
{
    authum_region_t root = test_region_root();
    authum_region_t nested;

    EXPECT_TRUE(authum_region_open(&nested, &root));
    authum_ptr p = authum_region_alloc(&nested, 16u);
    authum_region_set_destructor(&nested, p, test_region_count_destroy);
    g_destroyed = 0;
    authum_region_close(&nested);
    EXPECT_EQ(g_destroyed, 1);
    EXPECT_FALSE(authum_region_is_valid(&nested));

    EXPECT_TRUE(authum_region_open(&nested, &root));
    p = authum_region_alloc(&nested, 16u);
    authum_region_set_destructor(&nested, p, test_region_count_destroy);
    g_destroyed = 0;
    authum_region_host_root_destroy(root.root);
    EXPECT_EQ(g_destroyed, 1);

    EXPECT_FALSE(authum_region_open(&nested, authum_null));
    EXPECT_EQ(authum_region_alloc(authum_null, 4u), authum_null);
}

/* A block that moves on realloc takes its children along: freeing the tree
 * afterwards must reach them through the new address (GCC 11 + LTO once left
 * them pointing at the freed block). */
TEST(authum_region, region_realloc_with_children)
{
    authum_region_t region = test_region_root();
    authum_ptr parent = authum_region_alloc(&region, 16u);

    for (int i = 0; i < 3; ++i)
    {
        authum_ptr child = authum_region_alloc_under(&region, parent, 8u);
        authum_region_set_destructor(&region, child, test_region_count_destroy);
    }
    authum_ptr moved = authum_region_realloc(&region, parent, 1u << 16);
    EXPECT_NE(moved, authum_null);

    g_destroyed = 0;
    authum_region_free(&region, moved);
    EXPECT_EQ(g_destroyed, 3);
    authum_region_host_root_destroy(region.root);
}

/* A size no heap can give is refused, not fatal. */
TEST(authum_region, region_absurd_size)
{
    authum_region_t region = test_region_root();
    authum_ptr p = authum_region_alloc(&region, 8u);

    EXPECT_EQ(authum_region_alloc(&region, (authum_usize_t)-1), authum_null);
    EXPECT_EQ(authum_region_alloc(&region, (authum_usize_t)-1 / 2u), authum_null);
    EXPECT_EQ(authum_region_realloc(&region, p, (authum_usize_t)-1 - 8u), authum_null);
    EXPECT_NE(authum_region_alloc(&region, 8u), authum_null);
    authum_region_host_root_destroy(region.root);
}

struct test_item
{
    authum_i32_t id;
    authum_char_t name[12];
};

TEST(authum_region, array)
{
    authum_region_t region = test_region_root();
    struct test_item *at;

    EXPECT_EQ(authum_array_create(&region, 0u), authum_null);
    EXPECT_EQ(authum_array_create(authum_null, sizeof(struct test_item)), authum_null);
    authum_array_t *arr = authum_array_create(&region, sizeof(struct test_item));
    ASSERT_NE(arr, authum_null);
    EXPECT_TRUE(authum_array_is_empty(arr));
    EXPECT_EQ(authum_array_get_capacity(arr), 0u);
    struct test_item item;
    for (authum_i32_t i = 0; i < 100; ++i)
    {
        memset(&item, 0, sizeof item);
        item.id = i;
        EXPECT_NE(authum_array_push(arr, &item), authum_null);
    }
    EXPECT_EQ(authum_array_get_size(arr), 100u);
    at = static_cast<struct test_item *>(authum_array_get_at(arr, 42u));
    EXPECT_TRUE(at != authum_null && at->id == 42);
    EXPECT_EQ(authum_array_get_at(arr, 100u), authum_null);

    EXPECT_TRUE(authum_array_remove_at(arr, 0u));
    at = static_cast<struct test_item *>(authum_array_get_at(arr, 0u));
    EXPECT_TRUE(at != authum_null && at->id == 1);
    EXPECT_EQ(authum_array_get_size(arr), 99u);
    EXPECT_FALSE(authum_array_remove_at(arr, 99u));

    at = static_cast<struct test_item *>(authum_array_push(arr, authum_null));
    EXPECT_TRUE(at != authum_null && at->id == 0 && at->name[0] == '\0');

    authum_array_clear(arr);
    EXPECT_TRUE(authum_array_is_empty(arr));
    EXPECT_TRUE(authum_array_reserve(arr, 500u));
    EXPECT_GE(authum_array_get_capacity(arr), 500u);

    /* Not destroyed on purpose: the region root owns the record and its storage. */
    authum_region_host_root_destroy(region.root);
}

TEST(authum_region, array_destroy_frees_record_and_storage)
{
    authum_region_t region = test_region_root();
    authum_array_t *arr = authum_array_create(&region, sizeof(authum_i32_t));
    ASSERT_NE(arr, authum_null);
    for (authum_i32_t i = 0; i < 64; ++i)
    {
        EXPECT_NE(authum_array_push(arr, &i), authum_null);
    }
    EXPECT_EQ(*static_cast<authum_i32_t *>(authum_array_get_at(arr, 63u)), 63);
    authum_array_destroy(arr);
    authum_array_destroy(authum_null);
    EXPECT_EQ(authum_array_get_size(authum_null), 0u);
    EXPECT_TRUE(authum_array_is_empty(authum_null));
    authum_region_host_root_destroy(region.root);
}
