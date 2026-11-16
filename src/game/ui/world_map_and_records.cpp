#include "game/ui/world_map_and_records.hpp"

#include <utility>

namespace oasis::game::ui {

namespace {

inline constexpr std::array<std::pair<std::int8_t, std::int8_t>, 29> kDefaultOverworldOffsets = {{
    {52, 24}, {12, 32}, {-20, 28}, {-20, 44}, {-8, -20}, {-12, 28}, {4, 16}, {20, 32},
    {36, 32}, {12, 8}, {52, 24}, {8, -16}, {52, -16}, {-52, 28}, {-40, 16}, {-36, 28},
    {-24, 16}, {-12, 0}, {-24, -16}, {4, -36}, {12, -44}, {0, -56}, {-16, -48}, {-28, -40},
    {-64, -28}, {-72, -32}, {8, 34}, {12, 44}, {12, 16},
}};

inline constexpr std::array<std::pair<std::int8_t, std::int8_t>, 16> kDefaultDungeonPins = {{
    {0, 0}, {41, 40}, {17, 9}, {55, 17}, {56, 7}, {46, -12}, {-4, -17}, {16, -54},
    {-19, -36}, {-16, 32}, {-30, 27}, {-21, -13}, {-21, -13}, {-70, 7}, {-16, 32}, {16, 34},
}};

inline constexpr std::array<std::uint8_t, 6> kRank00Magic = {'R', 'a', 'n', 'k', '0', '0'};
inline constexpr std::array<std::uint8_t, 6> kRace00Magic = {'R', 'a', 'c', 'e', '0', '0'};

} // namespace

WorldMapPin compute_world_map_pin(
    std::span<const std::uint8_t> rom,
    std::uint16_t dungeon_id_ff0e40,
    std::uint8_t area_id_ff0e41,
    std::int32_t player_x,
    std::int32_t player_y) noexcept {
    if (dungeon_id_ff0e40 != 0U) {
        std::size_t idx = (dungeon_id_ff0e40 == 0x0F01U)
            ? static_cast<std::size_t>((dungeon_id_ff0e40 & 0x0FU) + 6U)
            : static_cast<std::size_t>(dungeon_id_ff0e40);
        if (idx >= WorldMapAndRecordsConfig::kDungeonPinCount) {
            idx = 0U;
        }
        std::int8_t dx = kDefaultDungeonPins[idx].first;
        std::int8_t dy = kDefaultDungeonPins[idx].second;
        const std::size_t rom_off =
            static_cast<std::size_t>(WorldMapAndRecordsConfig::kDungeonTableAddr) + idx * 2U;
        if (rom_off + 2U <= rom.size()) {
            dx = static_cast<std::int8_t>(rom[rom_off]);
            dy = static_cast<std::int8_t>(rom[rom_off + 1U]);
        }
        return WorldMapPin{
            WorldMapAndRecordsConfig::kScreenCenterX + dx,
            WorldMapAndRecordsConfig::kScreenCenterY + dy,
        };
    }

    const std::size_t idx = (area_id_ff0e41 < WorldMapAndRecordsConfig::kOverworldAreaCount)
        ? static_cast<std::size_t>(area_id_ff0e41)
        : 0U;
    std::int8_t dx = kDefaultOverworldOffsets[idx].first;
    std::int8_t dy = kDefaultOverworldOffsets[idx].second;
    const std::size_t rom_off =
        static_cast<std::size_t>(WorldMapAndRecordsConfig::kOverworldTableAddr) + idx * 2U;
    if (rom_off + 2U <= rom.size()) {
        dx = static_cast<std::int8_t>(rom[rom_off]);
        dy = static_cast<std::int8_t>(rom[rom_off + 1U]);
    }
    return WorldMapPin{
        WorldMapAndRecordsConfig::kScreenCenterX + dx + (player_x >> 6),
        WorldMapAndRecordsConfig::kScreenCenterY + dy + (player_y >> 6),
    };
}

std::vector<std::uint8_t> encode_rank00_sram_block(std::span<const std::uint8_t> payload_130) {
    std::vector<std::uint8_t> sram(WorldMapAndRecordsConfig::kRank00SramBytes, 0x00U);
    for (std::size_t i = 0U; i < kRank00Magic.size(); ++i) {
        sram[i * 2U] = kRank00Magic[i];
    }
    std::uint16_t sum = 0U;
    for (std::size_t i = 0U; i < WorldMapAndRecordsConfig::kRank00PayloadBytes; ++i) {
        const std::uint8_t b = (i < payload_130.size()) ? payload_130[i] : 0U;
        sram[(6U + i) * 2U] = b;
        sum = static_cast<std::uint16_t>(sum + b);
    }
    sram[(6U + WorldMapAndRecordsConfig::kRank00PayloadBytes) * 2U] =
        static_cast<std::uint8_t>((sum >> 8U) & 0xFFU);
    sram[(6U + WorldMapAndRecordsConfig::kRank00PayloadBytes + 1U) * 2U] =
        static_cast<std::uint8_t>(sum & 0xFFU);
    return sram;
}

std::optional<std::array<std::uint8_t, WorldMapAndRecordsConfig::kRank00PayloadBytes>>
decode_rank00_sram_block(std::span<const std::uint8_t> sram_odd_bytes) noexcept {
    if (sram_odd_bytes.size() < WorldMapAndRecordsConfig::kRank00SramBytes) {
        return std::nullopt;
    }
    for (std::size_t i = 0U; i < kRank00Magic.size(); ++i) {
        if (sram_odd_bytes[i * 2U] != kRank00Magic[i]) {
            return std::nullopt;
        }
    }
    std::array<std::uint8_t, WorldMapAndRecordsConfig::kRank00PayloadBytes> payload{};
    std::uint16_t sum = 0U;
    for (std::size_t i = 0U; i < WorldMapAndRecordsConfig::kRank00PayloadBytes; ++i) {
        const std::uint8_t b = sram_odd_bytes[(6U + i) * 2U];
        payload[i] = b;
        sum = static_cast<std::uint16_t>(sum + b);
    }
    const std::uint16_t stored_sum = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(sram_odd_bytes[(6U + WorldMapAndRecordsConfig::kRank00PayloadBytes) * 2U]) << 8U) |
        static_cast<std::uint16_t>(sram_odd_bytes[(6U + WorldMapAndRecordsConfig::kRank00PayloadBytes + 1U) * 2U]));
    if (stored_sum != sum) {
        return std::nullopt;
    }
    return payload;
}

