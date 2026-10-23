/**
 * @file host.h
 * @brief Host side of ::authum_engine_t — implemented in `authum` only.
 *
 * The engine does not keep a player's address; `ClientConnect` is the only
 * place it is seen. The DLL hooks hand it here so
 * ::authum_engine_t.client_get_address can answer later.
 */

#ifndef AUTHUM_ENGINE_HOST_H
#define AUTHUM_ENGINE_HOST_H

#include <authum/byte.h>
#include <authum/char.h>
#include <authum/engine.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** The table the host puts on every ::authum_host_t. */
    const authum_engine_t *
    authum_engine(void);

    /** Open / close the precache window (worldspawn's Spawn … ServerActivate). */
    void
    authum_engine_host_set_precache(authum_bool_t open);

    /** Remember @p address (`ip:port`) for slot @p index; null forgets it. */
    void
    authum_engine_host_set_client_address(authum_i32_t index, const authum_char_t *address);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENGINE_HOST_H */
