#include <authum/memory.h>
#include <authum/bool.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <lh/runtime/allocator.h>
#include <lh/memory/std.h>
#include <stdlib.h>
#include <string.h>

static authum_usize_t allocations, releases, resizes;
static authum_bool_t fail;

lh_ptr
allocate(lh_self_ptr context, lh_usize_t size)
{
    authum_unused(context);
    ++allocations;
    lh_ptr block = fail ? authum_null : malloc(size);
    if (!authum_null_eq(block))
    {
        lh_memory_std_set(block, 0xA5, size);
    }
    return block;
}

lh_void
release(lh_self_ptr context, lh_ptr block)
{
    authum_unused(context);
    ++releases;
    free(block);
}

lh_ptr
resize(lh_self_ptr context, lh_ptr block, lh_usize_t old_size, lh_usize_t new_size)
{
    authum_unused(context);
    authum_unused(old_size);
    ++resizes;
    return fail ? authum_null : realloc(block, new_size);
}

static lh_memory_allocator_t saved_allocator;
static authum_byte_t *block;

namespace tests
{

class authum_memory : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        allocations = releases = resizes = 0;
        fail = authum_bool_false;
        block = authum_null;
        lh_memory_allocator_assign(&saved_allocator, lh_runtime_allocator());
        lh_memory_allocator_init(lh_runtime_allocator(), allocate, release);
        lh_memory_allocator_set_realloc_cb(lh_runtime_allocator(), resize);
    }

    void
    TearDown() override
    {
        authum_memory_free(block);
        lh_memory_allocator_assign(lh_runtime_allocator(), &saved_allocator);
    }
};

TEST_F(authum_memory, rejects_zero_size)
{
    EXPECT_EQ(authum_memory_alloc(0), authum_null);
    EXPECT_EQ(allocations, 0u);
}

TEST_F(authum_memory, rejects_multiplication_overflow)
{
    EXPECT_EQ(authum_memory_alloc_zero(LH_U32_T_MAX, 2), authum_null);
    EXPECT_EQ(allocations, 0u);
}

TEST_F(authum_memory, zero_initializes_every_byte)
{
    block = static_cast<authum_byte_t *>(authum_memory_alloc_zero(4, 8));
    ASSERT_NE(block, authum_null);
    for (authum_usize_t index = 0; index < 32; ++index)
    {
        EXPECT_EQ(block[index], 0) << "byte " << index;
    }
}

TEST_F(authum_memory, reports_allocator_failure)
{
    fail = authum_bool_true;
    EXPECT_EQ(authum_memory_alloc(8), authum_null);
    EXPECT_EQ(allocations, 1u);
}

TEST_F(authum_memory, failed_resize_preserves_original_block)
{
    block = static_cast<authum_byte_t *>(authum_memory_alloc(32));
    ASSERT_NE(block, authum_null);
    lh_memory_std_copy(block, "keep", 5);
    fail = authum_bool_true;
    EXPECT_EQ(authum_memory_realloc(block, 32, 64), authum_null);
    EXPECT_STREQ(reinterpret_cast<const authum_char_t *>(block), "keep");
    EXPECT_EQ(resizes, 1u);
    EXPECT_EQ(releases, 0u);
}

TEST_F(authum_memory, resize_callback_preserves_bytes)
{
    block = static_cast<authum_byte_t *>(authum_memory_alloc(32));
    ASSERT_NE(block, authum_null);
    lh_memory_std_copy(block, "keep", 5);
    auto *grown = static_cast<authum_byte_t *>(authum_memory_realloc(block, 32, 64));
    ASSERT_NE(grown, authum_null);
    block = grown;
    EXPECT_STREQ(reinterpret_cast<const authum_char_t *>(block), "keep");
    EXPECT_EQ(resizes, 1u);
}

TEST_F(authum_memory, resize_without_callback_copies_and_releases)
{
    block = static_cast<authum_byte_t *>(authum_memory_alloc(64));
    ASSERT_NE(block, authum_null);
    lh_memory_std_copy(block, "keep", 5);
    lh_memory_allocator_set_realloc_cb(lh_runtime_allocator(), lh_null);
    auto *grown = static_cast<authum_byte_t *>(authum_memory_realloc(block, 64, 96));
    ASSERT_NE(grown, authum_null);
    block = grown;
    EXPECT_STREQ(reinterpret_cast<const authum_char_t *>(block), "keep");
    EXPECT_EQ(resizes, 0u);
    EXPECT_EQ(releases, 1u);
}

TEST_F(authum_memory, zero_resize_releases_block)
{
    block = static_cast<authum_byte_t *>(authum_memory_alloc(32));
    ASSERT_NE(block, authum_null);
    EXPECT_EQ(authum_memory_realloc(block, 32, 0), authum_null);
    block = authum_null;
    EXPECT_EQ(releases, 1u);
}

TEST_F(authum_memory, freeing_null_does_not_call_allocator)
{
    authum_memory_free(authum_null);
    EXPECT_EQ(releases, 0u);
}

} // namespace tests