std::optional<std::uint32_t> decode_race00_best_time(
    std::span<const std::uint8_t> sram_odd_bytes) noexcept {
    if (sram_odd_bytes.size() < WorldMapAndRecordsConfig::kRace00SramBytes) {
        return std::nullopt;
    }
    for (std::size_t i = 0U; i < kRace00Magic.size(); ++i) {
        if (sram_odd_bytes[i * 2U] != kRace00Magic[i]) {
            return std::nullopt;
        }
    }
    return (static_cast<std::uint32_t>(sram_odd_bytes[6U * 2U]) << 24U) |
           (static_cast<std::uint32_t>(sram_odd_bytes[7U * 2U]) << 16U) |
           (static_cast<std::uint32_t>(sram_odd_bytes[8U * 2U]) << 8U) |
           static_cast<std::uint32_t>(sram_odd_bytes[9U * 2U]);
}

std::optional<std::uint32_t> lookup_attract_demo_rom_addr(
    std::span<const std::uint8_t> rom,
    std::uint8_t slot) noexcept {
    if (slot >= WorldMapAndRecordsConfig::kAttractDemoCount) {
        return std::nullopt;
    }
    const std::size_t entry_addr =
        static_cast<std::size_t>(WorldMapAndRecordsConfig::kAttractTableAddr) +
        static_cast<std::size_t>(slot) * 2U;
    if (entry_addr + 2U > rom.size()) {
        return std::nullopt;
    }
    const std::uint16_t rel = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(rom[entry_addr]) << 8U) |
        static_cast<std::uint16_t>(rom[entry_addr + 1U]));
    const std::uint32_t target = WorldMapAndRecordsConfig::kAttractStreamBaseAddr + rel;
    if (target >= rom.size()) {
        return std::nullopt;
    }
    return target;
}

std::vector<std::uint8_t> delta_decode_demo_stream(std::span<const std::uint8_t> bytes) {
    std::vector<std::uint8_t> out(bytes.size(), 0U);
    std::uint8_t acc = 0U;
    for (std::size_t i = 0U; i < bytes.size(); ++i) {
        acc = static_cast<std::uint8_t>(acc + bytes[i]);
        out[i] = acc;
    }
    return out;
}

} // namespace oasis::game::ui
