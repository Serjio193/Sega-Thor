#include "game/render/bg_tile_dma.hpp"

#include "core/rom.hpp"
#include "core/sha256.hpp"
#include "game/resource_loader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using namespace thor::render;

void require_check(bool condition, const char* expr) {
    if (!condition) {
        std::cerr << "[FAIL] M16.187 check failed: " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr)

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) {
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void test_constants_and_struct_layout() {
    static_assert(kDecompressAndDmaBgTilesAddr == 0x00D3B2U);
    static_assert(kDecompressAndDmaBgTilesEndAddr == 0x00D406U);
    static_assert(kBgTilePatternTableAddr == 0x05CE96U);
    static_assert(kBgTileDecompressBufferRamAddr == 0x00FF2FA8U);
    static_assert(kBgTileDecompressBufferRamOffset == 0x2FA8U);
    static_assert(kBgTileDmaBusSource == 0x007F97D4U);
    static_assert((kBgTileDecompressBufferRamAddr >> 1U) == kBgTileDmaBusSource);
    static_assert(kBgTileDmaLengthWords == 0x0800U);
    static_assert(kBgTileDmaByteSize == 4096U);
    static_assert(kDmaQueuePtrRamOffset == 0x1892U);
    static_assert(kVintSyncFlagsRamOffset == 0x164EU);
    static_assert(kBgTileDmaSyncBit == 0x02U);

    constexpr BgTileDmaCommand default_cmd{};
    static_assert(default_cmd.src_bus_addr == 0x007F97D4U);
    static_assert(default_cmd.vram_dest == 0U);
    static_assert(default_cmd.length_words == 0x0800U);

    static_assert(oasis::game::render::kBgTilePatternTableAddr == kBgTilePatternTableAddr);
    static_assert(sizeof(oasis::game::render::BgTileDmaCommand) == sizeof(BgTileDmaCommand));
}

std::vector<std::uint8_t> make_synthetic_rom_with_streams() {
    // Format A stream producing 4096 bytes:
    // 8 literal bytes + 4088 repeated copies of 0x5A (extended repeat: 0x0FF4 + 4 = 4088).
    // Span = 2 + 9 + 3 = 14 (0x000E), trailing terminator = 0x00.
    const std::array<std::uint8_t, 15> stream_4k{
        0x0EU, 0x00U,
        0x08U, 0x12U, 0x34U, 0x56U, 0x78U, 0x9AU, 0xBCU, 0xDEU, 0xF0U,
        0x5FU, 0xF4U, 0x5AU,
        0x00U};

    // Format A stream with literal + back-reference producing "ABCABCA" (7 bytes).
    const std::array<std::uint8_t, 9> stream_format_a_match{
        0x08U, 0x00U, 0x03U, 'A', 'B', 'C', 0x80U, 0x03U, 0x00U};

    // Format B bit-stream producing 14 copies of 'Q'.
    const std::array<std::uint8_t, 10> stream_format_b{
        0x00U, 0x00U, 0x00U, 0x03U, 0x01U, 0x30U, 0x00U, 'Q', 0x00U, 0x00U};

    // Corrupt/truncated Format A stream (header claims span=0x20 past ROM end).
    const std::array<std::uint8_t, 4> stream_corrupt{0x20U, 0x00U, 0x03U, 'X'};

    constexpr std::uint32_t kAddr1 = 0x060000U;
    constexpr std::uint32_t kAddr2 = 0x060100U;
    constexpr std::uint32_t kAddr3 = 0x060200U;
    constexpr std::uint32_t kAddr4 = 0x00FFFFFFU; // Out-of-bounds stream pointer
    constexpr std::uint32_t kAddr5 = 0x060300U;
    constexpr std::size_t kRomSize = 0x060400U;

    std::vector<std::uint8_t> rom(kRomSize, 0x00U);

    // Table at 0x05CE96:
    write_be32(rom, kBgTilePatternTableAddr + 0U * 4U, 0x00000000U); // Index 0: null
    write_be32(rom, kBgTilePatternTableAddr + 1U * 4U, kAddr1);
    write_be32(rom, kBgTilePatternTableAddr + 2U * 4U, kAddr2);
    write_be32(rom, kBgTilePatternTableAddr + 3U * 4U, kAddr3);
    write_be32(rom, kBgTilePatternTableAddr + 4U * 4U, kAddr4);
    write_be32(rom, kBgTilePatternTableAddr + 5U * 4U, kAddr5);

    std::copy(stream_4k.begin(), stream_4k.end(), rom.begin() + kAddr1);
    std::copy(stream_format_a_match.begin(), stream_format_a_match.end(), rom.begin() + kAddr2);
    std::copy(stream_format_b.begin(), stream_format_b.end(), rom.begin() + kAddr3);
    std::copy(stream_corrupt.begin(), stream_corrupt.end(), rom.begin() + kAddr5);

    return rom;
}

void test_resolve_bg_tile_pattern_address() {
    std::vector<std::uint8_t> empty_rom;
    CHECK(resolve_bg_tile_pattern_address(empty_rom, 0U) == 0U);
    CHECK(resolve_bg_tile_pattern_address(empty_rom, 1U) == 0U);

    // ROM ending 1 byte short of full index-0 entry
    std::vector<std::uint8_t> short_rom(kBgTilePatternTableAddr + 3U, 0xAAU);
    CHECK(resolve_bg_tile_pattern_address(short_rom, 0U) == 0U);

    // Small ROM holding 2 table entries only (tests lookup without requiring 2MB payload)
    std::vector<std::uint8_t> table_only_rom(kBgTilePatternTableAddr + 8U, 0x00U);
    write_be32(table_only_rom, kBgTilePatternTableAddr + 0U, 0x00000000U);
    write_be32(table_only_rom, kBgTilePatternTableAddr + 4U, 0x001AD000U);
    CHECK(resolve_bg_tile_pattern_address(table_only_rom, 0U) == 0U);
    CHECK(resolve_bg_tile_pattern_address(table_only_rom, 1U) == 0x001AD000U);
    CHECK(resolve_bg_tile_pattern_address(table_only_rom, 2U) == 0U);
}

void test_decompress_bg_tile_pattern() {
    const auto rom = make_synthetic_rom_with_streams();
    std::array<std::uint8_t, kBgTileDmaByteSize> out{};

    // Index 0 (null) -> 0 bytes
    CHECK(decompress_bg_tile_pattern(rom, 0U, out) == 0U);

    // Index 1 -> 4096-byte Format A stream
    const std::size_t n1 = decompress_bg_tile_pattern(rom, 1U, out);
    CHECK(n1 == 4096U);
    const std::array<std::uint8_t, 8> expected_head{
        0x12U, 0x34U, 0x56U, 0x78U, 0x9AU, 0xBCU, 0xDEU, 0xF0U};
    for (std::size_t i = 0U; i < expected_head.size(); ++i) {
        CHECK(out[i] == expected_head[i]);
    }
    for (std::size_t i = expected_head.size(); i < n1; ++i) {
        CHECK(out[i] == 0x5AU);
    }

    // Index 2 -> 7-byte Format A literal + back-reference ("ABCABCA")
    out.fill(0U);
    const std::size_t n2 = decompress_bg_tile_pattern(rom, 2U, out);
    CHECK(n2 == 7U);
    const std::array<std::uint8_t, 7> expected_abc{'A', 'B', 'C', 'A', 'B', 'C', 'A'};
    for (std::size_t i = 0U; i < expected_abc.size(); ++i) {
        CHECK(out[i] == expected_abc[i]);
    }

    // Index 3 -> 14-byte Format B stream ("QQQQQQQQQQQQQQ")
    out.fill(0U);
    const std::size_t n3 = decompress_bg_tile_pattern(rom, 3U, out);
    CHECK(n3 == 14U);
    for (std::size_t i = 0U; i < n3; ++i) {
        CHECK(out[i] == 'Q');
    }

    // Invalid / OOB / corrupt / destination too small -> 0 bytes safely
    CHECK(decompress_bg_tile_pattern(rom, 4U, out) == 0U);
    CHECK(decompress_bg_tile_pattern(rom, 5U, out) == 0U);
    CHECK(decompress_bg_tile_pattern(rom, 999U, out) == 0U);
    CHECK(decompress_bg_tile_pattern(rom, 1U, std::span<std::uint8_t>{}) == 0U);
    std::array<std::uint8_t, 16> small_buf{};
    CHECK(decompress_bg_tile_pattern(rom, 1U, small_buf) == 0U);
}

void test_queue_bg_tile_pattern_dma() {
    std::vector<BgTileDmaCommand> queue;
    queue.push_back(BgTileDmaCommand{0x00112233U, 0x1000U, 0x0040U});

    std::uint8_t vint_flags = 0x81U; // Existing unrelated bits preserved
    queue_bg_tile_pattern_dma(queue, 0x4000U, vint_flags);

    CHECK(queue.size() == 2U);
    CHECK(queue[1].src_bus_addr == kBgTileDmaBusSource);
    CHECK(queue[1].vram_dest == 0x4000U);
    CHECK(queue[1].length_words == kBgTileDmaLengthWords);
    CHECK(vint_flags == 0x83U);
}

void test_decompress_and_dma_bg_tiles() {
    const auto rom = make_synthetic_rom_with_streams();
    std::array<std::uint8_t, kBgTileDmaByteSize> decomp_buf{};
    std::vector<BgTileDmaCommand> queue;
    std::uint8_t vint_flags = 0x40U;

    // Invalid index must not enqueue or modify sync flags or call V-Int callback
    bool callback_ran = false;
    const std::size_t fail_bytes = decompress_and_dma_bg_tiles(
        rom, 0U, 0x2000U, decomp_buf, queue, vint_flags,
        [&](std::uint8_t& flags) {
            callback_ran = true;
            flags = static_cast<std::uint8_t>(flags & ~kBgTileDmaSyncBit);
        });
    CHECK(fail_bytes == 0U);
    CHECK(queue.empty());
    CHECK(vint_flags == 0x40U);
    CHECK(!callback_ran);

    // Valid pattern without callback: bit 1 remains set
    const std::size_t bytes_no_cb = decompress_and_dma_bg_tiles(
        rom, 1U, 0x2000U, decomp_buf, queue, vint_flags);
    CHECK(bytes_no_cb == 4096U);
    CHECK(queue.size() == 1U);
    CHECK(queue[0] == (BgTileDmaCommand{kBgTileDmaBusSource, 0x2000U, kBgTileDmaLengthWords}));
    CHECK(vint_flags == 0x42U);

    // Valid pattern with V-Int completion callback: observes bit 1 set, clears bit 1
    vint_flags = 0x10U;
    std::uint8_t observed_flags_in_cb = 0U;
    const std::size_t bytes_with_cb = decompress_and_dma_bg_tiles(
        rom, 2U, 0x6000U, decomp_buf, queue, vint_flags,
        [&](std::uint8_t& flags) {
            observed_flags_in_cb = flags;
            flags = static_cast<std::uint8_t>(flags & ~kBgTileDmaSyncBit);
        });
    CHECK(bytes_with_cb == 7U);
    CHECK(observed_flags_in_cb == 0x12U);
    CHECK(vint_flags == 0x10U);
    CHECK(queue.size() == 2U);
    CHECK(queue[1] == (BgTileDmaCommand{kBgTileDmaBusSource, 0x6000U, kBgTileDmaLengthWords}));
}

void test_decompress_and_dma_bg_tiles_from_ram() {
    const auto rom = make_synthetic_rom_with_streams();
    std::array<std::uint8_t, 0x10000> ram{};

    // Guard bytes outside [$FF2FA8..$FF3FA7]
    ram[kBgTileDecompressBufferRamOffset - 1U] = 0xCCU;
    ram[kBgTileDecompressBufferRamOffset + kBgTileDmaByteSize] = 0xDDU;

    // Initialize $FF1892 -> $FF10B4 and $FF164E = 0x81
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FF10B4U);
    ram[kVintSyncFlagsRamOffset] = 0x81U;

    // 1. Call with clear_vint_sync = false
    const std::size_t n1 = decompress_and_dma_bg_tiles_from_ram(rom, ram, 1U, 0x4000U, false);
    CHECK(n1 == 4096U);
    CHECK(ram[kBgTileDecompressBufferRamOffset - 1U] == 0xCCU);
    CHECK(ram[kBgTileDecompressBufferRamOffset + kBgTileDmaByteSize] == 0xDDU);
    CHECK(ram[kBgTileDecompressBufferRamOffset + 0U] == 0x12U);
    CHECK(ram[kBgTileDecompressBufferRamOffset + 7U] == 0xF0U);
    CHECK(ram[kBgTileDecompressBufferRamOffset + 8U] == 0x5AU);
    CHECK(ram[kBgTileDecompressBufferRamOffset + 4095U] == 0x5AU);

    // Verify 8-byte BE DMA descriptor at 0x10B4 and updated $FF1892
    CHECK(read_be32(ram, 0x10B4U) == kBgTileDmaBusSource);
    CHECK(read_be16(ram, 0x10B8U) == 0x4000U);
    CHECK(read_be16(ram, 0x10BAU) == kBgTileDmaLengthWords);
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x00FF10BCU);
    CHECK(ram[kVintSyncFlagsRamOffset] == 0x83U); // Bit 1 set, unrelated bits preserved

    // 2. Call with default clear_vint_sync = true
    const std::size_t n2 = decompress_and_dma_bg_tiles_from_ram(rom, ram, 3U, 0x8000U);
    CHECK(n2 == 14U);
    CHECK(ram[kBgTileDecompressBufferRamOffset] == 'Q');
    CHECK(read_be32(ram, 0x10BCU) == kBgTileDmaBusSource);
    CHECK(read_be16(ram, 0x10C0U) == 0x8000U);
    CHECK(read_be16(ram, 0x10C2U) == kBgTileDmaLengthWords);
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x00FF10C4U);
    CHECK(ram[kVintSyncFlagsRamOffset] == 0x81U); // Bit 1 cleared, unrelated bits preserved

    // 3. Exact end-of-RAM queue boundary (0xFFF8 == 0x10000 - 8) succeeds
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FFFFF8U);
    CHECK(decompress_and_dma_bg_tiles_from_ram(rom, ram, 2U, 0xA000U) == 7U);
    CHECK(read_be32(ram, 0xFFF8U) == kBgTileDmaBusSource);
    CHECK(read_be16(ram, 0xFFFCU) == 0xA000U);
    CHECK(read_be16(ram, 0xFFFEU) == kBgTileDmaLengthWords);
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x01000000U);

    // 4. Out-of-bounds queue pointer (0xFFF9 > 0x10000 - 8) fails safely
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FFFFF9U);
    ram[kVintSyncFlagsRamOffset] = 0x00U;
    CHECK(decompress_and_dma_bg_tiles_from_ram(rom, ram, 1U, 0x2000U) == 0U);
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x00FFFFF9U);
    CHECK(ram[kVintSyncFlagsRamOffset] == 0x00U);

    // 5. Invalid pattern index fails safely without advancing $FF1892
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FF10B4U);
    CHECK(decompress_and_dma_bg_tiles_from_ram(rom, ram, 0U, 0x2000U) == 0U);
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x00FF10B4U);
}

