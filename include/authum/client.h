/**
 * @file client.h
 * @brief Umbrella for Authum **client modules** (no lh headers).
 *
 * Include this from your module `.c` and link `authum_client` — not liblh.
 */

#ifndef AUTHUM_CLIENT_H
#define AUTHUM_CLIENT_H

#include <authum/array.h>
#include <authum/memory.h>
#include <authum/cfg.h>
#include <authum/detour.h>
#include <authum/event/client.h>
#include <authum/event/message.h>
#include <authum/event/server.h>
#include <authum/client/ref.h>
#include <authum/json.h>
#include <authum/logger/buffer.h>
#include <authum/module/api.h>
#include <authum/module/info.h>
#include <authum/net.h>
#include <authum/net/address.h>
#include <authum/sql/thread.h>
#include <authum/timer.h>
#include <authum/unused.h>
#include <authum/symbol.h>

#endif /* AUTHUM_CLIENT_H */
