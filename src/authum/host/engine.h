/**
 * @file engine.h
 * @brief Host side of ::authum_engine_t — implemented in `authum` only.
 *
 * The engine does not keep a player's address; `ClientConnect` is the only
 * place it is seen. The DLL hooks hand it here so
 * ::authum_engine_t.client_get_address can answer later.
 */

#ifndef AUTHUM_HOST_ENGINE_H
#define AUTHUM_HOST_ENGINE_H

#include <lh/util/ptr.h>
#include <authum/engine.h>

AUTHUM_EXTERN_C_BEGIN

/** The table the host puts on every ::authum_host_t. */
const authum_engine_t lh_ptr_deref(authum_engine(void));

/** Open / close the precache window (worldspawn's Spawn … ServerActivate). */
void
authum_engine_host_set_precache(authum_bool_t open);

/** Remember @p address (`ip:port`) for slot @p index; null forgets it. */
void
authum_engine_host_set_client_address(authum_i32_t index, const authum_char_t *address);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_ENGINE_H */