void test_real_rom_if_available() {
    const std::array<std::filesystem::path, 3> candidates{
        "local-roms/Beyond Oasis (USA).md",
        "../local-roms/Beyond Oasis (USA).md",
        "../Sega-Thor/local-roms/Beyond Oasis (USA).md",
    };

    std::filesystem::path rom_path;
    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate, ec)) {
            rom_path = candidate;
            break;
        }
        ec.clear();
    }
    if (rom_path.empty()) {
        return; // Optional check: CI runs without commercial ROMs
    }

    const auto rom = oasis::Rom::load(rom_path);
    CHECK(resolve_bg_tile_pattern_address(rom.bytes(), 0U) == 0U);
    CHECK(resolve_bg_tile_pattern_address(rom.bytes(), 1U) == 0x001AD000U);
    CHECK(resolve_bg_tile_pattern_address(rom.bytes(), 2U) == 0x001AD9D4U);
    CHECK(resolve_bg_tile_pattern_address(rom.bytes(), 3U) == 0x001AE1A8U);
    CHECK(resolve_bg_tile_pattern_address(rom.bytes(), 4U) == 0x001AE8AAU);
    CHECK(resolve_bg_tile_pattern_address(rom.bytes(), 5U) == 0x001AF034U);

    std::array<std::uint8_t, kBgTileDmaByteSize> out{};
    const std::size_t id3_size = decompress_bg_tile_pattern(rom.bytes(), 3U, out);
    CHECK(id3_size == 4096U);
    CHECK(oasis::core::compute_sha256(out) == oasis::game::kResourceOutputSha256);

    std::array<std::uint8_t, 0x10000> ram{};
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FF10B4U);
    CHECK(decompress_and_dma_bg_tiles_from_ram(rom.bytes(), ram, 1U, 0x2000U) == 4096U);
    CHECK(read_be32(ram, 0x10B4U) == kBgTileDmaBusSource);
    CHECK(read_be16(ram, 0x10B8U) == 0x2000U);
    CHECK(read_be16(ram, 0x10BAU) == kBgTileDmaLengthWords);
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x00FF10BCU);
}

} // namespace

int main() {
    test_constants_and_struct_layout();
    test_resolve_bg_tile_pattern_address();
    test_decompress_bg_tile_pattern();
    test_queue_bg_tile_pattern_dma();
    test_decompress_and_dma_bg_tiles();
    test_decompress_and_dma_bg_tiles_from_ram();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.187 BG tile DMA tests succeeded!\n";
    return 0;
}
