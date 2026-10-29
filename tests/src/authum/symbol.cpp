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
