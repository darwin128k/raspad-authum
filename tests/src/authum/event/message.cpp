#include <authum/event/message.h>
#include <authum/unused.h>
#include <authum/null.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

static authum_event_message_t ev;

namespace tests
{
class authum_event_message : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        memset(&ev, 0, sizeof ev);
        ev.argc = 3;
        ev.args[0].kind = authum_message_arg_byte;
        ev.args[0].value = 2;
        ev.args[1].kind = authum_message_arg_string;
        ev.args[1].text = 0u;
        memcpy(ev.text, "ak47", 5);
        ev.text_used = 5u;
        ev.args[2].kind = authum_message_arg_coord;
        ev.args[2].real = 1.5f;
    }
};

TEST_F(authum_event_message, reads_integer_argument)
{
    EXPECT_EQ(authum_event_message_get_i32(&ev, 0), 2);
}

TEST_F(authum_event_message, reads_string_argument)
{
    EXPECT_STREQ(authum_event_message_get_string(&ev, 1), "ak47");
}

TEST_F(authum_event_message, reads_coordinate_argument)
{
    EXPECT_FLOAT_EQ(authum_event_message_get_float(&ev, 2), 1.5f);
}

TEST_F(authum_event_message, wrong_type_returns_empty_string)
{
    EXPECT_STREQ(authum_event_message_get_string(&ev, 0), "");
}

TEST_F(authum_event_message, out_of_range_integer_returns_zero)
{
    EXPECT_EQ(authum_event_message_get_i32(&ev, 3), 0);
}

TEST_F(authum_event_message, replaces_integer_argument)
{
    ASSERT_TRUE(authum_event_message_set_i32(&ev, 0, 7));
    EXPECT_EQ(authum_event_message_get_i32(&ev, 0), 7);
}

TEST_F(authum_event_message, refuses_changing_argument_type)
{
    EXPECT_FALSE(authum_event_message_set_i32(&ev, 1, 7));
    EXPECT_FALSE(authum_event_message_set_string(&ev, 0, "x"));
}

TEST_F(authum_event_message, replaces_string_argument)
{
    ASSERT_TRUE(authum_event_message_set_string(&ev, 1, "knife"));
    EXPECT_STREQ(authum_event_message_get_string(&ev, 1), "knife");
}

TEST_F(authum_event_message, replaces_coordinate_argument)
{
    ASSERT_TRUE(authum_event_message_set_float(&ev, 2, -3.0f));
    EXPECT_FLOAT_EQ(authum_event_message_get_float(&ev, 2), -3.0f);
}

TEST_F(authum_event_message, overflow_does_not_replace_previous_string)
{
    ASSERT_TRUE(authum_event_message_set_string(&ev, 1, "knife"));
    ev.text_used = authum_event_message_text_max - 2u;
    EXPECT_FALSE(authum_event_message_set_string(&ev, 1, "longer"));
    EXPECT_STREQ(authum_event_message_get_string(&ev, 1), "knife");
}

} // namespace tests
