#include "game/render/tilemap_rect_ops.hpp"
#include "game/render/dual_plane_loader.hpp"
#include "game/render/vdp_scroll_builder.hpp"

#include "core/rom.hpp"

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
        std::cerr << "[FAIL] M16.192 check failed: " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
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

void test_constants_and_structs() {
    static_assert(kTilemapFillRectByteAddr == 0x00DB1AU);
    static_assert(kTilemapFillRectByteEndAddr == 0x00DB64U);
    static_assert(kTilemapCopyRectWordAndSyncAddr == 0x00DB64U);
    static_assert(kTilemapCopyRectWordOnlyAddr == 0x00DB68U);
    static_assert(kTilemapCopyRectWordEndAddr == 0x00DBCAU);
    static_assert(kTilemapCopyRectByteAddr == 0x00DBCAU);
    static_assert(kTilemapCopyRectByteEndAddr == 0x00DC26U);
    static_assert(kVdpSyncDirtyPlaneRectAddr == 0x00DC26U);
    static_assert(kTilemapRectOpsEndAddr == 0x00DC26U);

    static_assert(TilemapRectOpsConfig::kPlaneAContextRamOffset == 0x1716U);
    static_assert(TilemapRectOpsConfig::kPlaneBContextRamOffset == 0x173EU);
    static_assert(TilemapRectOpsConfig::kPlaneAAuxRamOffset == 0x1766U);
    static_assert(TilemapRectOpsConfig::kPlaneBAuxRamOffset == 0x1770U);
    static_assert(TilemapRectOpsConfig::kScrollModeRamOffset == 0x16F0U);
    static_assert(TilemapRectOpsConfig::kScrollModeSelectPlaneBBit == 0x80U);

    constexpr TilemapRect single{5, 7, 5, 7};
    static_assert(single.is_valid());
    static_assert(single.width_minus_1() == 0);
    static_assert(single.height_minus_1() == 0);
    static_assert(single.width() == 1U);
    static_assert(single.height() == 1U);

    constexpr TilemapRect bad{5, 7, 4, 7};
    static_assert(!bad.is_valid());
    static_assert(bad.width() == 0U);
    static_assert(bad.height() == 0U);

    static_assert(oasis::game::render::kTilemapFillRectByteAddr == kTilemapFillRectByteAddr);
    static_assert(sizeof(oasis::game::render::TilemapRect) == sizeof(TilemapRect));
}

void test_parse_script_args() {
    const std::array<std::uint8_t, 8> src_bytes{0x00U, 0x02U, 0x00U, 0x03U, 0x00U, 0x06U, 0x00U, 0x05U};
    const std::array<std::uint8_t, 4> dst_bytes{0x00U, 0x0AU, 0xFFU, 0xFEU};
    TilemapRect rect{};
    TilemapPoint dst{};

    CHECK(!parse_tilemap_rect_copy_args(std::span<const std::uint8_t>(src_bytes).first(7U), dst_bytes, rect, dst));
    CHECK(!parse_tilemap_rect_copy_args(src_bytes, std::span<const std::uint8_t>(dst_bytes).first(3U), rect, dst));
    CHECK(parse_tilemap_rect_copy_args(src_bytes, dst_bytes, rect, dst));
    CHECK(rect == (TilemapRect{2, 3, 6, 5}));
    CHECK(dst == (TilemapPoint{10, -2}));
}

