/**
 * @file fields.h
 * @brief Per-module event facade members.
 *
 * The ops table and borrowed module owner preserve the original ABI layout.
 */

#ifndef AUTHUM_EVENTS_FIELDS_H
#define AUTHUM_EVENTS_FIELDS_H

/** @brief Declare the dispatch table and module identity using supplied types. */
#define authum_events_fields(ops_type, owner_type)                                                 \
    ops_type ops;                                                                                  \
    owner_type owner

#endif /* AUTHUM_EVENTS_FIELDS_H */
