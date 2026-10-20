/**
 * @file sink.h
 * @brief Where ::authum_logger_t writes (module-side bricks).
 *
 * Bit values match the common `LoggingMode` cfg convention (1/2/4) so modules
 * can pass that integer straight into ::authum_logger_init.
 */

#ifndef AUTHUM_LOGGER_SINK_H
#define AUTHUM_LOGGER_SINK_H

#include <authum/byte.h>

/** No sinks — every record dropped. */
#define authum_logger_sink_none 0u
/** HLDS console via ::authum_engine_server_print. */
#define authum_logger_sink_console 1u
/** Module-private file (::authum_logger_open_file). */
#define authum_logger_sink_file 2u
/** Host channel (`host->log` → `authum.log`). */
#define authum_logger_sink_host 4u

/**
 * @typedef authum_logger_sink_t
 * @brief Bitmask of `authum_logger_sink_*` (may combine).
 */
typedef authum_u32_t authum_logger_sink_t;

#endif /* AUTHUM_LOGGER_SINK_H */
