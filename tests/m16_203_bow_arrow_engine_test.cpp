#include "game/combat/bow_arrow_engine.hpp"
#include "../include/game/combat/bow_arrow_engine.hpp"
#include "game/combat/elemental_blade.hpp"
#include "game/combat/hit_spark_generator.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

using namespace thor::combat;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.203 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

[[nodiscard]] std::int32_t read_be32_signed(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return static_cast<std::int32_t>(read_be32(data, offset));
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32_signed(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::int32_t value) {
    const auto u = static_cast<std::uint32_t>(value);
    data[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
}

void test_constants_and_coexistence() {
    static_assert(kBowArrowStartAddr == 0x00F396U);
    static_assert(kBowArrowGroundEntryAddr == 0x00F396U);
    static_assert(kBowArrowGroundEntrySizeBytes == 10U);
    static_assert(kBowArrowGroundOffsetsAddr == 0x00F3A0U);
    static_assert(kBowArrowGroundOffsetsSizeBytes == 8U);

    static_assert(kBowArrowWaterEntryAddr == 0x00F3A8U);
    static_assert(kBowArrowWaterEntrySizeBytes == 10U);
    static_assert(kBowArrowWaterOffsetsAddr == 0x00F3B2U);
    static_assert(kBowArrowWaterOffsetsSizeBytes == 8U);

    static_assert(kBowArrowSharedInitAddr == 0x00F3BAU);
    static_assert(kBowArrowSharedInitSizeBytes == 188U);
    static_assert(kBowArrowEndAddr == 0x00F476U);
    static_assert(kBowArrowSizeBytes == 224U);
    static_assert(kBowArrowGroundEntrySizeBytes +
                  kBowArrowGroundOffsetsSizeBytes +
                  kBowArrowWaterEntrySizeBytes +
                  kBowArrowWaterOffsetsSizeBytes +
                  kBowArrowSharedInitSizeBytes == kBowArrowSizeBytes);

    static_assert(kBowArrowSlotRamAddr == 0x00FF1B60U);
    static_assert(kBowArrowTypeId == 0x003AU);
    static_assert(kBowArrowDamageFlags == 0x8005U);
    static_assert(kBowArrowGroundCallerAddr == 0x0073C0U);
    static_assert(kBowArrowWaterCallerAddr == 0x00741AU);

    // Coexistence & alias verification inside oasis::game::combat.
    static_assert(oasis::game::combat::kBowArrowStartAddr == kBowArrowStartAddr);
    static_assert(oasis::game::combat::kElementalBladeStartAddr == 0x00F258U);
    static_assert(oasis::game::combat::HitSparkGeneratorConfig::kHitSparkGeneratorAddr == 0x00C012U);
    static_assert(sizeof(oasis::game::combat::BowArrowSlotState) == sizeof(BowArrowSlotState));
    CHECK(compute_bow_arrow_caller_param_d2(BowArrowSurface::Ground, 2U) == 33U);
    CHECK(compute_bow_arrow_caller_param_d2(BowArrowSurface::Water, 2U) == 16U);
}

void test_tip_offset_tables() {
    for (std::size_t dir = 0U; dir < 4U; ++dir) {
        CHECK(unpack_bow_arrow_tip_word(kBowArrowGroundTipWords[dir]) == kBowArrowGroundTipOffsets[dir]);
        CHECK(unpack_bow_arrow_tip_word(kBowArrowWaterTipWords[dir]) == kBowArrowWaterTipOffsets[dir]);
    }

    static_assert(unpack_bow_arrow_tip_word(0x06C8U) == BowArrowTipOffset{6, -56});
    static_assert(unpack_bow_arrow_tip_word(0x20DEU) == BowArrowTipOffset{32, -34});
    static_assert(unpack_bow_arrow_tip_word(0xF6F8U) == BowArrowTipOffset{-10, -8});
    static_assert(unpack_bow_arrow_tip_word(0xE0DEU) == BowArrowTipOffset{-32, -34});

    static_assert(unpack_bow_arrow_tip_word(0x0EEEU) == BowArrowTipOffset{14, -18});
    static_assert(unpack_bow_arrow_tip_word(0x1204U) == BowArrowTipOffset{18, 4});
    static_assert(unpack_bow_arrow_tip_word(0xFA14U) == BowArrowTipOffset{-6, 20});
    static_assert(unpack_bow_arrow_tip_word(0xEE04U) == BowArrowTipOffset{-18, 4});

    CHECK((get_bow_arrow_tip_offset(BowArrowSurface::Ground, 0U) == BowArrowTipOffset{6, -56}));
    CHECK((get_bow_arrow_tip_offset(BowArrowSurface::Ground, 5U) == BowArrowTipOffset{32, -34}));
    CHECK((get_bow_arrow_tip_offset(BowArrowSurface::Water, 2U) == BowArrowTipOffset{-6, 20}));
    CHECK((get_bow_arrow_tip_offset(BowArrowSurface::Water, 7U) == BowArrowTipOffset{-18, 4}));
}

void test_parabolic_arc_unsigned_threshold() {
    // Low/fast arc for unsigned elevation 0..15 (< 0x10)
    const auto elev_0 = calculate_bow_arrow_launch_params(0U, 0);
    CHECK(!elev_0.high_or_wrapped_arc && elev_0.speed_x == 0x0400U && elev_0.vel_z_fp == 0x00028000);

    const auto elev_15 = calculate_bow_arrow_launch_params(1U, 15);
    CHECK(!elev_15.high_or_wrapped_arc && elev_15.speed_x == 0x0400U && elev_15.vel_z_fp == 0x00028000);

    // Boundary 16 (bcc triggers -> high/standard arc: speed 0x0300, vel_z 0x00020000)
    const auto elev_16 = calculate_bow_arrow_launch_params(1U, 16);
    CHECK(elev_16.high_or_wrapped_arc && elev_16.speed_x == 0x0300U && elev_16.vel_z_fp == 0x00020000);

    const auto elev_34 = calculate_bow_arrow_launch_params(2U, kBowArrowGroundCallerElevDefault);
    CHECK(elev_34.high_or_wrapped_arc && elev_34.speed_x == 0x0300U && elev_34.vel_z_fp == 0x00020000);

    // Negative s16 (-4 = 0xFFFC >= 0x0010 unsigned) MUST take high_or_wrapped_arc!
    const auto elev_neg = calculate_bow_arrow_launch_params(0U, -4);
    CHECK(elev_neg.high_or_wrapped_arc && elev_neg.speed_x == 0x0300U && elev_neg.vel_z_fp == 0x00020000);
}

void test_anchor_facing_and_spawn() {
    // Ground facing Up (+6, -56) + elev 34 -> dy = -22; 16.16 normal & slowmo-doubled
    const auto a_norm = compute_bow_arrow_anchor(BowArrowSurface::Ground, 0U, 34, false);
    CHECK(a_norm.anchor_dx_fp == 0x00060000 && a_norm.anchor_dy_fp == -0x00160000);

    const auto a_slow = compute_bow_arrow_anchor(BowArrowSurface::Ground, 0U, 34, true);
    CHECK(a_slow.anchor_dx_fp == 0x000C0000 && a_slow.anchor_dy_fp == -0x002C0000);

    // Ground caller replay (Ali facing SE = 5? Wait table index 5 & 3 = 1 (Right), velocity dir 5 (SE) -> 0.75x)
    const BowArrowOwnerState ali_se{
        .owner_ram_addr = 0x00FF19E8U,
        .pos_x_fp = 0x01204000,
        .pos_y_fp = 0x00C08000,
        .facing = 5U,
        .sprite_priority = 0x0610U,
    };
    const auto fire_ground = spawn_bow_arrow(
        BowArrowSurface::Ground, ali_se, static_cast<std::uint16_t>(BowArrowType::Fire),
        kBowArrowGroundCallerElevDefault,
        compute_bow_arrow_caller_param_d2(BowArrowSurface::Ground, 1U), false);
    CHECK(fire_ground.type_id == 0x003AU);
    CHECK(fire_ground.param_or_lifetime_02 == 17U);
    CHECK(fire_ground.state_04 == 0U);
    CHECK(fire_ground.pos_x_fp == 0x01204000 && fire_ground.pos_y_fp == 0x00C08000);
    CHECK(fire_ground.pos_z_fp == 0x00220000);
    CHECK(fire_ground.facing_16 == 0U); // clr.w $16(a6) on projectile!
    CHECK(fire_ground.sprite_priority_18 == 0x0610U);
    CHECK(fire_ground.anim_id_2a == 0U);
    CHECK(fire_ground.vel_x_fp == 0x00024000); // +0x030000 * 0.75
    CHECK(fire_ground.vel_y_fp == 0x00024000);
    CHECK(fire_ground.vel_z_fp == 0x00020000);
    CHECK(fire_ground.anchor_dx_fp == 0x00200000); // Right (+32)
    CHECK(fire_ground.anchor_dy_fp == 0x00000000); // -34 + 34 = 0
    CHECK(fire_ground.subtype_a6 == 1U);
    CHECK(fire_ground.damage_a8 == 0x8005U);
    CHECK(fire_ground.owner_ptr_aa == 0x00FF19E8U);

    // Water caller (elev=4 -> low/fast arc speed 0x0400 -> 0x00030000 diagonal SE, vel_z = 0x00028000)
    const auto bomb_water = spawn_bow_arrow(
        BowArrowSurface::Water, ali_se, static_cast<std::uint16_t>(BowArrowType::Bomb),
        kBowArrowWaterCallerElevDefault,
        compute_bow_arrow_caller_param_d2(BowArrowSurface::Water, 2U), true);
    CHECK(bomb_water.param_or_lifetime_02 == 16U);
    CHECK(bomb_water.vel_x_fp == 0x00030000 && bomb_water.vel_y_fp == 0x00030000);
    CHECK(bomb_water.vel_z_fp == 0x00028000);
    CHECK(bomb_water.anchor_dx_fp == 0x00240000); // (+18 * 2) << 16
    CHECK(bomb_water.anchor_dy_fp == 0x00100000); // ((+4 + 4) * 2) << 16
}

void test_ram_window_and_guards() {
    std::array<std::uint8_t, 0x10000> ram{};
    ram.fill(0x5AU);

    constexpr std::size_t kAliOff = 0x19E8U;
    write_be32_signed(ram, kAliOff + 0x08U, 0x00800000);
    write_be32_signed(ram, kAliOff + 0x0CU, 0x00900000);
    write_be16(ram, kAliOff + 0x16U, 3U);    // West (facing 3)
    write_be16(ram, kAliOff + 0x5AU, 0x0720U);
    ram[kBowArrowPauseSlowmoRamOffset] = 0x00U;

    const auto ground_arrow = spawn_bow_arrow_ground_from_ram(
        ram, 0x00FF19E8U, 0U, kBowArrowGroundCallerElevDefault, 1U);
    CHECK(load_bow_arrow_slot_from_ram(ram) == ground_arrow);
    CHECK(read_be16(ram, kBowArrowSlotRamOffset + 0x00U) == 0x003AU);
    CHECK(read_be16(ram, kBowArrowSlotRamOffset + 0x02U) == 1U);
    CHECK(read_be32_signed(ram, kBowArrowSlotRamOffset + 0x10U) == 0x00220000);
    CHECK(read_be16(ram, kBowArrowSlotRamOffset + 0x16U) == 0U);
    CHECK(read_be16(ram, kBowArrowSlotRamOffset + 0x18U) == 0x0720U);
    CHECK(read_be32_signed(ram, kBowArrowSlotRamOffset + 0x4EU) == -0x00030000);
    CHECK(read_be32_signed(ram, kBowArrowSlotRamOffset + 0x52U) == 0);
    CHECK(read_be32_signed(ram, kBowArrowSlotRamOffset + 0x56U) == 0x00020000);
    CHECK(read_be32_signed(ram, kBowArrowSlotRamOffset + 0x72U) == -0x00200000); // -32
    CHECK(read_be32_signed(ram, kBowArrowSlotRamOffset + 0x76U) == 0);           // -34 + 34 = 0
    CHECK(read_be16(ram, kBowArrowSlotRamOffset + 0xA6U) == 0U);
    CHECK(read_be16(ram, kBowArrowSlotRamOffset + 0xA8U) == 0x8005U);
    CHECK(read_be32(ram, kBowArrowSlotRamOffset + 0xAAU) == 0x00FF19E8U);

    // Adjacent RAM untouched; cleared unused bytes inside $FF1B60 zeroed
    CHECK(ram[kBowArrowSlotRamOffset - 1U] == 0x5AU);
    CHECK(ram[kBowArrowSlotRamOffset + kBowArrowSlotStrideBytes] == 0x5AU);
    CHECK(ram[kBowArrowSlotRamOffset + 0x20U] == 0x00U);

    // Water arrow with slowmo bit 0 set
    ram[kBowArrowPauseSlowmoRamOffset] = 0x01U;
    const auto water_arrow = spawn_bow_arrow_water_from_ram(
        ram, 0x19E8U, 1U, kBowArrowWaterCallerElevDefault, 8U);
    CHECK(water_arrow.vel_x_fp == -0x00040000);
    CHECK(water_arrow.vel_z_fp == 0x00028000);
    CHECK(water_arrow.anchor_dx_fp == -0x00240000); // (-18 * 2) << 16
    CHECK(water_arrow.anchor_dy_fp == 0x00100000);  // ((+4 + 4) * 2) << 16

    // OOB / short buffer guards
    std::array<std::uint8_t, 64> short_ram{};
    CHECK(load_bow_arrow_slot_from_ram(short_ram) == BowArrowSlotState{});
    store_bow_arrow_slot_to_ram(short_ram, water_arrow);
    CHECK(spawn_bow_arrow_ground_from_ram(short_ram, 0x19E8U, 0U, 0, 1U) == BowArrowSlotState{});
    CHECK(spawn_bow_arrow_water_from_ram(ram, 0x00FE0000U, 0U, 0, 1U) == BowArrowSlotState{});
}

void test_optional_local_rom_tables() {
    const std::array<std::filesystem::path, 4> candidates{
        "local-roms/Beyond Oasis (USA).md",
        "../local-roms/Beyond Oasis (USA).md",
        "../Sega-Thor/local-roms/Beyond Oasis (USA).md",
        "../../Sega-Thor/local-roms/Beyond Oasis (USA).md",
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

    std::ifstream in(rom_path, std::ios::binary);
    CHECK(in.good());
    std::vector<std::uint8_t> slice(kBowArrowSizeBytes);
    in.seekg(static_cast<std::streamoff>(kBowArrowStartAddr), std::ios::beg);
    in.read(reinterpret_cast<char*>(slice.data()), static_cast<std::streamsize>(slice.size()));
    CHECK(in.gcount() == static_cast<std::streamsize>(slice.size()));

    for (std::size_t i = 0U; i < 4U; ++i) {
        const std::size_t g_off = (kBowArrowGroundOffsetsAddr - kBowArrowStartAddr) + i * 2U;
        const std::size_t w_off = (kBowArrowWaterOffsetsAddr - kBowArrowStartAddr) + i * 2U;
        CHECK(read_be16(slice, g_off) == kBowArrowGroundTipWords[i]);
        CHECK(read_be16(slice, w_off) == kBowArrowWaterTipWords[i]);
    }
    CHECK(read_be16(slice, 0x00F3C4U - kBowArrowStartAddr) == 0x003AU); // move.w #$3a, $0(a6)
    CHECK(read_be16(slice, 0x00F428U - kBowArrowStartAddr) == 0x8005U); // move.w #$8005, $a8(a6)
    CHECK(read_be32(slice, 0x00F456U - kBowArrowStartAddr) == 0x00028000U);
    CHECK(read_be16(slice, 0x00F474U - kBowArrowStartAddr) == 0x4E75U);
}

} // namespace

int main() {
    test_constants_and_coexistence();
    test_tip_offset_tables();
    test_parabolic_arc_unsigned_threshold();
    test_anchor_facing_and_spawn();
    test_ram_window_and_guards();
    test_optional_local_rom_tables();

    std::cout << "[PASS] All M16.203 bow arrow engine tests succeeded!\n";
    return 0;
}
