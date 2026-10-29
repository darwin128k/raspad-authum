#ifndef AUTHUM_LOGGER_OPTIONS_H
#define AUTHUM_LOGGER_OPTIONS_H

#include <authum/char.h>
#include <authum/size.h>

/** Built-in, host-owned formatters. No plugin callbacks run on the writer. */
typedef enum authum_logger_format
{
    authum_logger_format_inherit = 0,
    authum_logger_format_standard = 1,
    /** Message bytes only, with one terminating newline when needed. */
    authum_logger_format_raw = 2
} authum_logger_format_t;

/** Construction defaults; administrator configuration still takes precedence.
 * A named logger always stays under the module's logs/<name>/ sandbox. */
typedef struct authum_logger_options
{
    const authum_char_t *name;
    authum_logger_format_t format;
} authum_logger_options_t;

#endif
