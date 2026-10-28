#include <authum/client.h>
#include <check.h>
#include <lh/runtime/allocator.h>
#include <lh/memory/std.h>
#include <lh/numeric/fixed/limits.h>
#include <stdlib.h>
#include <string.h>

static authum_usize_t allocations, releases, resizes;
static authum_bool_t fail;

static lh_ptr
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

static lh_void
release(lh_self_ptr context, lh_ptr block)
{
    authum_unused(context);
    ++releases;
    free(block);
}

static lh_ptr
resize(lh_self_ptr context, lh_ptr block, lh_usize_t old_size, lh_usize_t new_size)
{
    authum_unused(context);
    authum_unused(old_size);
    ++resizes;
    return fail ? authum_null : realloc(block, new_size);
}

int
main(void)
{
    lh_memory_allocator_t saved = *lh_runtime_allocator();
    lh_memory_allocator_init(lh_runtime_allocator(), allocate, release);
    lh_memory_allocator_set_realloc_cb(lh_runtime_allocator(), resize);
    CHECK(authum_null_eq(authum_memory_alloc(0)));
    CHECK(authum_null_eq(authum_memory_alloc_zero(LH_U32_T_MAX, 2)));
    CHECK(allocations == 0);
    authum_byte_t *block = authum_memory_alloc_zero(4, 8);
    CHECK(!authum_null_eq(block));
    if (block)
    {
        for (authum_usize_t index = 0; index < 32; ++index)
        {
            CHECK(block[index] == 0);
        }
        lh_memory_std_copy(block, "keep", 5);
        fail = authum_bool_true;
        CHECK(authum_null_eq(authum_memory_alloc(8)));
        CHECK(authum_null_eq(authum_memory_realloc(block, 32, 64)));
        CHECK_STR((const authum_char_t *)block, "keep");
        fail = authum_bool_false;
        authum_byte_t *grown = authum_memory_realloc(block, 32, 64);
        CHECK(!authum_null_eq(grown));
        if (!grown)
        {
            authum_memory_free(block);
            goto restore;
        }
        block = grown;
        CHECK_STR((const authum_char_t *)block, "keep");
        lh_memory_allocator_set_realloc_cb(lh_runtime_allocator(), lh_null);
        grown = authum_memory_realloc(block, 64, 96);
        CHECK(!authum_null_eq(grown));
        if (!grown)
        {
            authum_memory_free(block);
            goto restore;
        }
        block = grown;
        CHECK_STR((const authum_char_t *)block, "keep");
        CHECK(authum_null_eq(authum_memory_realloc(block, 96, 0)));
        CHECK(releases == 2);
    }
    authum_memory_free(authum_null);
    CHECK(resizes == 2);
restore:
    lh_memory_allocator_assign(lh_runtime_allocator(), &saved);
    return authum_check_report("memory facade");
}
