/**
 * @file entity.h
 * @brief Entity fields a module may read and write without HLSDK.
 *
 * The fields are the engine's own (`entvars_t`), the same in every GoldSrc
 * game: health, frags, origin, flags… Game-specific data (a CS player's money
 * or team) lives in the game's private data and is not covered — use the
 * `engine_funcs` escape hatch for that.
 *
 * Entities are addressed by index: 0 is the world, 1 … maxClients are the
 * players, the rest is everything else. The numbers below never change;
 * new fields are only added at the end.
 */

#ifndef AUTHUM_ENGINE_ENTITY_H
#define AUTHUM_ENGINE_ENTITY_H

#ifdef __cplusplus
extern "C"
{
#endif

    /** Number fields (`authum_engine_entity_get_float` / `_set_float`). */
    enum
    {
        authum_entity_float_health = 0,
        authum_entity_float_armor = 1,
        authum_entity_float_armor_type = 2,
        authum_entity_float_frags = 3,
        authum_entity_float_max_health = 4,
        /** 1 = normal gravity. */
        authum_entity_float_gravity = 5,
        authum_entity_float_max_speed = 6,
        /** 0 = cannot be hurt (godmode). */
        authum_entity_float_take_damage = 7,
        authum_entity_float_friction = 8,
        authum_entity_float_fov = 9,
        /** Render amount (transparency with a render mode). */
        authum_entity_float_render_amount = 10,
        /** Damage taken this frame. */
        authum_entity_float_damage_taken = 11
    };

    /** Integer fields (`authum_engine_entity_get_i32` / `_set_i32`). */
    enum
    {
        /** `authum_entity_flag_*` bits. */
        authum_entity_i32_flags = 0,
        /** `authum_entity_dead_*`. */
        authum_entity_i32_dead = 1,
        /** The engine's team field (games like CS keep the real team elsewhere). */
        authum_entity_i32_team = 2,
        authum_entity_i32_move_type = 3,
        authum_entity_i32_solid = 4,
        authum_entity_i32_effects = 5,
        /** Buttons held this frame (IN_* bits). */
        authum_entity_i32_buttons = 6,
        authum_entity_i32_old_buttons = 7,
        authum_entity_i32_spawn_flags = 8,
        /** Bit per weapon id the player carries. */
        authum_entity_i32_weapons = 9,
        authum_entity_i32_render_mode = 10,
        authum_entity_i32_render_fx = 11,
        authum_entity_i32_impulse = 12,
        authum_entity_i32_player_class = 13,
        /** 0 dry … 3 head under water. */
        authum_entity_i32_water_level = 14
    };

    /** Vector fields, 3 numbers (`authum_engine_entity_get_vector` / `_set_vector`). */
    enum
    {
        /** Setting it moves the entity properly (SET_ORIGIN). */
        authum_entity_vector_origin = 0,
        authum_entity_vector_velocity = 1,
        authum_entity_vector_angles = 2,
        /** Where a player looks. */
        authum_entity_vector_view_angle = 3,
        authum_entity_vector_view_offset = 4,
        /** Setting mins or maxs resizes the entity (SET_SIZE). */
        authum_entity_vector_mins = 5,
        authum_entity_vector_maxs = 6,
        authum_entity_vector_punch_angle = 7,
        authum_entity_vector_render_color = 8,
        authum_entity_vector_angular_velocity = 9
    };

    /** Bits of ::authum_entity_i32_flags (the engine's FL_*). */
    enum
    {
        authum_entity_flag_fly = 1 << 0,
        authum_entity_flag_swim = 1 << 1,
        authum_entity_flag_client = 1 << 3,
        authum_entity_flag_in_water = 1 << 4,
        authum_entity_flag_god_mode = 1 << 6,
        authum_entity_flag_no_target = 1 << 7,
        authum_entity_flag_on_ground = 1 << 9,
        authum_entity_flag_frozen = 1 << 12,
        /** A bot. */
        authum_entity_flag_fake_client = 1 << 13,
        authum_entity_flag_ducking = 1 << 14,
        authum_entity_flag_proxy = 1 << 20,
        authum_entity_flag_spectator = 1 << 26
    };

    /** Values of ::authum_entity_i32_dead (the engine's DEAD_*). */
    enum
    {
        authum_entity_dead_no = 0,
        authum_entity_dead_dying = 1,
        authum_entity_dead_dead = 2,
        authum_entity_dead_respawnable = 3,
        authum_entity_dead_discard_body = 4
    };

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_ENGINE_ENTITY_H */
