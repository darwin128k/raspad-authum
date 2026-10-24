/**
 * @file gate.c
 * @brief Connect-gate lifetime (hook install lands in a later 1.0.0 step).
 */

#include <authum/host/gate.h>

#include <lh/bool.h>

lh_bool_t
authum_gate_init(void)
{
    /* Next: port qproto's engine layout resolve + JMP on
     * Steam_NotifyClientConnect, then module-registered authorizers
     * (Vellum first). Until then Authum only hosts modules. */
    return lh_bool_true;
}

void
authum_gate_deinit(void)
{
}
