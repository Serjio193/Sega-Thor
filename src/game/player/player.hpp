#pragma once

#include "core/runtime.hpp"
#include "game/entities/screen_entity.hpp"
#include "game/world/byte_grid.hpp"

#include <cstdint>

namespace oasis::game::player {

inline constexpr std::uint32_t kPlayerEntityRamAddress = 0x00FF19E8;
inline constexpr std::uint32_t kPlayerUpdateRoutineAddress = 0x0000557A;
inline constexpr std::uint32_t kPlayerStateDispatchTableAddress = 0x000059BA;
inline constexpr std::uint32_t kPlayerMovementIntentRoutineAddress = 0x000061F6;
inline constexpr std::uint32_t kPlayerDirectionRoutineAddress = 0x000085E2;
inline constexpr std::uint32_t kPlayerVelocityAdjustRoutineAddress = 0x000064C4;
inline constexpr std::uint32_t kPlayerMovementRoutineAddress = 0x00008F12;
inline constexpr std::uint32_t kPlayerFootprintRoutineAddress = 0x00009BF2;
inline constexpr std::uint32_t kPlayerTerrainGateAddress = 0x0000938E;

enum class Direction : std::uint8_t {
    none,
    up,
    down,
    left,
    right,
    up_left,
    up_right,
    down_left,
    down_right,
};

struct PlayerMovementConfig {
    // These are 16.16 fixed-point deltas recovered from 0x85E2.
    std::int32_t cardinal_x_speed{0x36000};
    std::int32_t cardinal_y_speed{0x30000};
    std::int32_t diagonal_x_speed{0x2A000};
    std::int32_t diagonal_y_speed{0x25800};
    std::uint16_t footprint_radius{10};
    std::uint8_t entity_flags{};
};

struct VelocityAdjustContext {
    // These names intentionally preserve the observed external RAM sources.
    bool available{};
    bool flag_ff1985{};
    bool flag_ff1984{};
    bool flag_ff16f1_bit4{};
    std::uint8_t footprint_any_bits{};
};

struct PlayerState {
    // Positions use the same 16.16 fixed-point convention as movement deltas.
    std::int32_t x_fixed{};
    std::int32_t y_fixed{};
    std::int8_t terrain_state{-1};
    // Entity +0x04 selects the movement/update state in the 0x59BA table.
    std::uint16_t movement_state{};
    // These fields mirror only offsets observed by the verified movement path.
    std::uint16_t direction_code{}; // entity +0x16
    std::uint8_t orientation_flags{}; // entity +0x17
    std::int32_t intent_x_fixed{}; // entity +0x4E
    std::int32_t intent_y_fixed{}; // entity +0x52
    std::int32_t accumulated_x_fixed{}; // entity +0x72
    std::int32_t accumulated_y_fixed{}; // entity +0x76
    std::uint8_t footprint_any_bits{}; // entity +0x6F, produced by 0x9BF2
    std::uint16_t turn_timer{}; // FF197E in the state-4 branch
    std::int16_t hp{100};       // entity +0x8a in ROM (Max HP in +0x88 is 100)
    std::int16_t max_hp{100};   // entity +0x88 in ROM
    std::uint8_t invuln_timer{0};    // entity +0x82 in ROM (decremented each frame)
    std::uint8_t hurt_timer{0};      // entity +0x83 in ROM (knockback duration, 6 frames)
    std::int32_t knockback_x_fixed{0}; // entity +0x4e in ROM (fixed point 16.16)
    std::int32_t knockback_y_fixed{0}; // entity +0x52 in ROM (fixed point 16.16)
    std::uint16_t contact_entity{0}; // type ID of harmful entity in contact
    bool damage_gate_open{false};    // true when contact passes damage predicates
    std::uint8_t attack_state{0};       // 0: idle, 10: State 10 (Player Attack in ROM)
    std::uint8_t attack_timer{0};       // 0..8 (9 frames total: 4 startup, 2 active, 3 recovery)
    bool attack_active{false};          // true during active frames (4 and 5)
    std::uint8_t attack_direction{0};   // locked facing direction during attack
    bool attack_hit_latched{false};     // bit 3 of $37(a6): hit once per swing latch
    std::int16_t last_hit_damage{0};    // last damage dealt to enemy
    std::int16_t sp{100};               // entity +0x8e in ROM (current SP / magic points)
    std::int16_t max_sp{100};           // entity +0x8c in ROM (max SP / magic points)
    std::uint16_t currency{0};          // gold / currency
    std::uint16_t item_count{0};        // count of items collected
    std::uint16_t last_pickup_type{0};  // item ID of most recent pickup (e.g. 0x001E)
    std::uint16_t last_pickup_heal{0};  // HP restored by most recent pickup
    std::uint8_t secondary_weapon{1};       // 0x00FF0DB1: 1 (Knife), 6 (Bow)
    std::uint8_t secondary_slot{0};         // 0x00FF0DB0: active slot index 0..7
    std::uint8_t secondary_ammo{0xFF};      // 0x00FF198E: current weapon ammo
    std::uint8_t secondary_attack_state{0}; // 0: idle, 14: State 14 (Bow Attack in ROM)
    std::uint8_t secondary_attack_timer{0}; // 0..15 ticks
    bool secondary_attack_fired{false};     // arrow spawned at frame 12
    std::uint8_t secondary_attack_direction{0}; // locked facing direction
    std::uint16_t last_weapon_hit{0};       // type ID of enemy hit by secondary weapon