void test_fill_rect_byte() {
    // 64-tile wide x 32-tile tall aux byte map: word_stride_shift = 6, row_stride = 64
    std::vector<std::uint8_t> aux(64U * 32U, 0x11U);

    // 1x1 inclusive rect (dbra executes +1 = 1 cell)
    const std::size_t n1 = tilemap_fill_rect_byte(aux, 6U, 64U, TilemapRect{4, 2, 4, 2}, 0x7EU);
    CHECK(n1 == 1U);
    CHECK(aux[2U * 64U + 4U] == 0x7EU);
    CHECK(aux[2U * 64U + 3U] == 0x11U);
    CHECK(aux[2U * 64U + 5U] == 0x11U);

    // 3x2 inclusive rect (x=10..12, y=5..6)
    const std::size_t n2 = tilemap_fill_rect_byte(aux, 6U, 64U, TilemapRect{10, 5, 12, 6}, 0xA5U);
    CHECK(n2 == 6U);
    for (std::size_t y = 5U; y <= 6U; ++y) {
        CHECK(aux[y * 64U + 9U] == 0x11U);
        for (std::size_t x = 10U; x <= 12U; ++x) {
            CHECK(aux[y * 64U + x] == 0xA5U);
        }
        CHECK(aux[y * 64U + 13U] == 0x11U);
    }
    CHECK(aux[4U * 64U + 10U] == 0x11U);
    CHECK(aux[7U * 64U + 10U] == 0x11U);

    // Invalid / OOB rects return 0 without mutating buffer
    const auto snapshot = aux;
    CHECK(tilemap_fill_rect_byte(aux, 6U, 64U, TilemapRect{5, 5, 4, 5}, 0xFFU) == 0U);
    CHECK(tilemap_fill_rect_byte(aux, 6U, 64U, TilemapRect{-1, 0, 2, 2}, 0xFFU) == 0U);
    CHECK(tilemap_fill_rect_byte(aux, 16U, 64U, TilemapRect{0, 0, 1, 1}, 0xFFU) == 0U);
    CHECK(tilemap_fill_rect_byte(aux, 6U, 0U, TilemapRect{0, 0, 1, 1}, 0xFFU) == 0U);
    CHECK(tilemap_fill_rect_byte(aux, 6U, 64U, TilemapRect{60, 31, 65, 31}, 0xFFU) == 0U);
    CHECK(aux == snapshot);
}

void test_copy_rect_byte() {
    // 32-tile wide x 16-tile tall aux byte map: word_stride_shift = 5, row_stride = 32
    std::vector<std::uint8_t> aux(32U * 16U, 0x00U);
    for (std::size_t y = 0U; y < 4U; ++y) {
        for (std::size_t x = 0U; x < 5U; ++x) {
            aux[y * 32U + x] = static_cast<std::uint8_t>(y * 16U + x + 1U);
        }
    }

    // Copy 3x2 block (1..3, 1..2) -> (20, 8)
    const std::size_t n = tilemap_copy_rect_byte(aux, 5U, 32U, TilemapRect{1, 1, 3, 2}, TilemapPoint{20, 8});
    CHECK(n == 6U);
    for (std::size_t row = 0U; row < 2U; ++row) {
        for (std::size_t col = 0U; col < 3U; ++col) {
            CHECK(aux[(8U + row) * 32U + (20U + col)] == aux[(1U + row) * 32U + (1U + col)]);
        }
    }

    // Overlapping forward copy matches M68K `move.b (a0)+, (a1)+`
    std::vector<std::uint8_t> overlap(32U, 0U);
    overlap[0] = 0xAAU;
    overlap[1] = 0xBBU;
    overlap[2] = 0xCCU;
    CHECK(tilemap_copy_rect_byte(overlap, 5U, 32U, TilemapRect{0, 0, 2, 0}, TilemapPoint{1, 0}) == 3U);
    CHECK(overlap[0] == 0xAAU);
    CHECK(overlap[1] == 0xAAU);
    CHECK(overlap[2] == 0xAAU);
    CHECK(overlap[3] == 0xAAU);

    // Separate src/dst spans and OOB check
    std::vector<std::uint8_t> dst_buf(32U * 16U, 0x55U);
    CHECK(tilemap_copy_rect_byte(std::span<const std::uint8_t>(aux), dst_buf, 5U, 32U,
                                 TilemapRect{1, 1, 2, 2}, TilemapPoint{4, 3}) == 4U);
    CHECK(dst_buf[3U * 32U + 4U] == aux[1U * 32U + 1U]);
    CHECK(dst_buf[4U * 32U + 5U] == aux[2U * 32U + 2U]);
    CHECK(tilemap_copy_rect_byte(aux, 5U, 32U, TilemapRect{0, 0, 2, 2}, TilemapPoint{30, 15}) == 0U);
}

