#include "game/combat/enemy_loot_drop.hpp"

#include <cstring>

namespace oasis::game::combat {

namespace {

inline void write_be16(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline void write_be32(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) noexcept {
    if (offset + 4U <= buf.size()) {
        const auto u = static_cast<std::uint32_t>(val);
        buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
        buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
    }
}

} // namespace

std::optional<std::uint16_t> roll_enemy_loot(
    std::uint16_t fixed_drop,
    bool no_drop_flag,
    const EnemyDropTable& table,
    std::uint16_t roll1,
    std::uint16_t roll2) noexcept {
    if (fixed_drop != 0U) {
        if (fixed_drop == EnemyLootConfig::kNoDropSentinel) {
            return std::nullopt;
        }
        return fixed_drop;
    }
    if (no_drop_flag) {
        return std::nullopt;
    }
    if (table.prob1 != 0U && roll1 <= table.prob1) {
        return table.item1_word;
    }
    if (table.prob2 != 0U && roll2 <= table.prob2) {
        return table.item2_word;
    }
    return std::nullopt;
}

std::optional<std::uint16_t> roll_enemy_loot(
    std::uint16_t fixed_drop,
    bool no_drop_flag,
    std::uint16_t prob1,
    std::uint16_t item1,
    std::uint16_t prob2,
    std::uint16_t item2,
    std::uint16_t roll1,
    std::uint16_t roll2) noexcept {
    const EnemyDropTable table{
        .prob1 = prob1,
        .item1_word = item1,
        .prob2 = prob2,
        .item2_word = item2
    };
    return roll_enemy_loot(fixed_drop, no_drop_flag, table, roll1, roll2);
}

PickupEntityState spawn_pickup_entity(
    std::uint16_t item_word,
    std::int32_t enemy_pos_x,
    std::int32_t enemy_pos_y,
    std::int32_t enemy_pos_z,
    std::uint16_t enemy_room_index,
    std::uint16_t tile_attr,
    std::uint16_t prng_word) noexcept {
    PickupEntityState state{};
    state.entity_type = EnemyLootConfig::kPickupEntityType;
    state.timer_2 = prng_word & EnemyLootConfig::kPickupTimerJitterMask;

    const auto item_id = static_cast<std::uint8_t>((item_word >> 8U) & 0xFFU);
    const auto qty = static_cast<std::uint8_t>(item_word & 0xFFU);
    state.item_id = item_id;
    state.quantity = qty;

    if (!item_has_flag_bit7(item_id)) {
        state.sub_action_4 = EnemyLootConfig::kSubActionFloatingBlink;
    } else {
        state.sub_action_4 = 0U;
    }

    state.pos_x_16_16 = enemy_pos_x;
    state.pos_y_16_16 = enemy_pos_y;
    state.pos_z_16_16 = enemy_pos_z + EnemyLootConfig::kPickupElevationBias16_16;
    state.vel_z_16_16 = EnemyLootConfig::kPickupPopZVel16_16;
    state.room_index_14 = enemy_room_index;
    state.tile_attr_18 = tile_attr;
    state.flags_39 = EnemyLootConfig::kFlags39Initial;

    return state;
}

void write_pickup_to_entity_slot(
    const PickupEntityState& state,
    std::span<std::uint8_t> slot_bytes) noexcept {
    if (slot_bytes.size() < EnemyLootConfig::kEntityStructMinSize) {
        return;
    }
    std::memset(slot_bytes.data(), 0, EnemyLootConfig::kEntityStructMinSize);

    write_be16(slot_bytes, 0x00U, state.entity_type);
    write_be16(slot_bytes, 0x02U, state.timer_2);
    write_be16(slot_bytes, 0x04U, state.sub_action_4);
    write_be32(slot_bytes, 0x08U, state.pos_x_16_16);
    write_be32(slot_bytes, 0x0CU, state.pos_y_16_16);
    write_be32(slot_bytes, 0x10U, state.pos_z_16_16);
    write_be16(slot_bytes, 0x14U, state.room_index_14);
    write_be16(slot_bytes, 0x18U, state.tile_attr_18);
    slot_bytes[0x33U] = state.item_id;
    slot_bytes[0x39U] |= state.flags_39;
    slot_bytes[0x52U] = state.quantity;
    write_be32(slot_bytes, 0x56U, state.vel_z_16_16);
}

} // namespace oasis::game::combat
