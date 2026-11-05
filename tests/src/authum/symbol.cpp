#include <authum/symbol.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#    define TEST_SYMBOL_LIBRARY "kernel32.dll"
#    define TEST_SYMBOL_NAME "lstrlenA"
#else
#    define TEST_SYMBOL_LIBRARY "libc.so.6"
#    define TEST_SYMBOL_NAME "strlen"
#endif

TEST(authum_symbol, finds_exported_symbol_in_loaded_library)
{
    authum_bool_t via_dynsym = authum_bool_false;
    EXPECT_NE(authum_symbol_find(TEST_SYMBOL_LIBRARY, TEST_SYMBOL_NAME, &via_dynsym), authum_null);
    EXPECT_TRUE(via_dynsym);
}

TEST(authum_symbol, missing_symbol_clears_lookup_route_flag)
{
    authum_bool_t via_dynsym = authum_bool_true;
    EXPECT_EQ(authum_symbol_find(TEST_SYMBOL_LIBRARY, "authum_no_such_symbol", &via_dynsym),
              authum_null);
    EXPECT_FALSE(via_dynsym);
}

TEST(authum_symbol, missing_library_returns_null)
{
    EXPECT_EQ(authum_symbol_find("authum_not_loaded.so", "strlen", authum_null), authum_null);
}

TEST(authum_symbol, absent_engine_returns_null)
{
    EXPECT_EQ(authum_symbol_find_engine("SVC_Info", authum_null), authum_null);
}

TEST(authum_symbol, refuses_missing_library_name)
{
    EXPECT_EQ(authum_symbol_find(authum_null, "strlen", authum_null), authum_null);
}

TEST(authum_symbol, pattern_matches_loaded_code_and_rejects_ambiguity)
{
    const auto *code = static_cast<const authum_byte_t *>(
        authum_symbol_find(TEST_SYMBOL_LIBRARY, TEST_SYMBOL_NAME, authum_null));
    ASSERT_NE(code, nullptr);
    authum_byte_t mask[32];
    memset(mask, 0xff, sizeof mask);
    authum_usize_t count = 0;
    EXPECT_NE(authum_symbol_find_pattern(TEST_SYMBOL_LIBRARY, code, mask, sizeof mask, &count),
              nullptr);
    EXPECT_EQ(count, 1u);
    mask[0] = 0;
    EXPECT_NE(authum_symbol_find_pattern(TEST_SYMBOL_LIBRARY, code, mask, 1, &count), nullptr);
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(authum_symbol_find_pattern(TEST_SYMBOL_LIBRARY, nullptr, mask, 1, &count), nullptr);
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(authum_symbol_find_pattern(TEST_SYMBOL_LIBRARY, code, mask, 0, &count), nullptr);
    EXPECT_EQ(count, 0u);
}
