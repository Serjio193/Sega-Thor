#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace oasis::game::spirits {

// ROM 0x018CAA: Spirit Awakening Dispatcher configuration entry.
// Dispatched when player interacts with altar/prop entity 0x0020 (0x00835E -> 0x008392 -> 0x018CAA).
struct ShrineEventConfig {
    std::uint16_t room_id{0};                  // $FF0E40.W (Area << 8 | Room)
    std::uint32_t handler_address{0};          // ROM address of shrine handler
    std::uint32_t spawn_helper_address{0};     // ROM address of spirit spawn helper
    std::uint16_t message_id{0};               // Dialogue table 0x051514 message ID
    std::uint16_t global_flag_idx{0};          // Global flag index in $FF0DBC (bank 1)
    std::optional<std::uint8_t> active_spirit_bit{}; // Bit index in $FF0D7A (0=Dytto, 1=Efreet, 2=Shade, 3=Bow)
    std::optional<std::uint16_t> area_flag_idx{};    // Area flag index in $FF0E1C (bank 2), e.g. $FF0E1D:2 -> 10
    std::int16_t elevation_delta_px{0};        // Z spawn offset above altar prop ($10(A4) + delta)
};

// All 5 branches of Spirit Awakening Dispatcher 0x018CAA.
inline constexpr std::array<ShrineEventConfig, 5> kShrines = {{
    {0x0106U, 0x018CDEU, 0x014668U, 0U,  3U,  std::uint8_t{0U}, std::uint16_t{10U}, 16}, // Dytto (Water Shrine)
    {0x0206U, 0x018D4EU, 0x0146D6U, 9U,  27U, std::uint8_t{1U}, std::nullopt,       8},  // Efreet (Fire Shrine)
    {0x0805U, 0x018DAAU, 0x014742U, 12U, 63U, std::uint8_t{2U}, std::nullopt,       16}, // Shade (Shadow Shrine)
    {0x0A10U, 0x018E12U, 0x014798U, 13U, 20U, std::uint8_t{3U}, std::nullopt,       0},  // Bow (Plant Shrine)
    {0x090BU, 0x018E7AU, 0x00F53CU, 56U, 54U, std::nullopt,     std::nullopt,       0},  // Agito (Seal Chamber)
}};

// Models ROM 0x018CAA: compares current room_id ($FF0E40.W) against shrine table.
[[nodiscard]] std::optional<ShrineEventConfig> dispatch_spirit_awakening(std::uint16_t room_id) noexcept;

} // namespace oasis::game::spirits
