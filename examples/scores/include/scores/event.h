/**
 * @file scores/event.h
 * @brief Public events of the `scores` example module — types only.
 *
 * The owner of an event declares its name and payload in a header like this
 * one. Other modules include it and talk through the event bus; they never
 * link `scores` or call its functions. If `scores` is not loaded, a query
 * simply comes back unanswered.
 *
 * Every payload starts with `size`; fields are only ever appended, so a
 * subscriber checks `size >= sizeof(what it knows)` before reading.
 */

#ifndef SCORES_EVENT_H
#define SCORES_EVENT_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/size.h>

/** Forward, published by `scores` whenever a player's score changes. */
#define SCORES_EVENT_CHANGED "scores.changed"

/** Query, answered by `scores`: fill `index`, publish, read `found` / `score`. */
#define SCORES_EVENT_QUERY "scores.query"

typedef struct scores_changed
{
    authum_usize_t size;
    /** Player entity index, 1 … maxClients. */
    authum_i32_t index;
    authum_i32_t score;
    /** How much it changed by. */
    authum_i32_t delta;
} scores_changed_t;

typedef struct scores_query
{
    authum_usize_t size;
    /** In: player entity index. */
    authum_i32_t index;
    /** Out: set by `scores` when it answered. */
    authum_bool_t found;
    authum_i32_t score;
} scores_query_t;

#endif /* SCORES_EVENT_H */
