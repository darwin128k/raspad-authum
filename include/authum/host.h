/**
 * @file host.h
 * @brief Host facade passed into ::authum_module_api.start (no lh dependency).
 *
 * Filled by `authum_mm`. Pointers are valid for the life of the loaded module
 * (until `stop`). Escape hatch: link your own lh / other libs privately.
 */

#ifndef AUTHUM_HOST_H
#define AUTHUM_HOST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*authum_host_log_fn)(int is_error, const char *line);
typedef void *(*authum_host_get_ptr_fn)(void);

typedef struct authum_host
{
    authum_host_log_fn log;
    authum_host_get_ptr_fn engine_funcs;
    authum_host_get_ptr_fn globals;
} authum_host_t;

static inline void
authum_host_log_line(const authum_host_t *host, int is_error, const char *line)
{
    if (host != 0 && host->log != 0 && line != 0) {
        host->log(is_error, line);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_HOST_H */
