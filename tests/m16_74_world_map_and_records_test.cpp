#include "game/ui/world_map_and_records.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace {

void write_be16(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t v) {
    buf[off] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>(v & 0xFFU);
}

} // namespace

int main() {
    using oasis::game::ui::compute_world_map_pin;
    using oasis::game::ui::decode_race00_best_time;
    using oasis::game::ui::decode_rank00_sram_block;
    using oasis::game::ui::delta_decode_demo_stream;
    using oasis::game::ui::encode_rank00_sram_block;
    using oasis::game::ui::lookup_attract_demo_rom_addr;
    using oasis::game::ui::WorldMapAndRecordsConfig;

    // 1. World Map Pin Math (0x004B4E): overworld + dungeon + 0x0F01 (+6).
    const auto ow_pin = compute_world_map_pin({}, 0U, 0x01U, 256, 128);
    assert(ow_pin.screen_x == 160 + 12 + (256 >> 6));
    assert(ow_pin.screen_y == 120 + 32 + (128 >> 6));

    const auto dg_pin = compute_world_map_pin({}, 0x0001U, 0U, 0, 0);
    assert(dg_pin.screen_x == 201 && dg_pin.screen_y == 160);

    const auto dg_0f01 = compute_world_map_pin({}, 0x0F01U, 0U, 0, 0);
    assert(dg_0f01.screen_x == 176 && dg_0f01.screen_y == 66); // index 1+6 = 7 -> (+16, -54)

    // 2. "Rank00" odd-byte SRAM encode/decode + checksum verification.
    std::array<std::uint8_t, WorldMapAndRecordsConfig::kRank00PayloadBytes> payload{};
    payload[0] = 0x12U;
    payload[129] = 0xABU;
    auto sram_rank = encode_rank00_sram_block(payload);
    assert(sram_rank.size() == WorldMapAndRecordsConfig::kRank00SramBytes);
    const auto decoded_rank = decode_rank00_sram_block(sram_rank);
    assert(decoded_rank.has_value());
    assert((*decoded_rank)[0] == 0x12U && (*decoded_rank)[129] == 0xABU);

    // Corrupt checksum -> std::nullopt.
    sram_rank[sram_rank.size() - 2U] ^= 0xFFU;
    assert(!decode_rank00_sram_block(sram_rank).has_value());

    // 3. "Race00" odd-byte SRAM best time decode.
    std::vector<std::uint8_t> sram_race(WorldMapAndRecordsConfig::kRace00SramBytes, 0U);
    const char magic[] = "Race00";
    for (std::size_t i = 0U; i < 6U; ++i) sram_race[i * 2U] = static_cast<std::uint8_t>(magic[i]);
    sram_race[6U * 2U] = 0x00U;
    sram_race[7U * 2U] = 0x01U;
    sram_race[8U * 2U] = 0xE2U;
    sram_race[9U * 2U] = 0x40U;
    assert(decode_race00_best_time(sram_race) == 0x0001E240U);

    // 4. Attract demo table (0x008A94 -> 0x1698FE + rel) & delta-decoder (0x008A30).
    std::vector<std::uint8_t> rom(0x170000U, 0x00U);
    write_be16(rom, WorldMapAndRecordsConfig::kAttractTableAddr + 0U * 2U, 0U);
    write_be16(rom, WorldMapAndRecordsConfig::kAttractTableAddr + 1U * 2U, 1068U);
    write_be16(rom, WorldMapAndRecordsConfig::kAttractTableAddr + 8U * 2U, 8222U);

    assert(lookup_attract_demo_rom_addr(rom, 0U) == 0x001698FEU);
    assert(lookup_attract_demo_rom_addr(rom, 1U) == 0x00169D2AU);
    assert(lookup_attract_demo_rom_addr(rom, 8U) == 0x0016B91CU);
    assert(!lookup_attract_demo_rom_addr(rom, 9U).has_value());

    const std::array<std::uint8_t, 4> delta_in = {0x10U, 0x20U, 0xF0U, 0x05U};
    const auto delta_out = delta_decode_demo_stream(delta_in);
    assert((delta_out == std::vector<std::uint8_t>{0x10U, 0x30U, 0x20U, 0x25U}));

    return 0;
}
