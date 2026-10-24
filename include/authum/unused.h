/**
 * @file unused.h
 * @brief Mark a parameter or value as deliberately unused.
 *
 * Event handlers and commands share one signature, so most of them leave a
 * parameter or two alone; `authum_unused(user);` says so and silences the
 * compiler's unused warning.
 */

#ifndef AUTHUM_UNUSED_H
#define AUTHUM_UNUSED_H

/**
 * @def authum_unused(x)
 * @brief Use @p x without doing anything with it. A statement, like a call:
 *        `authum_unused(user);`.
 */
#define authum_unused(x) ((void)(x))

#endif /* AUTHUM_UNUSED_H */
