#include "runtime_rom_properties.hpp"
#include "runtime_rom_observation.hpp"

#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace oasis::thor::runtime_map;

namespace {
Identity identity(std::string run = "run-a", std::uint64_t generation = 1) {
    return {std::string(64, 'a'), 8193, std::string(SCHEMA_ID), std::string(CONTRACT_SHA256),
            std::string(64, 'c'), std::move(run), generation, 0x7, "VALIDATED"};
}
template<class Fn> bool throws(Fn&& fn) {
    try { fn(); } catch (const std::exception&) { return true; }
    return false;
}
}

int main() {
    std::vector<std::uint8_t> m68k_rom(24, 0);
    const std::vector<std::uint8_t> instructions{
        0x66, 0x02,                         // BNE.S (same encoding when taken or not taken)
        0x66, 0x00, 0x00, 0x04,             // BNE.W, including its extension on the untaken path
        0x51, 0xC8, 0x00, 0x04,             // DBF D0
        0x4E, 0xB9, 0x00, 0x00, 0x01, 0x00, // JSR absolute long
        0x4E, 0x71, 0x4E, 0x75, 0x4E, 0x75, // NOP, RTS, RTS
    };
    std::copy(instructions.begin(), instructions.end(), m68k_rom.begin());
    PropertyMap execution_map(m68k_rom.size());
    const PhysicalOffsetResolver linear = [](std::uint32_t address) -> std::optional<std::uint64_t> {
        if (address >= 0x1000 && address < 0x1000 + 24) return address - 0x1000;
        return std::nullopt;
    };
    assert(mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1000, 0x6602, true, linear) == 2);
    PropertyMap untaken_branch_map(m68k_rom.size());
    assert(mark_m68k_executed_encoding(untaken_branch_map, m68k_rom, 0x1000, 0x6602, true, linear) == 2);
    assert(untaken_branch_map.at(1) == execution_map.at(1));
    assert(mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1002, 0x6600, true, linear) == 4);
    assert(mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1006, 0x51C8, true, linear) == 4);
    assert(mark_m68k_executed_encoding(execution_map, m68k_rom, 0x100A, 0x4EB9, true, linear) == 6);
    assert(mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1010, 0x4E71, true, linear) == 2);
    assert(mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1012, 0x4E75, true, linear) == 2);
    assert(execution_map.at(5) & M68K_EXECUTED_ENCODING); // Bcc.W extension byte.
    assert(!mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1016, 0x4E75, false, linear));
    assert(!mark_m68k_executed_encoding(execution_map, m68k_rom, 4, 0x0000, true, linear)); // Reset vector is not execution.
    assert(!mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1000, 0x6600, true, linear));
    const PhysicalOffsetResolver boundary = [](std::uint32_t address) -> std::optional<std::uint64_t> {
        if (address < 0x1000 || address >= 0x1018) return std::nullopt;
        return (address - 0x1000) + (address >= 0x1004 ? 8 : 0);
    };
    assert(!mark_m68k_executed_encoding(execution_map, m68k_rom, 0x1002, 0x6600, true, boundary));
    assert(mark_resolved_data_read(execution_map, m68k_rom, 0x1000,
           std::span<const std::uint8_t>(m68k_rom.data(), 2), true, M68K_DATA_READ, linear));
    assert(execution_map.at(0) == (M68K_DATA_READ | M68K_EXECUTED_ENCODING));
    assert(!mark_resolved_data_read(execution_map, m68k_rom, 0x1000,
           std::span<const std::uint8_t>(m68k_rom.data(), 2), false, M68K_DATA_READ, linear));
    const std::uint8_t changed_read[]{0, 1};
    assert(!mark_resolved_data_read(execution_map, m68k_rom, 0x1000, changed_read,
                                    true, M68K_DATA_READ, linear));

    std::vector<std::uint8_t> z80_rom{0xDD, 0xFD, 0xDD, 0xCB, 0xFE, 0x46, 0x18, 0xFA, 0xCD, 0x00, 0x01, 0xC9};
    PropertyMap z80_map(z80_rom.size());
    std::vector<Z80FetchedByte> prefixed;
    for (std::size_t i = 0; i < z80_rom.size(); ++i) prefixed.push_back({z80_rom[i], i});
    assert(mark_z80_executed_encoding(z80_map, z80_rom,
           std::span<const Z80FetchedByte>(prefixed.data(), 6), true));
    assert(z80_map.at(3) & Z80_EXECUTED_ENCODING); // DD/FD and DDCB operand sequence.
    std::vector<std::uint8_t> banked_rom(32);
    std::vector<Z80FetchedByte> banked_fetch;
    for (std::size_t i = 0; i < 6; ++i) {
        const std::size_t offset = i * 3;
        banked_rom[offset] = z80_rom[i];
        banked_fetch.push_back({z80_rom[i], offset});
    }
    PropertyMap banked_map(banked_rom.size());
    assert(mark_z80_executed_encoding(banked_map, banked_rom, banked_fetch, true));
    assert(banked_map.at(15) & Z80_EXECUTED_ENCODING); // Per-byte bank mapping, no PC delta.
    auto untracked_ram_fetch = prefixed;
    untracked_ram_fetch[0].rom_origin.reset();
    assert(!mark_z80_executed_encoding(z80_map, z80_rom, untracked_ram_fetch, true));
    auto modified_copy = prefixed;
    modified_copy[1].value ^= 1;
    assert(!mark_z80_executed_encoding(z80_map, z80_rom, modified_copy, true));
    assert(!mark_z80_executed_encoding(z80_map, z80_rom, prefixed, false));
    assert(mark_z80_executed_encoding(z80_map, z80_rom,
           std::span<const Z80FetchedByte>(prefixed.data() + 6, 2), true)); // JR displacement.
    assert(mark_z80_executed_encoding(z80_map, z80_rom,
           std::span<const Z80FetchedByte>(prefixed.data() + 8, 3), true)); // CALL absolute.
    assert(mark_z80_executed_encoding(z80_map, z80_rom,
           std::span<const Z80FetchedByte>(prefixed.data() + 11, 1), true)); // RET.
    std::vector<Z80FetchedByte> overflow(257, {0xDD, 0});
    assert(!mark_z80_executed_encoding(z80_map, z80_rom, overflow, true));

    PropertyMap vdp_map(8);
    assert(mark_vdp_consumed_source(vdp_map, 2, 2, true, VDP_VRAM_SOURCE));
    assert(vdp_map.at(2) == VDP_VRAM_SOURCE && vdp_map.at(3) == VDP_VRAM_SOURCE);
    assert(!mark_vdp_consumed_source(vdp_map, 4, 2, false, VDP_CRAM_SOURCE));
    assert(!mark_vdp_consumed_source(vdp_map, 4, 2, true, M68K_DATA_READ)); // Fill/copy have no ROM-source class.
    assert(!mark_vdp_consumed_source(vdp_map, 7, 2, true, VDP_VSRAM_SOURCE));
    assert(mark_vdp_consumed_source(vdp_map, 6, 2, true, VDP_CRAM_SOURCE));

    RomOriginMemory ram_origins(8);
    ram_origins.copy_rom_to_ram(1, 2, std::span<const std::uint8_t>(z80_rom.data() + 2, 2), z80_rom);
    assert(ram_origins.origin(1) == 2 && ram_origins.origin(2) == 3);
    PropertyMap copied_vdp_map(z80_rom.size());
    assert(mark_vdp_ram_sources(copied_vdp_map, ram_origins, 1, 2, true, VDP_VRAM_SOURCE) == 2);
    assert(copied_vdp_map.at(2) & VDP_VRAM_SOURCE);
    ram_origins.copy_ram_to_ram(3, 1, 2);
    assert(ram_origins.origin(3) == 2 && ram_origins.origin(4) == 3);
    ram_origins.invalidate(4, 1); // Unknown writes and transforms discard origin.
    assert(!ram_origins.origin(4));
    assert(mark_vdp_ram_sources(copied_vdp_map, ram_origins, 3, 2, true, VDP_CRAM_SOURCE) == 1);
    assert(!(copied_vdp_map.at(3) & VDP_CRAM_SOURCE) &&
           (copied_vdp_map.at(2) & VDP_CRAM_SOURCE));
    const std::uint8_t changed_byte = static_cast<std::uint8_t>(z80_rom[2] ^ 0xff);
    ram_origins.copy_rom_to_ram(6, 2, std::span<const std::uint8_t>(&changed_byte, 1), z80_rom);
    assert(!ram_origins.origin(6));
    assert(throws([&] { ram_origins.copy_ram_to_ram(7, 0, 2); }));

    PropertyMap map(8193);
    assert(map.bytes().size() * sizeof(std::uint16_t) == 16386);
    assert(map.at(0) == 0); // UNKNOWN is represented by no proven properties.
    map.apply_validated(10, 4, M68K_EXECUTED_ENCODING);
    map.apply_validated(12, 3, M68K_DATA_READ);
    map.apply_validated(4095, 2, M68K_DATA_READ);
    assert(map.at(12) == (M68K_EXECUTED_ENCODING | M68K_DATA_READ));
    assert(map.dirty_pages().size() == 3 && map.dirty_pages()[0] && map.dirty_pages()[1]);
    assert(throws([&] { map.apply_validated(8192, 2, M68K_DATA_READ); }));
    assert(throws([&] { map.apply_validated(0, 1, 0x8000); }));

    const auto ranges = map.ranges();
    auto restored = PropertyMap::from_ranges(map.size(), ranges);
    assert(restored.bytes() == map.bytes());
    assert(ranges.front().start == 0 && ranges.front().properties == 0);
    assert(ranges.back().end == 8193 && ranges.back().properties == 0);
    assert(throws([&] { (void)PropertyMap::from_ranges(8193, {{1, 8193, 0}}); }));
    assert(throws([&] { (void)PropertyMap::from_ranges(8193, {{0, 9000, 0}}); }));

    auto id = identity();
    auto encoded = encode_checkpoint(id, map);
    Identity decoded_id;
    auto decoded = decode_checkpoint(encoded, decoded_id);
    assert(decoded.bytes() == map.bytes());
    assert(decoded_id.rom_sha256 == id.rom_sha256 && decoded_id.generation == id.generation);
    auto corrupt = encoded; corrupt[corrupt.size() / 2] ^= 1;
    assert(throws([&] { Identity ignored; (void)decode_checkpoint(corrupt, ignored); }));
    auto wrong_identity = id; wrong_identity.rom_sha256[0] = 'c';
    assert(throws([&] { require_compatible(id, wrong_identity); }));

    PropertyMap second(8193); second.apply_validated(12, 2, VDP_CRAM_SOURCE);
    Contribution a{id, map.bytes()};
    Contribution b{identity("run-b", 4), second.bytes()};
    const auto ab = rebuild_union({a, b}, id);
    const auto ba = rebuild_union({b, a}, id);
    const auto aa = rebuild_union({a, a}, id);
    assert(ab.bytes() == ba.bytes() && aa.bytes() == a.properties);
    assert(ab.at(12) == (M68K_EXECUTED_ENCODING | M68K_DATA_READ | VDP_CRAM_SOURCE));
    auto incompatible = b; incompatible.identity.core_build_id = "other-build";
    assert(throws([&] { (void)rebuild_union({a, incompatible}, id); }));

    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
        ("thor-rom-properties-test-" + std::to_string(suffix) + ".bin");
    std::error_code ec; std::filesystem::remove(path, ec);
    save_contribution(path, a);
    const auto loaded = load_contribution(path);
    assert(loaded.identity.run_id == id.run_id && loaded.properties == a.properties);
    auto checkpoint_path = path; checkpoint_path += ".checkpoint";
    save_checkpoint(checkpoint_path, id, map);
    Identity loaded_identity;
    assert(load_checkpoint(checkpoint_path, loaded_identity).bytes() == map.bytes());
    auto invalid_id = id; invalid_id.contract_sha256[0] = '0';
    assert(throws([&] { save_checkpoint(checkpoint_path, invalid_id, map); }));
    Identity after_failed_save;
    assert(load_checkpoint(checkpoint_path, after_failed_save).bytes() == map.bytes());
    assert(rebuild_union({a}, id).bytes() == a.properties);
    PropertyMap dirty(8193); dirty.apply_validated(5, 1, Z80_DATA_READ);
    dirty.clear_dirty(); assert(dirty.dirty_pages()[0] == 0);
    std::filesystem::remove(path, ec);
    std::filesystem::remove(checkpoint_path, ec);
}
