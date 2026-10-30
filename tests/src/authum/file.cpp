/**
 * @file file.cpp
 * @brief The module's own file: it opens, writes, reads and closes.
 */

#include <authum/file.h>

#include <gtest/gtest.h>

#include <stdio.h>

#define TEST_PATH "authum_file_test.txt"

TEST(authum_file, reads_and_writes)
{
    authum_file_t file;
    authum_char_t text[32];
    authum_usize_t count = 0u;

    authum_file_init(&file);
    EXPECT_FALSE(authum_file_open_cstr(&file, TEST_PATH, authum_file_mode_read));
    EXPECT_FALSE(authum_file_is_open(&file));

    EXPECT_TRUE(authum_file_open_cstr(&file, TEST_PATH, authum_file_mode_write));
    EXPECT_TRUE(authum_file_is_open(&file));
    EXPECT_TRUE(authum_file_write(&file, "hello", 6u, &count));
    EXPECT_EQ(count, 6u);
    authum_file_close(&file);
    EXPECT_FALSE(authum_file_is_open(&file));

    EXPECT_TRUE(authum_file_open_cstr(&file, TEST_PATH, authum_file_mode_read));
    EXPECT_TRUE(authum_file_read(&file, &text, sizeof text, &count));
    EXPECT_EQ(count, 6u);
    EXPECT_STREQ(text, "hello");
    /* The end of the file is a read of zero bytes, not a failure. */
    EXPECT_TRUE(authum_file_read(&file, &text, sizeof text, &count));
    EXPECT_EQ(count, 0u);
    authum_file_close(&file);

    remove(TEST_PATH);
}

TEST(authum_file, opens_a_path_object)
{
    authum_file_t file;
    authum_path_t path;

    authum_file_init(&file);
    EXPECT_TRUE(authum_path_set_cstr(&path, TEST_PATH));
    EXPECT_TRUE(authum_file_open(&file, &path, authum_file_mode_write));
    EXPECT_TRUE(authum_file_is_open(&file));
    authum_file_close(&file);
    remove(TEST_PATH);
}

TEST(authum_file, reads_the_whole_file)
{
    authum_file_t file;
    authum_char_t text[16];
    authum_usize_t count = 0u;

    authum_file_init(&file);
    EXPECT_TRUE(authum_file_open_cstr(&file, TEST_PATH, authum_file_mode_write));
    EXPECT_TRUE(authum_file_write(&file, "0123456789", 10u, &count));
    authum_file_close(&file);

    authum_file_init(&file);
    EXPECT_TRUE(authum_file_open_cstr(&file, TEST_PATH, authum_file_mode_read));
    EXPECT_TRUE(authum_file_read_all(&file, &text, sizeof text, &count));
    EXPECT_EQ(count, 10u);
    EXPECT_STREQ(text, "0123456789");
    authum_file_close(&file);

    /* A buffer that is too small is refused, not truncated. */
    authum_file_init(&file);
    EXPECT_TRUE(authum_file_open_cstr(&file, TEST_PATH, authum_file_mode_read));
    EXPECT_FALSE(authum_file_read_all(&file, &text, 4u, &count));
    authum_file_close(&file);

    remove(TEST_PATH);
}

TEST(authum_file, refuses_a_closed_file)
{
    authum_file_t file;
    authum_char_t text[8];
    authum_usize_t count = 0u;

    authum_file_init(&file);
    authum_file_close(&file);
    EXPECT_FALSE(authum_file_read(&file, &text, sizeof text, &count));
    EXPECT_FALSE(authum_file_write(&file, "x", 1u, &count));
}