void test_copy_rect_word_and_sync() {
    // Cleanroom uint16_t helper
    std::vector<std::uint16_t> words(64U * 32U, 0U);
    words[2U * 64U + 3U] = 0x8123U;
    words[2U * 64U + 4U] = 0x8124U;
    words[3U * 64U + 3U] = 0x8133U;
    words[3U * 64U + 4U] = 0x8134U;
    CHECK(tilemap_copy_rect_word(words, 6U, 64U, TilemapRect{3, 2, 4, 3}, TilemapPoint{10, 12}) == 4U);
    CHECK(words[12U * 64U + 10U] == 0x8123U);
    CHECK(words[13U * 64U + 11U] == 0x8134U);

    // Composite BE16 byte-addressed tilemap + aux byte map + 0x00DC26 sync callback
    // Width = 64 tiles: word_stride_shift = 6, byte_stride_shift = 7, row_stride = 128 bytes
    std::vector<std::uint8_t> tilemap_bytes(64U * 32U * 2U, 0x00U);
    std::vector<std::uint8_t> aux_bytes(64U * 32U, 0x00U);
    for (std::size_t y = 2U; y <= 3U; ++y) {
        for (std::size_t x = 4U; x <= 6U; ++x) {
            const auto cell = static_cast<std::uint16_t>(0x9000U + y * 0x10U + x);
            write_be16(tilemap_bytes, (y << 7U) + x * 2U, cell);
            aux_bytes[(y << 6U) + x] = static_cast<std::uint8_t>(0x40U + y * 8U + x);
        }
    }

    bool sync_called = false;
    TilemapRectSyncRequest captured{};
    std::uint16_t captured_top_left_word = 0U;
    const std::size_t copied = tilemap_copy_rect_word_and_sync(
        tilemap_bytes, aux_bytes, 7U, 64U, 6U,
        TilemapRect{4, 2, 6, 3}, TilemapPoint{14, 9}, true,
        [&](const TilemapRectSyncRequest& req) {
            sync_called = true;
            captured = req;
            captured_top_left_word = read_be16(req.dst_tilemap_tail, 0U);
        });

    CHECK(copied == 6U);
    CHECK(sync_called);
    CHECK(captured.is_plane_b);
    CHECK(captured.plane_ram_offset == kPlaneBRamOffset);
    CHECK(captured.dst_x == 14);
    CHECK(captured.dst_y == 9);
    CHECK(captured.width_minus_1 == 2);
    CHECK(captured.height_minus_1 == 1);
    CHECK(captured.width() == 3U);
    CHECK(captured.height() == 2U);
    CHECK(captured.dst_byte_offset == (9U << 7U) + 28U);
    CHECK(captured_top_left_word == 0x9024U);
    CHECK(read_be16(tilemap_bytes, (10U << 7U) + 16U * 2U) == 0x9036U);
    CHECK(aux_bytes[(10U << 6U) + 16U] == 0x5EU);

    // Undersized/OOB aux map aborts composite copy before mutating tilemap or calling sync
    std::vector<std::uint8_t> bad_aux(16U, 0x00U);
    bool should_not_run = false;
    CHECK(tilemap_copy_rect_word_and_sync(
              tilemap_bytes, bad_aux, 7U, 64U, 6U,
              TilemapRect{4, 2, 6, 3}, TilemapPoint{20, 12}, false,
              [&](const TilemapRectSyncRequest&) { should_not_run = true; }) == 0U);
    CHECK(!should_not_run);
    CHECK(read_be16(tilemap_bytes, (12U << 7U) + 20U * 2U) == 0x0000U);
}

