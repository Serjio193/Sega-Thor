#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::ui {

struct WorldMapPin {
    std::int32_t screen_x{160};
    std::int32_t screen_y{120};
};

struct WorldMapAndRecordsConfig {
    // World Map (0x004950..0x004C27)
    static constexpr std::uint32_t kWorldMapEntryAddr = 0x00004950U;
    static constexpr std::uint32_t kPinCalcAddr = 0x00004B4EU;
    static constexpr std::uint32_t kOverworldTableAddr = 0x00004BAEU;
    static constexpr std::uint32_t kDungeonTableAddr = 0x00004BE8U;
    static constexpr std::int32_t kScreenCenterX = 160; // 0xA0
    static constexpr std::int32_t kScreenCenterY = 120; // 0x78
    static constexpr std::size_t kOverworldAreaCount = 29U;
    static constexpr std::size_t kDungeonPinCount = 16U;

    // SRAM Clear Records ("Rank00" at $2025CD, "Race00" at $2026E1, stride 2)
    static constexpr std::size_t kRank00PayloadBytes = 130U; // 0x82
    static constexpr std::size_t kRank00LogicalBytes = 6U + kRank00PayloadBytes + 2U;
    static constexpr std::size_t kRank00SramBytes = kRank00LogicalBytes * 2U;
    static constexpr std::size_t kRace00LogicalBytes = 6U + 4U;
    static constexpr std::size_t kRace00SramBytes = kRace00LogicalBytes * 2U;

    // Attract / Demo Stream Table (0x008A94, 9 slots relative to 0x1698FE)
    static constexpr std::uint32_t kAttractTableAddr = 0x00008A94U;
    static constexpr std::uint32_t kAttractStreamBaseAddr = 0x001698FEU;
    static constexpr std::uint8_t kAttractDemoCount = 9U;
    static constexpr std::size_t kAttractDeltaBytes = 0x1FFFU; // 8191
};

// ROM 0x004B4E: Computes Ali's blinking map pin on the World Map.
// - If `dungeon_id_ff0e40 != 0`: uses 0x004BE8 table (adding +6 when `dungeon_id_ff0e40 == 0x0F01`).
// - If `dungeon_id_ff0e40 == 0`: uses 0x004BAE (29 areas) + `(player_x >> 6, player_y >> 6)`.
[[nodiscard]] WorldMapPin compute_world_map_pin(
    std::span<const std::uint8_t> rom,
    std::uint16_t dungeon_id_ff0e40,
    std::uint8_t area_id_ff0e41,
    std::int32_t player_x,
    std::int32_t player_y) noexcept;

// ROM 0x001E46: Encodes 130-byte "Rank00" payload + u16_be byte-sum checksum at odd-byte stride 2.
[[nodiscard]] std::vector<std::uint8_t> encode_rank00_sram_block(
    std::span<const std::uint8_t> payload_130);

// ROM 0x001DDC: Decodes and verifies "Rank00" odd-byte SRAM block. Returns std::nullopt on
// magic or checksum mismatch.
[[nodiscard]] std::optional<std::array<std::uint8_t, WorldMapAndRecordsConfig::kRank00PayloadBytes>>
decode_rank00_sram_block(std::span<const std::uint8_t> sram_odd_bytes) noexcept;

// ROM 0x001ECE: Decodes and verifies "Race00" + u32_be best time at odd-byte stride 2.
[[nodiscard]] std::optional<std::uint32_t> decode_race00_best_time(
    std::span<const std::uint8_t> sram_odd_bytes) noexcept;

// ROM 0x008A94: Resolves `0x1698FE + u16_be(0x008A94 + slot * 2)` for `slot < 9`.
[[nodiscard]] std::optional<std::uint32_t> lookup_attract_demo_rom_addr(
    std::span<const std::uint8_t> rom,
    std::uint8_t slot) noexcept;

// ROM 0x008A30: Prefix-sum mod 256 delta-decoder for attract demo input streams.
[[nodiscard]] std::vector<std::uint8_t> delta_decode_demo_stream(
    std::span<const std::uint8_t> bytes);

} // namespace oasis::game::ui
