#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::combat {

struct EnemyLootConfig {
    static constexpr std::uint32_t kLootRollerAddr = 0x0000F476U;
    static constexpr std::uint16_t kNoDropSentinel = 0xFFFFU;
    static constexpr std::uint16_t kPickupEntityType = 0x0008U;
    static constexpr std::int32_t kPickupPopZVel16_16 = 0x00080000; // +8.0 px/frame
    static constexpr std::int32_t kPickupElevationBias16_16 = 0x00010000; // +1.0 px
    static constexpr std::uint16_t kPickupTimerJitterMask = 0x003FU; // prng & 0x3F (0..63)
    static constexpr std::uint32_t kItemFlagsTableRom = 0x0005CE16U;
    static constexpr std::size_t kEntityStructMinSize = 0xBCU;
    static constexpr std::uint8_t kFlags39Initial = 0x01U; // bset.b #0, $39
    static constexpr std::uint16_t kSubActionFloatingBlink = 2U; // move.w #2, $4

    // 64-byte Item Attribute Table at ROM 0x0005CE16
    static constexpr std::array<std::uint8_t, 64> kItemFlagsTable = {
        0x00U, 0xA1U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U,
        0x01U, 0x01U, 0x01U, 0x01U, 0x00U, 0xA2U, 0xA2U, 0xA2U,
        0xA2U, 0x04U, 0xA4U, 0xA4U, 0xA4U, 0xA4U, 0x88U, 0x88U,
        0x88U, 0x88U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0xA0U, 0xA0U, 0xA0U, 0xA0U, 0xA0U, 0xA0U, 0xA0U,
        0xA0U, 0xA0U, 0xA0U, 0xA0U, 0xA0U, 0xA0U, 0xA0U, 0xA0U
    };
};

struct EnemyDropTable {
    std::uint16_t prob1{0U};
    std::uint16_t item1_word{0U}; // (item_id << 8) | quantity
    std::uint16_t prob2{0U};
    std::uint16_t item2_word{0U};
};

struct PickupEntityState {
    std::uint16_t entity_type{EnemyLootConfig::kPickupEntityType};
    std::uint16_t timer_2{0U};
    std::uint16_t sub_action_4{0U};
    std::int32_t pos_x_16_16{0};
    std::int32_t pos_y_16_16{0};
    std::int32_t pos_z_16_16{0};
    std::int32_t vel_z_16_16{EnemyLootConfig::kPickupPopZVel16_16};
    std::uint16_t room_index_14{0U};
    std::uint16_t tile_attr_18{0U};
    std::uint8_t item_id{0U};
    std::uint8_t quantity{0U};
    std::uint8_t flags_39{EnemyLootConfig::kFlags39Initial};
};

// Checks bit 7 of ROM 0x0005CE16 item table. If bit 7 is set, returns true.
[[nodiscard]] constexpr bool item_has_flag_bit7(std::uint8_t item_id) noexcept {
    if (item_id < EnemyLootConfig::kItemFlagsTable.size()) {
        return (EnemyLootConfig::kItemFlagsTable[item_id] & 0x80U) != 0U;
    }
    return false;
}

// ROM 0x00F47A..0x00F4C6: Evaluates fixed drop or 2-tier drop table against rolls.
[[nodiscard]] std::optional<std::uint16_t> roll_enemy_loot(
    std::uint16_t fixed_drop,
    bool no_drop_flag,
    const EnemyDropTable& table,
    std::uint16_t roll1,
    std::uint16_t roll2) noexcept;

// Convenience overload with 4 separate table elements matching Python reference.
[[nodiscard]] std::optional<std::uint16_t> roll_enemy_loot(
    std::uint16_t fixed_drop,
    bool no_drop_flag,
    std::uint16_t prob1,
    std::uint16_t item1,
    std::uint16_t prob2,
    std::uint16_t item2,
    std::uint16_t roll1,
    std::uint16_t roll2) noexcept;

// ROM 0x00F4C6..0x00F534: Constructs pickup entity state from item word and enemy coordinates.
[[nodiscard]] PickupEntityState spawn_pickup_entity(
    std::uint16_t item_word,
    std::int32_t enemy_pos_x,
    std::int32_t enemy_pos_y,
    std::int32_t enemy_pos_z,
    std::uint16_t enemy_room_index,
    std::uint16_t tile_attr,
    std::uint16_t prng_word) noexcept;

// Serializes pickup entity state into 0xBC-byte entity structure.
void write_pickup_to_entity_slot(
    const PickupEntityState& state,
    std::span<std::uint8_t> slot_bytes) noexcept;

} // namespace oasis::game::combat
