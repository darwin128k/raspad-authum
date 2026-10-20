/**
 * @file byte.h
 * @brief 8-bit unsigned byte for Authum modules (no lh).
 *
 * Mirrors ::lh_byte_t for the module ABI. Use for raw binary; text is
 * ::authum_char_t in <authum/char.h>.
 */

#ifndef AUTHUM_BYTE_H
#define AUTHUM_BYTE_H

/**
 * @typedef authum_u8_t
 * @brief Exactly 8-bit unsigned integer.
 */
typedef unsigned char authum_u8_t;

/**
 * @typedef authum_byte_t
 * @brief Semantic “one byte” name (::authum_u8_t).
 */
typedef authum_u8_t authum_byte_t;

/**
 * @typedef authum_u32_t
 * @brief 32-bit unsigned (Authum modules target 32-bit HLDS).
 */
typedef unsigned int authum_u32_t;

/**
 * @typedef authum_i32_t
 * @brief 32-bit signed (`int` on the HLDS ABI).
 */
typedef int authum_i32_t;

#endif /* AUTHUM_BYTE_H */
