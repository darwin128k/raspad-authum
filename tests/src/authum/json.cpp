#include <authum/json.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

static const char *reply =
    "{\"ip\":\"8.8.8.8\",\"countryCode\":\"US\",\"asn\":15169,"
    "\"isp\":\"Caf\\u00e9 \\\"Net\\\"\",\"note\":\"asn: 1\",\"big\":3000000000}";

TEST(authum_json, copies_string_property)
{
    authum_char_t text[32];
    authum_char_t small[4];
    authum_i32_t n = 0;
    EXPECT_TRUE(authum_json_object_copy_cstr(reply, "countryCode", text, sizeof text));
    EXPECT_STREQ(text, "US");
}

TEST(authum_json, decodes_unicode_and_escaped_quotes)
{
    authum_char_t text[32];
    authum_char_t small[4];
    authum_i32_t n = 0;
    /* Escapes are decoded, é to UTF-8 included. */
    EXPECT_TRUE(authum_json_object_copy_cstr(reply, "isp", text, sizeof text));
    EXPECT_STREQ(text, "Caf\xC3\xA9 \"Net\"");
}

TEST(authum_json, truncates_string_to_output_capacity)
{
    authum_char_t text[32];
    authum_char_t small[4];
    authum_i32_t n = 0;
    /* Truncated to the buffer. */
    EXPECT_TRUE(authum_json_object_copy_cstr(reply, "ip", small, sizeof small));
    EXPECT_STREQ(small, "8.8");
}

TEST(authum_json, refuses_non_string_or_missing_property)
{
    authum_char_t text[32];
    authum_char_t small[4];
    authum_i32_t n = 0;
    EXPECT_FALSE(authum_json_object_copy_cstr(reply, "asn", text, sizeof text)); /* not a string */
    EXPECT_FALSE(authum_json_object_copy_cstr(reply, "missing", text, sizeof text));
    EXPECT_STREQ(text, "");
}

TEST(authum_json, integer_read_checks_type_and_range)
{
    authum_char_t text[32];
    authum_char_t small[4];
    authum_i32_t n = 0;
    EXPECT_TRUE(authum_json_object_try_get_i32(reply, "asn", &n) && n == 15169);
    EXPECT_FALSE(authum_json_object_try_get_i32(reply, "big", &n)); /* past i32 */
    EXPECT_FALSE(authum_json_object_try_get_i32(reply, "ip", &n));
}

TEST(authum_json, field_lookup_does_not_parse_string_contents)
{
    authum_char_t text[32];
    authum_char_t small[4];
    authum_i32_t n = 0;
    /* "asn: 1" inside a string value is text, not a field. */
    EXPECT_EQ(authum_json_object_get_i32("{\"note\":\"asn: 1\"}", "asn", -1), -1);
    EXPECT_EQ(authum_json_object_get_i32("{\"asn\": 7}", "asn", -1), 7);
    EXPECT_EQ(authum_json_object_get_i32("{\"asn\": 7", "asn", -1), -1); /* broken JSON */
    EXPECT_EQ(authum_json_object_get_i32("[1,2]", "asn", -1), -1);
    EXPECT_EQ(authum_json_object_get_i32(authum_null, "asn", -1), -1);
}
