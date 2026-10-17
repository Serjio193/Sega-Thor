#include "game/player/player.hpp"

#include "game/world/terrain_collision.hpp"

#include <cstdint>

namespace oasis::game::player {
namespace {

struct DirectionEntry {
    Direction direction;
    std::int8_t x_sign;
    std::int8_t y_sign;
    bool diagonal;
    std::uint16_t rom_code;
};

// This is the observable result of the 0x85FA dispatch table. Values 3, C and
// F (and the impossible opposite-direction combinations) intentionally stop.
constexpr DirectionEntry kDirectionTable[16]{
    {Direction::none, 0, 0, false, 0},
    {Direction::up, 0, -1, false, 0},
    {Direction::down, 0, 1, false, 2},
    {Direction::none, 0, 0, false, 0},
    {Direction::left, -1, 0, false, 3},
    {Direction::up_left, -1, -1, true, 0},
    {Direction::down_left, -1, 1, true, 2},
    {Direction::left, -1, 0, false, 3},
    {Direction::right, 1, 0, false, 1},
    {Direction::up_right, 1, -1, true, 0},
    {Direction::down_right, 1, 1, true, 2},
    {Direction::right, 1, 0, false, 1},
    {Direction::none, 0, 0, false, 0},
    {Direction::up, 0, -1, false, 0},
    {Direction::down, 0, 1, false, 2},
    {Direction::none, 0, 0, false, 0},
};

std::int32_t world_coordinate(std::int32_t fixed) noexcept {
    return fixed / 0x10000;
}

void stop_movement(PlayerState& player) noexcept {
    player.intent_x_fixed = 0;
    player.intent_y_fixed = 0;
    player.movement_state = 0;
}

std::uint16_t direction_bit(std::uint16_t direction_code) noexcept {
    constexpr std::uint16_t bits[4]{0, 3, 1, 2};
    return bits[direction_code & 3U];
}

std::int32_t approach_zero(std::int32_t value) noexcept {
    constexpr std::int32_t step = 0xACCC;
    if (value > 0) return value > step ? value - step : 0;
    if (value < 0) return value < -step ? value + step : 0;
    return 0;
}

bool cardinal_footprint_mask(std::uint8_t mask) noexcept {
    switch (mask & 0x0FU) {
    case 0x1U:
    case 0x2U:
    case 0x4U:
    case 0x8U:
        return true;
    default:
        return false;
    }
}

} // namespace

Direction direction_from_input(std::uint8_t input_nibble) noexcept {
    return kDirectionTable[input_nibble & 0x0FU].direction;
}

MovementVector movement_vector(std::uint8_t input_nibble,
                               const PlayerMovementConfig& config) noexcept {
    const auto entry = kDirectionTable[input_nibble & 0x0FU];
    if (entry.direction == Direction::none) {
        return {};
    }

    const auto x_speed = entry.diagonal ? config.diagonal_x_speed
                                        : config.cardinal_x_speed;
    const auto y_speed = entry.diagonal ? config.diagonal_y_speed
                                        : config.cardinal_y_speed;
    return MovementVector{entry.direction,
                          static_cast<std::int32_t>(entry.x_sign * x_speed),
                          static_cast<std::int32_t>(entry.y_sign * y_speed)};
}

void adjust_velocity_for_context(std::int32_t& x_fixed,
                                 std::int32_t& y_fixed,
                                 const VelocityAdjustContext& context) noexcept {
    if (!context.available || context.flag_ff1985 || context.flag_ff16f1_bit4) {
        return;
    }
    if (context.flag_ff1984) {
        x_fixed >>= 1;
        y_fixed >>= 1;
        return;
    }
    if (!cardinal_footprint_mask(context.footprint_any_bits)) {
        y_fixed >>= 1;
    }
}

MovementVector update_movement_state(
    PlayerState& player,
    std::uint8_t input_nibble,
    const PlayerMovementConfig& config,
    const VelocityAdjustContext& velocity_context) noexcept {
    const auto input = static_cast<std::uint8_t>(input_nibble & 0x0FU);
    if (player.movement_state == 4U) {
        const auto& entry = kDirectionTable[input];
        const auto selected_bit = direction_bit(player.direction_code);
        if ((input & static_cast<std::uint8_t>(1U << selected_bit)) == 0U) {
            if (player.turn_timer < 6U) {
                stop_movement(player);
                return {};
            }
            player.movement_state = 0x0CU;
            player.intent_x_fixed = approach_zero(player.intent_x_fixed);
            player.intent_y_fixed = approach_zero(player.intent_y_fixed);
            player.accumulated_x_fixed += player.intent_x_fixed;
            player.accumulated_y_fixed += player.intent_y_fixed;
            return {};
        }

        const auto diagonal_x = config.diagonal_x_speed;
        const auto diagonal_y = config.diagonal_y_speed;
        MovementVector result{entry.direction, 0, 0};
        if ((player.orientation_flags & 1U) == 0U) {
            auto retained_y = player.intent_y_fixed;
            std::int32_t ignored_x = 0;
            adjust_velocity_for_context(ignored_x, retained_y, velocity_context);
            player.accumulated_y_fixed += retained_y;
            if ((input & (1U << 2U)) != 0U) {
                player.accumulated_x_fixed -= diagonal_x;
                result.x_fixed -= diagonal_x;
            }
            if ((input & (1U << 3U)) != 0U) {
                player.accumulated_x_fixed += diagonal_x;
                result.x_fixed += diagonal_x;
            }
            result.y_fixed = retained_y;
        } else {
            auto retained_x = player.intent_x_fixed;
            std::int32_t ignored_y = 0;
            adjust_velocity_for_context(retained_x, ignored_y, velocity_context);
            player.accumulated_x_fixed += retained_x;
            if ((input & (1U << 0U)) != 0U) {
                player.accumulated_y_fixed -= diagonal_y;
                result.y_fixed -= diagonal_y;
            }
            if ((input & (1U << 1U)) != 0U) {
                player.accumulated_y_fixed += diagonal_y;
                result.y_fixed += diagonal_y;
            }
            result.x_fixed = retained_x;
        }
        ++player.turn_timer;
        return result;
    }

    const auto vector = movement_vector(input, config);
    if (vector.direction == Direction::none) {
        stop_movement(player);
        return {};
    }

    player.direction_code = kDirectionTable[input].rom_code;
    player.intent_x_fixed = vector.x_fixed;
    player.intent_y_fixed = vector.y_fixed;
    player.accumulated_x_fixed += vector.x_fixed;
    player.accumulated_y_fixed += vector.y_fixed;
    player.movement_state = 2;
    return vector;
}

MovementResult try_move(PlayerState& player,
                        const core::ControllerState& controller,
                        const world::ByteGridView& terrain,
                        const PlayerMovementConfig& config,
                        const VelocityAdjustContext& velocity_context) noexcept {
    const auto input = static_cast<std::uint8_t>(controller.buttons & 0x0FU);
    const auto vector = update_movement_state(player, input, config, velocity_context);
    MovementResult result{vector, false, false};
    if (vector.direction == Direction::none && player.movement_state == 0U) {
        return result;
    }

    const auto candidate_x = player.x_fixed + player.accumulated_x_fixed;
    const auto candidate_y = player.y_fixed + player.accumulated_y_fixed;
    if (candidate_x < 0 || candidate_y < 0) {
        player.accumulated_x_fixed = 0;
        player.accumulated_y_fixed = 0;
        result.blocked = true;
        return result;
    }
    const auto aggregate = terrain.aggregate_world_square(
        world_coordinate(candidate_x), world_coordinate(candidate_y),
        config.footprint_radius);
    if (!aggregate) {
        player.accumulated_x_fixed = 0;
        player.accumulated_y_fixed = 0;
        result.blocked = true;
        return result;
    }
    player.footprint_any_bits = aggregate->any_bits;

    const auto prospective_state = world::terrain_state_from_code(
        aggregate->common_terrain_code());
    const auto gate = world::evaluate_terrain_gate(
        world::TerrainGateInput{config.entity_flags, player.terrain_state,
                                prospective_state, aggregate->any_bits});
    if (gate == world::TerrainGateResult::blocked) {
        player.accumulated_x_fixed = 0;
        player.accumulated_y_fixed = 0;
        result.blocked = true;
        return result;
    }

    player.x_fixed = candidate_x;
    player.y_fixed = candidate_y;
    player.terrain_state = prospective_state;
    player.accumulated_x_fixed = 0;
    player.accumulated_y_fixed = 0;
    result.moved = true;
    return result;
}

bool update_player_hurt_knockback(
    PlayerState& player,
    const world::ByteGridView* terrain) noexcept {
    if (player.invuln_timer > 0) {
        --player.invuln_timer;
    }
    if (player.hurt_timer == 0) {
        return false;
    }

    --player.hurt_timer;
    const auto candidate_x = player.x_fixed + player.knockback_x_fixed;
    const auto candidate_y = player.y_fixed + player.knockback_y_fixed;

    bool blocked = false;
    if (candidate_x < 0 || candidate_y < 0) {
        blocked = true;
    } else if (terrain != nullptr && terrain->valid()) {
        const auto code = terrain->terrain_code_world(
            world_coordinate(candidate_x),
            world_coordinate(candidate_y));
        if (!code || world::evaluate_terrain_gate({
            .entity_flags = 0,
            .current_state = 0,
            .prospective_state = world::terrain_state_from_code(*code),
            .prospective_any_bits = 0
        }) == world::TerrainGateResult::blocked) {
            blocked = true;
        }
    }

    if (!blocked) {
        player.x_fixed = candidate_x;
        player.y_fixed = candidate_y;
    }

    if (player.hurt_timer == 0) {
        player.movement_state = 0; // State 0 (recovered back to normal control)
        player.knockback_x_fixed = 0;
        player.knockback_y_fixed = 0;
    }
    return true;
}

void apply_enemy_contact_damage(
    PlayerState& player,
    std::uint16_t attacker_type,
    std::uint8_t attacker_dir,
    std::int16_t damage_amount,
    std::uint8_t knockback_frames,
    std::uint8_t invuln_duration) noexcept {
    player.contact_entity = attacker_type;
    if (player.invuln_timer > 0 || player.hurt_timer > 0) {
        player.damage_gate_open = false;
        return;
    }
    player.damage_gate_open = true;
    player.hp = static_cast<std::int16_t>(std::max(0, player.hp - damage_amount));
    player.hurt_timer = knockback_frames;
    player.invuln_timer = invuln_duration;
    player.movement_state = 24; // State 24: Hurt in ROM
    constexpr std::int32_t kSpeed = 0x30000; // 3.0 px/frame cardinal from 0x00BF2E
    player.knockback_x_fixed = (attacker_dir == 1) ? kSpeed : ((attacker_dir == 3) ? -kSpeed : 0);
    player.knockback_y_fixed = (attacker_dir == 2) ? kSpeed : ((attacker_dir == 0) ? -kSpeed : 0);
}

bool start_player_attack(PlayerState& player) noexcept {
    if (player.is_hurt() || player.attack_state != 0) return false;
    player.attack_state = 10;
    player.attack_timer = 0;
    player.attack_active = false;
    player.attack_hit_latched = false;
    player.attack_direction = static_cast<std::uint8_t>(player.direction_code & 3U);
    return true;
}

void update_player_attack(PlayerState& player) noexcept {
    if (player.attack_state == 0) return;
    ++player.attack_timer;
    if (player.attack_timer >= 9) {
        player.attack_state = 0;
        player.attack_timer = 0;
        player.attack_active = false;
        player.attack_hit_latched = false;
    } else {
        player.attack_active = (player.attack_timer >= 4 && player.attack_timer <= 5);
    }
}

std::optional<entities::EntityHitbox> player_attack_hitbox(
    const PlayerState& player) noexcept {
    if (!player.attack_active) return std::nullopt;
    const auto px = static_cast<std::int32_t>(player.x_fixed / 0x10000);
    const auto py = static_cast<std::int32_t>(player.y_fixed / 0x10000);
    switch (player.attack_direction & 3U) {
    case 0: // Up (Dir 0)
        return entities::EntityHitbox{px - 8, py - 40, px + 7, py - 9, -8, 8};
    case 1: // Right (Dir 1)
        return entities::EntityHitbox{px + 8, py - 8, px + 47, py + 7, -8, 8};
    case 2: // Down (Dir 2)
        return entities::EntityHitbox{px - 8, py + 8, px + 7, py + 47, -8, 8};
    case 3: // Left (Dir 3)
    default:
        return entities::EntityHitbox{px - 47, py - 8, px - 8, py + 7, -8, 8};
    }
}

void apply_item_pickup(
    PlayerState& player,
    std::uint16_t item_id,
    std::uint8_t count) noexcept {
    player.last_pickup_type = item_id;
    player.item_count += count;
    if (item_id == 0x001E) { // Food / cheese (ROM 0x0053A2..0x0053E8): heals max_hp / 4 (25 HP)
        const auto heal_amount = static_cast<std::uint16_t>(player.max_hp / 4);
        player.hp = static_cast<std::int16_t>(std::min<std::int32_t>(player.max_hp, player.hp + heal_amount));
        player.last_pickup_heal = heal_amount;
    }
}

} // namespace oasis::game::player
