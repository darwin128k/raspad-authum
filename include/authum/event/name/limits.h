/**
 * @file limits.h
 * @brief Authum event-name length limit.
 *
 * The limit excludes the terminating NUL. LH's generic registry has no Authum
 * name limit; the host enforces this policy when resolving module event names.
 */

#ifndef AUTHUM_EVENT_NAME_LIMITS_H
#define AUTHUM_EVENT_NAME_LIMITS_H

/** @brief Maximum number of characters in a registered Authum event name. */
#define AUTHUM_EVENT_NAME_LENGTH_MAX 63u

#endif /* AUTHUM_EVENT_NAME_LIMITS_H */
