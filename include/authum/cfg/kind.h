/**
 * @file kind.h
 * @brief Value kinds for ::authum_cfg_item_t bindings.
 */

#ifndef AUTHUM_CFG_KIND_H
#define AUTHUM_CFG_KIND_H

#include <authum/byte.h>

enum authum_cfg_kind
{
    authum_cfg_kind_bool = 0,
    authum_cfg_kind_i32 = 1,
    authum_cfg_kind_u32 = 2,
    authum_cfg_kind_cstr = 3
};
typedef enum authum_cfg_kind authum_cfg_kind_t;

#endif /* AUTHUM_CFG_KIND_H */
