/**
 * @file sandbox.c
 * @brief Host sandbox (lh_memory_tree) + client sandbox/array API.
 */

#include <check.h>

#include <authum/array.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/sandbox.h>
#include <authum/sandbox/host.h>

#include <string.h>

int g_destroyed;

void
test_sandbox_count_destroy(authum_ptr ptr)
{
    (void)ptr;
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

void
test_sandbox_alloc_and_unload(void)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_byte_t *a;
    authum_byte_t *b;
    authum_byte_t *child;
    authum_usize_t i;
    authum_bool_t zero = authum_bool_true;

    CHECK(authum_sandbox_is_valid(&sb));
    a = authum_sandbox_alloc(&sb, 64u);
    CHECK(a != authum_null);
    for (i = 0u; i < 64u; ++i) {
        if (a[i] != 0u) {
            zero = authum_bool_false;
        }
    }
    CHECK(zero);
    b = authum_sandbox_alloc(&sb, 8u);
    child = authum_sandbox_alloc_under(&sb, b, 8u);
    authum_sandbox_set_destructor(&sb, a, test_sandbox_count_destroy);
    authum_sandbox_set_destructor(&sb, child, test_sandbox_count_destroy);

    g_destroyed = 0;
    authum_sandbox_free(&sb, b);
    CHECK(g_destroyed == 1);

    a = authum_sandbox_realloc(&sb, a, 256u);
    CHECK(a != authum_null);

    authum_sandbox_host_root_destroy(sb.root);
    CHECK(g_destroyed == 2);
}

void
test_sandbox_nested(void)
{
    authum_sandbox_t root = test_sandbox_root();
    authum_sandbox_t nested;
    authum_ptr p;

    CHECK(authum_sandbox_open(&nested, &root));
    p = authum_sandbox_alloc(&nested, 16u);
    authum_sandbox_set_destructor(&nested, p, test_sandbox_count_destroy);
    g_destroyed = 0;
    authum_sandbox_close(&nested);
    CHECK(g_destroyed == 1);
    CHECK(!authum_sandbox_is_valid(&nested));

    CHECK(authum_sandbox_open(&nested, &root));
    p = authum_sandbox_alloc(&nested, 16u);
    authum_sandbox_set_destructor(&nested, p, test_sandbox_count_destroy);
    g_destroyed = 0;
    authum_sandbox_host_root_destroy(root.root);
    CHECK(g_destroyed == 1);

    CHECK(!authum_sandbox_open(&nested, authum_null));
    CHECK(authum_sandbox_alloc(authum_null, 4u) == authum_null);
}

/* A block that moves on realloc takes its children along: freeing the tree
 * afterwards must reach them through the new address (GCC 11 + LTO once left
 * them pointing at the freed block). */
void
test_sandbox_realloc_with_children(void)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_ptr parent = authum_sandbox_alloc(&sb, 16u);
    authum_ptr moved;
    int i;

    for (i = 0; i < 3; ++i) {
        authum_ptr child = authum_sandbox_alloc_under(&sb, parent, 8u);
        authum_sandbox_set_destructor(&sb, child, test_sandbox_count_destroy);
    }
    moved = authum_sandbox_realloc(&sb, parent, 1u << 16);
    CHECK(moved != authum_null);

    g_destroyed = 0;
    authum_sandbox_free(&sb, moved);
    CHECK(g_destroyed == 3);
    authum_sandbox_host_root_destroy(sb.root);
}

/* A size no heap can give is refused, not fatal. */
void
test_sandbox_absurd_size(void)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_ptr p = authum_sandbox_alloc(&sb, 8u);

    CHECK(authum_sandbox_alloc(&sb, (authum_usize_t)-1) == authum_null);
    CHECK(authum_sandbox_alloc(&sb, (authum_usize_t)-1 / 2u) == authum_null);
    CHECK(authum_sandbox_realloc(&sb, p, (authum_usize_t)-1 - 8u) == authum_null);
    CHECK(authum_sandbox_alloc(&sb, 8u) != authum_null);
    authum_sandbox_host_root_destroy(sb.root);
}

struct test_item
{
    authum_i32_t id;
    authum_char_t name[12];
};

void
test_array(void)
{
    authum_sandbox_t sb = test_sandbox_root();
    authum_array_t arr;
    struct test_item item;
    struct test_item *at;
    authum_i32_t i;

    CHECK(!authum_array_init(&arr, &sb, 0u));
    CHECK(authum_array_init(&arr, &sb, sizeof(struct test_item)));
    CHECK(authum_array_is_empty(&arr));
    for (i = 0; i < 100; ++i) {
        memset(&item, 0, sizeof item);
        item.id = i;
        CHECK(authum_array_push(&arr, &item) != authum_null);
    }
    CHECK(authum_array_size(&arr) == 100u);
    at = authum_array_at(&arr, 42u);
    CHECK(at != authum_null && at->id == 42);
    CHECK(authum_array_at(&arr, 100u) == authum_null);

    CHECK(authum_array_remove_at(&arr, 0u));
    at = authum_array_at(&arr, 0u);
    CHECK(at != authum_null && at->id == 1);
    CHECK(authum_array_size(&arr) == 99u);
    CHECK(!authum_array_remove_at(&arr, 99u));

    at = authum_array_push(&arr, authum_null);
    CHECK(at != authum_null && at->id == 0 && at->name[0] == '\0');

    authum_array_clear(&arr);
    CHECK(authum_array_is_empty(&arr));
    CHECK(authum_array_reserve(&arr, 500u));
    CHECK(arr.capacity >= 500u);

    /* Not deinit'ed on purpose: the sandbox root owns the storage. */
    authum_sandbox_host_root_destroy(sb.root);
}

int
main(void)
{
    test_sandbox_alloc_and_unload();
    test_sandbox_nested();
    test_sandbox_realloc_with_children();
    test_sandbox_absurd_size();
    test_array();
    return authum_check_report("sandbox");
}
