/**
 * @file ops.c
 * @brief ::authum_logger_channel_t: its fields — see <authum/logger/ops.h>.
 */

#include <authum/logger/ops.h>

const authum_logger_ops_t *
authum_logger_channel_get_ops(const authum_logger_channel_t *self)
{
    return self->ops;
}

void
authum_logger_channel_set_ops(authum_logger_channel_t *self, const authum_logger_ops_t *value)
{
    self->ops = value;
}

authum_ptr
authum_logger_channel_get_owner(const authum_logger_channel_t *self)
{
    return self->owner;
}

void
authum_logger_channel_set_owner(authum_logger_channel_t *self, authum_ptr value)
{
    self->owner = value;
}