void test_64kb_ram_window_helpers() {
    // Layout in 64KB Work RAM ($FF0000..$FFFFFF):
    // Plane A: 32x32 tiles (shift: byte=6, word=5) at $FF4000 (2048B) + aux at $FF4800 (1024B)
    // Plane B: 64x32 tiles (shift: byte=7, word=6) at $FF5000 (4096B) + aux at $FF6000 (2048B)
    std::array<std::uint8_t, 0x10000> ram{};

    // Plane A context ($FF1716) & aux ($FF1766)
    write_be32(ram, kPlaneARamOffset + 0x16U, 0x00FF4000U);
    write_be16(ram, kPlaneARamOffset + 0x20U, 32U);
    write_be16(ram, kPlaneARamOffset + 0x24U, 6U);
    write_be32(ram, TilemapRectOpsConfig::kPlaneAAuxRamOffset + 0x00U, 0x00FF4800U);
    write_be16(ram, TilemapRectOpsConfig::kPlaneAAuxRamOffset + 0x04U, 32U);
    write_be16(ram, TilemapRectOpsConfig::kPlaneAAuxRamOffset + 0x08U, 5U);

    // Plane B context ($FF173E) & aux ($FF1770)
    write_be32(ram, kPlaneBRamOffset + 0x16U, 0x00FF5000U);
    write_be16(ram, kPlaneBRamOffset + 0x20U, 64U);
    write_be16(ram, kPlaneBRamOffset + 0x24U, 7U);
    write_be32(ram, TilemapRectOpsConfig::kPlaneBAuxRamOffset + 0x00U, 0x00FF6000U);
    write_be16(ram, TilemapRectOpsConfig::kPlaneBAuxRamOffset + 0x04U, 64U);
    write_be16(ram, TilemapRectOpsConfig::kPlaneBAuxRamOffset + 0x08U, 6U);

    // 1. tilemap_fill_rect_byte_from_ram: $FF16F0 bit 7 = 0 -> Plane A aux ($FF4800)
    ram[TilemapRectOpsConfig::kScrollModeRamOffset] = 0x00U;
    CHECK(tilemap_fill_rect_byte_from_ram(ram, TilemapRect{2, 3, 4, 4}, 0x3CU) == 6U);
    CHECK(ram[0x4800U + 3U * 32U + 2U] == 0x3CU);
    CHECK(ram[0x4800U + 4U * 32U + 4U] == 0x3CU);
    CHECK(ram[0x6000U + 3U * 64U + 2U] == 0x00U);

    // $FF16F0 bit 7 = 1 -> Plane B aux ($FF6000)
    ram[TilemapRectOpsConfig::kScrollModeRamOffset] = 0x80U;
    CHECK(tilemap_fill_rect_byte_from_ram(ram, TilemapRect{5, 1, 6, 2}, 0x5DU) == 4U);
    CHECK(ram[0x6000U + 1U * 64U + 5U] == 0x5DU);
    CHECK(ram[0x6000U + 2U * 64U + 6U] == 0x5DU);

    // 2. tilemap_copy_rect_word_and_sync_from_ram on Plane A
    write_be16(ram, 0x4000U + (3U << 6U) + 2U * 2U, 0xB001U);
    write_be16(ram, 0x4000U + (3U << 6U) + 3U * 2U, 0xB002U);
    write_be16(ram, 0x4000U + (4U << 6U) + 2U * 2U, 0xB011U);
    write_be16(ram, 0x4000U + (4U << 6U) + 3U * 2U, 0xB012U);

    std::size_t sync_calls = 0U;
    TilemapRectSyncRequest last_req{};
    CHECK(tilemap_copy_rect_word_and_sync_from_ram(
              ram, false, TilemapRect{2, 3, 3, 4}, TilemapPoint{10, 8},
              [&](const TilemapRectSyncRequest& req) {
                  ++sync_calls;
                  last_req = req;
                  CHECK(read_be16(req.dst_tilemap_tail, 0U) == 0xB001U);
              }) == 4U);
    CHECK(sync_calls == 1U);
    CHECK(!last_req.is_plane_b);
    CHECK(last_req.plane_ram_offset == kPlaneARamOffset);
    CHECK(last_req.dst_x == 10 && last_req.dst_y == 8);
    CHECK(read_be16(ram, 0x4000U + (8U << 6U) + 10U * 2U) == 0xB001U);
    CHECK(read_be16(ram, 0x4000U + (9U << 6U) + 11U * 2U) == 0xB012U);
    CHECK(ram[0x4800U + 8U * 32U + 10U] == 0x3CU);
    CHECK(ram[0x4800U + 9U * 32U + 11U] == 0x3CU);

    // 3. tilemap_copy_rect_word_only_and_sync_from_ram (0x00DB68) on Plane B skips aux copy
    write_be16(ram, 0x5000U + (1U << 7U) + 5U * 2U, 0xC123U);
    CHECK(tilemap_copy_rect_word_only_and_sync_from_ram(
              ram, true, TilemapRect{5, 1, 5, 1}, TilemapPoint{20, 4},
              [&](const TilemapRectSyncRequest& req) {
                  ++sync_calls;
                  CHECK(req.is_plane_b);
                  CHECK(req.plane_ram_offset == kPlaneBRamOffset);
              }) == 1U);
    CHECK(sync_calls == 2U);
    CHECK(read_be16(ram, 0x5000U + (4U << 7U) + 20U * 2U) == 0xC123U);
    CHECK(ram[0x6000U + 4U * 64U + 20U] == 0x00U); // Aux untouched

    // 4. tilemap_copy_rect_byte_from_ram (0x00DBCA) standalone on Plane B
    CHECK(tilemap_copy_rect_byte_from_ram(ram, true, TilemapRect{5, 1, 6, 2}, TilemapPoint{20, 4}) == 4U);
    CHECK(ram[0x6000U + 4U * 64U + 20U] == 0x5DU);
    CHECK(ram[0x6000U + 5U * 64U + 21U] == 0x5DU);

    // 5. Invalid non-$FFxxxx / null RAM pointers fail safely
    write_be32(ram, TilemapRectOpsConfig::kPlaneAAuxRamOffset + 0x00U, 0x00123456U);
    CHECK(tilemap_copy_rect_byte_from_ram(ram, false, TilemapRect{0, 0, 1, 1}, TilemapPoint{2, 2}) == 0U);
    write_be32(ram, kPlaneARamOffset + 0x16U, 0x00000000U);
    CHECK(tilemap_copy_rect_word_only_and_sync_from_ram(ram, false, TilemapRect{0, 0, 1, 1}, TilemapPoint{2, 2}) == 0U);
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
        return; // CI runs without commercial ROMs
    }

    const auto rom = oasis::Rom::load(rom_path);
    std::array<std::uint8_t, 0x10000> ram{};

    // Spirit Sanctuary (0x259F78): Plane B has 64x32 tilemap + aux byte map
    const auto init_res = init_dual_plane_layout_from_ram(rom.bytes(), ram, 0x259F78U, 0x0101U, 0U);
    CHECK(init_res.ok);

    ram[TilemapRectOpsConfig::kScrollModeRamOffset] = 0x80U; // Select Plane B for fill
    const std::uint8_t prev_aux_cell = ram[(init_res.plane_b_aux.next_ptr & 0xFFFFU) + 2U * 64U + 3U];
    CHECK(tilemap_fill_rect_byte_from_ram(ram, TilemapRect{10, 4, 11, 5}, 0x77U) == 4U);
    CHECK(ram[(init_res.plane_b_aux.next_ptr & 0xFFFFU) + 4U * 64U + 10U] == 0x77U);
    CHECK(ram[(init_res.plane_b_aux.next_ptr & 0xFFFFU) + 5U * 64U + 11U] == 0x77U);
    CHECK(ram[(init_res.plane_b_aux.next_ptr & 0xFFFFU) + 2U * 64U + 3U] == prev_aux_cell);

    bool synced = false;
    const std::uint16_t src_word = read_be16(ram, (init_res.plane_b.tile_data_ptr & 0xFFFFU) + (4U << 7U) + 10U * 2U);
    CHECK(tilemap_copy_rect_word_and_sync_from_ram(
              ram, true, TilemapRect{10, 4, 11, 5}, TilemapPoint{2, 2},
              [&](const TilemapRectSyncRequest& req) {
                  synced = true;
                  CHECK(req.is_plane_b);
                  CHECK(req.width() == 2U && req.height() == 2U);
                  CHECK(read_be16(req.dst_tilemap_tail, 0U) == src_word);
              }) == 4U);
    CHECK(synced);
    CHECK(ram[(init_res.plane_b_aux.next_ptr & 0xFFFFU) + 2U * 64U + 2U] == 0x77U);
}

} // namespace

int main() {
    test_constants_and_structs();
    test_parse_script_args();
    test_fill_rect_byte();
    test_copy_rect_byte();
    test_copy_rect_word_and_sync();
    test_64kb_ram_window_helpers();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.192 tilemap rect ops tests succeeded!\n";
    return 0;
}