    [[nodiscard]] constexpr bool is_hurt() const noexcept { return hurt_timer > 0; }
    [[nodiscard]] constexpr bool is_invulnerable() const noexcept { return invuln_timer > 0; }
    [[nodiscard]] constexpr bool is_attacking() const noexcept { return attack_state != 0 || secondary_attack_state != 0; }
    [[nodiscard]] constexpr bool is_secondary_attacking() const noexcept { return secondary_attack_state != 0; }
};

struct MovementVector {
    Direction direction{Direction::none};
    std::int32_t x_fixed{};
    std::int32_t y_fixed{};
};

struct MovementResult {
    MovementVector vector{};
    bool moved{};
    bool blocked{};
};

[[nodiscard]] Direction direction_from_input(std::uint8_t input_nibble) noexcept;

[[nodiscard]] MovementVector movement_vector(
    std::uint8_t input_nibble,
    const PlayerMovementConfig& config = {}) noexcept;

// Update the confirmed input/state portion of the player path. Collision is
// intentionally left to try_move, which consumes the accumulated deltas.
[[nodiscard]] MovementVector update_movement_state(
    PlayerState& player,
    std::uint8_t input_nibble,
    const PlayerMovementConfig& config = {},
    const VelocityAdjustContext& velocity_context = {}) noexcept;

void adjust_velocity_for_context(
    std::int32_t& x_fixed,
    std::int32_t& y_fixed,
    const VelocityAdjustContext& context) noexcept;

[[nodiscard]] MovementResult try_move(
    PlayerState& player,
    const core::ControllerState& controller,
    const world::ByteGridView& terrain,
    const PlayerMovementConfig& config = {},
    const VelocityAdjustContext& velocity_context = {}) noexcept;

[[nodiscard]] bool update_player_hurt_knockback(
    PlayerState& player,
    const world::ByteGridView* terrain) noexcept;

void apply_enemy_contact_damage(
    PlayerState& player,
    std::uint16_t attacker_type,
    std::uint8_t attacker_dir,
    std::int16_t damage_amount = 10,
    std::uint8_t knockback_frames = 6,
    std::uint8_t invuln_duration = 30) noexcept;

bool start_player_attack(PlayerState& player) noexcept;

void update_player_attack(PlayerState& player) noexcept;

[[nodiscard]] std::optional<entities::EntityHitbox> player_attack_hitbox(
    const PlayerState& player) noexcept;

void apply_item_pickup(
    PlayerState& player,
    std::uint16_t item_id,
    std::uint8_t count = 1) noexcept;

} // namespace oasis::game::player
