#include "game/combat/elemental_blade.hpp"
#include "../include/game/combat/elemental_blade.hpp"
#include "game/combat/elemental_damage.hpp"
#include "game/combat/projectile_hit_knockback.hpp"

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
        std::cerr << "[FAIL] M16.202 check failed at line " << line << ": " << expr << '\n';
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
    static_assert(kElementalBladeStartAddr == 0x00F258U);
    static_assert(kElementalBladeGroundEntryAddr == 0x00F258U);
    static_assert(kElementalBladeGroundEntrySizeBytes == 10U);
    static_assert(kElementalBladeGroundOffsetsAddr == 0x00F262U);
    static_assert(kElementalBladeGroundOffsetsSizeBytes == 8U);

    static_assert(kElementalBladeWaterEntryAddr == 0x00F26AU);
    static_assert(kElementalBladeWaterEntrySizeBytes == 10U);
    static_assert(kElementalBladeWaterOffsetsAddr == 0x00F274U);
    static_assert(kElementalBladeWaterOffsetsSizeBytes == 8U);

    static_assert(kElementalBladeSharedInitAddr == 0x00F27CU);
    static_assert(kElementalBladeSharedInitSizeBytes == 282U);
    static_assert(kElementalBladeEndAddr == 0x00F396U);
    static_assert(kElementalBladeSizeBytes == 318U);
    static_assert(kElementalBladeGroundEntrySizeBytes +
                  kElementalBladeGroundOffsetsSizeBytes +
                  kElementalBladeWaterEntrySizeBytes +
                  kElementalBladeWaterOffsetsSizeBytes +
                  kElementalBladeSharedInitSizeBytes == kElementalBladeSizeBytes);

    static_assert(kSwordWaveSlotRamAddr == 0x00FF1B60U);
    static_assert(kSwordWaveSlotRamOffset == 0x1B60U);
    static_assert(kSwordWaveSlotStrideBytes == 0xBCU);
    static_assert(kSwordWavePauseSlowmoRamAddr == 0x00FF17B8U);
    static_assert(kSwordWavePauseSlowmoRamOffset == 0x17B8U);
    static_assert(kSwordWaveTypeId == 0x0034U);
    static_assert(kSwordWaveSpritePriority == 0x055EU);
    static_assert(kSwordWaveFloatVelZFp == 0x0000C000);
    static_assert(kSwordWaveLifetimeFrames == 240U);

    // Coexistence & alias verification inside oasis::game::combat.
    static_assert(oasis::game::combat::kElementalBladeStartAddr == kElementalBladeStartAddr);
    static_assert(oasis::game::combat::kProjectileHitKnockbackAddr == 0x00EB40U);
    static_assert(sizeof(oasis::game::combat::UniversalDamageResult) > 0U);
    static_assert(sizeof(oasis::game::combat::SwordWaveSlotState) == sizeof(SwordWaveSlotState));

    CHECK(kElementalBladeGroundCallerAddr == 0x0072C4U);
    CHECK(kElementalBladeWaterCallerAddr == 0x007348U);
}

void test_tip_offset_tables() {
    for (std::size_t dir = 0U; dir < 4U; ++dir) {
        CHECK(unpack_sword_tip_word(kGroundSwordTipWords[dir]) == kGroundSwordTipOffsets[dir]);
        CHECK(unpack_sword_tip_word(kWaterSwordTipWords[dir]) == kWaterSwordTipOffsets[dir]);
    }

    static_assert(unpack_sword_tip_word(0x01D8U) == SwordTipOffset{1, -40});
    static_assert(unpack_sword_tip_word(0x10E6U) == SwordTipOffset{16, -26});
    static_assert(unpack_sword_tip_word(0xFD00U) == SwordTipOffset{-3, 0});
    static_assert(unpack_sword_tip_word(0xF0E6U) == SwordTipOffset{-16, -26});

    static_assert(unpack_sword_tip_word(0x03E0U) == SwordTipOffset{3, -32});
    static_assert(unpack_sword_tip_word(0x14EFU) == SwordTipOffset{20, -17});
    static_assert(unpack_sword_tip_word(0xFD04U) == SwordTipOffset{-3, 4});
    static_assert(unpack_sword_tip_word(0xECEFU) == SwordTipOffset{-20, -17});

    CHECK((get_sword_tip_offset(SwordWaveSurface::Ground, 0U) == SwordTipOffset{1, -40}));
    CHECK((get_sword_tip_offset(SwordWaveSurface::Ground, 5U) == SwordTipOffset{16, -26}));
    CHECK((get_sword_tip_offset(SwordWaveSurface::Water, 2U) == SwordTipOffset{-3, 4}));
    CHECK((get_sword_tip_offset(SwordWaveSurface::Water, 7U) == SwordTipOffset{-20, -17}));
}

void test_sword_parameter_decoding() {
    // 1. Standard / un-typed wave (0x0000001C)
    const auto std_cfg = decode_sword_wave_parameters(0x0000001CU);
    CHECK(std_cfg.type == SwordWaveType::Standard);
    CHECK(std_cfg.elevation == 28);
    CHECK(std_cfg.damage == 0x0003U);
    CHECK(std_cfg.anim_id == 2U);
    CHECK(!std_cfg.piercing);
    CHECK(!std_cfg.flag_3c_bit1);
    CHECK(std_cfg.speed_x == 0x0C00U);
    CHECK(std_cfg.speed_y == 0x0BC0U);

    // 2. Fire Sword (bit 31 = 0x80000000)
    const auto fire_cfg = get_sword_wave_config(SwordWaveType::Fire, kSwordWaveGroundElevDefault);
    CHECK(fire_cfg.type == SwordWaveType::Fire);
    CHECK(fire_cfg.elevation == 28);
    CHECK(fire_cfg.damage == 0x2004U);
    CHECK(fire_cfg.anim_id == 4U);
    CHECK(!fire_cfg.piercing);
    CHECK(fire_cfg.flag_3c_bit1);
    CHECK(fire_cfg.speed_x == 0x0C00U);
    CHECK(fire_cfg.speed_y == 0x0BC0U);

    // 3. Omega Sword (bit 30 = 0x40000000)
    const auto omega_cfg = get_sword_wave_config(SwordWaveType::Omega, kSwordWaveWaterElevDefault);
    CHECK(omega_cfg.type == SwordWaveType::Omega);
    CHECK(omega_cfg.elevation == 13);
    CHECK(omega_cfg.damage == 0x8005U);
    CHECK(omega_cfg.anim_id == 2U);
    CHECK(omega_cfg.piercing);
    CHECK(!omega_cfg.flag_3c_bit1);
    CHECK(omega_cfg.speed_x == 0x1000U);
    CHECK(omega_cfg.speed_y == 0x0F00U);

    // 4. Hyper Sword (bit 29 = 0x20000000) + negative elevation (-5 = 0xFFFB)
    const auto hyper_cfg = decode_sword_wave_parameters(0x2000FFFBU);
    CHECK(hyper_cfg.type == SwordWaveType::Hyper);
    CHECK(hyper_cfg.elevation == -5);
    CHECK(hyper_cfg.damage == 0x0000U);
    CHECK(hyper_cfg.anim_id == 4U);
    CHECK(!hyper_cfg.piercing);
    CHECK(hyper_cfg.flag_3c_bit1);
    CHECK(hyper_cfg.speed_x == 0x0E00U);
    CHECK(hyper_cfg.speed_y == 0x0D00U);

    // 5. Composite bitmask fallthrough parity: Fire (bit 31) falls through into Omega (bit 30) / Hyper (bit 29)
    const auto fire_omega = decode_sword_wave_parameters(0xC000001CU);
    CHECK(fire_omega.anim_id == 4U);
    CHECK(fire_omega.flag_3c_bit1);
    CHECK(fire_omega.piercing);
    CHECK(fire_omega.damage == 0x8005U);
    CHECK(fire_omega.speed_x == 0x1000U);
    CHECK(fire_omega.speed_y == 0x0F00U);

    const auto fire_hyper = decode_sword_wave_parameters(0xA000000DU);
    CHECK(fire_hyper.damage == 0x0000U);
    CHECK(fire_hyper.anim_id == 4U);
    CHECK(fire_hyper.flag_3c_bit1);
    CHECK(fire_hyper.speed_x == 0x0E00U);
    CHECK(fire_hyper.speed_y == 0x0D00U);
}

void test_anchor_and_8way_spawn() {
    // Ground facing Up (dx=+1, dy=-40) + elev=28 -> dy+elev = -12
    const auto a_ground_up = compute_sword_wave_anchor(
        SwordWaveSurface::Ground, 0U, kSwordWaveGroundElevDefault, false);
    CHECK(a_ground_up.anchor_dx_fp == 0x00010000);
    CHECK(a_ground_up.anchor_dy_fp == -0x000C0000);

    // Water facing Right (dx=+20, dy=-17) + elev=0, slowmo bit 0 set -> doubled
    const auto a_water_slow = compute_sword_wave_anchor(
        SwordWaveSurface::Water, 1U, 0, true);
    CHECK(a_water_slow.anchor_dx_fp == 0x00280000);
    CHECK(a_water_slow.anchor_dy_fp == -0x00220000);

    // Spawn projections along cardinal & diagonal directions (0x00E2D4)
    const ElementalBladeOwnerState ali_up{
        .owner_ram_addr = 0x00FF19E8U,
        .pos_x_fp = 0x01008000,
        .pos_y_fp = 0x00B44000,
        .facing = 0U,
    };
    const auto fire_up = spawn_elemental_blade(
        SwordWaveSurface::Ground, ali_up, 0x8000001CU, false, 0x10U);
    CHECK(fire_up.type_id == 0x0034U);
    CHECK(fire_up.pos_x_fp == 0x01008000);
    CHECK(fire_up.pos_y_fp == 0x00B44000);
    CHECK(fire_up.pos_z_fp == 0x001C0000);
    CHECK(fire_up.facing == 0U);
    CHECK(fire_up.sprite_priority_18 == 0x055EU);
    CHECK(fire_up.anim_id_2a == 4U);
    CHECK(fire_up.flags_38 == 0x06U);
    CHECK(fire_up.flags_3c == 0x12U);
    CHECK(fire_up.flags_3d == 0x00U);
    CHECK(fire_up.hitbox_radius_46 == 6U);
    CHECK(fire_up.hitbox_rx_48 == 6U);
    CHECK(fire_up.hitbox_ry_49 == 6U);
    CHECK(fire_up.hitbox_height_4a == 8U);
    CHECK(fire_up.vel_x_fp == 0);
    CHECK(fire_up.vel_y_fp == -0x000BC000);
    CHECK(fire_up.vel_z_fp == 0x0000C000);
    CHECK(fire_up.anchor_dx_fp == 0x00010000);
    CHECK(fire_up.anchor_dy_fp == -0x000C0000);
    CHECK(fire_up.lifetime_a6 == 240U);
    CHECK(fire_up.pierce_a7 == 0U);
    CHECK(fire_up.damage_a8 == 0x2004U);
    CHECK(fire_up.owner_ptr_aa == 0x00FF19E8U);

    // Diagonal NE (facing 4) with Omega speeds (0x1000, 0x0F00) -> (+0.75 * 16.0, -0.75 * 15.0)
    const ElementalBladeOwnerState ali_ne{
        .owner_ram_addr = 0x00FF19E8U,
        .pos_x_fp = 0x00800000,
        .pos_y_fp = 0x00800000,
        .facing = 4U,
    };
    const auto omega_ne = spawn_elemental_blade(
        SwordWaveSurface::Water, ali_ne, 0x4000000DU, false);
    CHECK(omega_ne.vel_x_fp == 0x000C0000);
    CHECK(omega_ne.vel_y_fp == -0x000B4000);
    CHECK(omega_ne.pierce_a7 == 0xFFU);
    CHECK(omega_ne.damage_a8 == 0x8005U);
    CHECK(omega_ne.anchor_dx_fp == 0x00030000);
    CHECK(omega_ne.anchor_dy_fp == -0x00130000); // Water Up (-32) + 13 = -19
}

void test_ram_window_spawn_and_guards() {
    std::array<std::uint8_t, 0x10000> ram{};
    ram.fill(0xA5U);

    // Populate owner Ali at $FF19E8 (0x19E8)
    constexpr std::size_t kAliOff = 0x19E8U;
    write_be32_signed(ram, kAliOff + 0x08U, 0x01008000);
    write_be32_signed(ram, kAliOff + 0x0CU, 0x00B44000);
    write_be16(ram, kAliOff + 0x16U, 1U); // Right
    ram[kSwordWavePauseSlowmoRamOffset] = 0x00U;

    // 1. Ground Fire Sword Wave ($FF1B60)
    const auto ground_fire = spawn_elemental_blade_ground_from_ram(
        ram, 0x00FF19E8U, 0x8000001CU);
    const auto loaded_ground = load_sword_wave_slot_from_ram(ram);
    CHECK(loaded_ground == ground_fire);
    CHECK(read_be16(ram, kSwordWaveSlotRamOffset + 0x00U) == 0x0034U);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x08U) == 0x01008000);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x0CU) == 0x00B44000);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x10U) == 0x001C0000);
    CHECK(read_be16(ram, kSwordWaveSlotRamOffset + 0x16U) == 1U);
    CHECK(read_be16(ram, kSwordWaveSlotRamOffset + 0x18U) == 0x055EU);
    CHECK(read_be16(ram, kSwordWaveSlotRamOffset + 0x2AU) == 4U);
    CHECK(ram[kSwordWaveSlotRamOffset + 0x38U] == 0x06U);
    CHECK(ram[kSwordWaveSlotRamOffset + 0x3CU] == 0x02U);
    CHECK(ram[kSwordWaveSlotRamOffset + 0x3DU] == 0x00U);
    CHECK(read_be16(ram, kSwordWaveSlotRamOffset + 0x46U) == 6U);
    CHECK(ram[kSwordWaveSlotRamOffset + 0x48U] == 6U);
    CHECK(ram[kSwordWaveSlotRamOffset + 0x49U] == 6U);
    CHECK(read_be16(ram, kSwordWaveSlotRamOffset + 0x4AU) == 8U);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x4EU) == 0x000C0000);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x52U) == 0);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x56U) == 0x0000C000);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x72U) == 0x00100000);
    CHECK(read_be32_signed(ram, kSwordWaveSlotRamOffset + 0x76U) == 0x00020000); // -26 + 28 = +2
    CHECK(ram[kSwordWaveSlotRamOffset + 0xA6U] == 0xF0U);
    CHECK(ram[kSwordWaveSlotRamOffset + 0xA7U] == 0x00U);
    CHECK(read_be16(ram, kSwordWaveSlotRamOffset + 0xA8U) == 0x2004U);
    CHECK(read_be32(ram, kSwordWaveSlotRamOffset + 0xAAU) == 0x00FF19E8U);

    // Bytes adjacent to slot $FF1B60..$FF1C1B must stay 0xA5, and cleared unused bytes inside slot are 0
    CHECK(ram[kSwordWaveSlotRamOffset - 1U] == 0xA5U);
    CHECK(ram[kSwordWaveSlotRamOffset + kSwordWaveSlotStrideBytes] == 0xA5U);
    CHECK(ram[kSwordWaveSlotRamOffset + 0x04U] == 0x00U);

    // 2. Water Omega with slow-motion bit 0 set in $FF17B8
    ram[kSwordWavePauseSlowmoRamOffset] = 0x01U;
    write_be16(ram, kAliOff + 0x16U, 3U); // Left
    const auto water_omega = spawn_elemental_blade_water_from_ram(
        ram, 0x19E8U, 0x4000000DU);
    CHECK(water_omega.vel_x_fp == -0x00100000);
    CHECK(water_omega.vel_y_fp == 0);
    CHECK(water_omega.anchor_dx_fp == -0x00280000); // (-20 * 2) << 16
    CHECK(water_omega.anchor_dy_fp == -0x00080000); // ((-17 + 13) * 2) << 16
    CHECK(water_omega.pierce_a7 == 0xFFU);
    CHECK(water_omega.damage_a8 == 0x8005U);
    CHECK(water_omega.owner_ptr_aa == 0x00FF19E8U);

    // 3. OOB / short buffer guards
    std::array<std::uint8_t, 64> short_ram{};
    CHECK(load_sword_wave_slot_from_ram(short_ram) == SwordWaveSlotState{});
    store_sword_wave_slot_to_ram(short_ram, water_omega);
    CHECK(spawn_elemental_blade_ground_from_ram(short_ram, 0x19E8U, 0U) == SwordWaveSlotState{});
    CHECK(spawn_elemental_blade_water_from_ram(ram, 0x00FE0000U, 0U) == SwordWaveSlotState{});
    CHECK(spawn_elemental_blade_ground_from_ram(ram, 0xFFF8U, 0U) == SwordWaveSlotState{});
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
    std::vector<std::uint8_t> slice(kElementalBladeSizeBytes);
    in.seekg(static_cast<std::streamoff>(kElementalBladeStartAddr), std::ios::beg);
    in.read(reinterpret_cast<char*>(slice.data()), static_cast<std::streamsize>(slice.size()));
    CHECK(in.gcount() == static_cast<std::streamsize>(slice.size()));

    for (std::size_t i = 0U; i < 4U; ++i) {
        const std::size_t g_off = (kElementalBladeGroundOffsetsAddr - kElementalBladeStartAddr) + i * 2U;
        const std::size_t w_off = (kElementalBladeWaterOffsetsAddr - kElementalBladeStartAddr) + i * 2U;
        CHECK(read_be16(slice, g_off) == kGroundSwordTipWords[i]);
        CHECK(read_be16(slice, w_off) == kWaterSwordTipWords[i]);
    }
    CHECK(read_be32(slice, 0x00F280U - kElementalBladeStartAddr) == 0x4DF900FFU);
    CHECK(read_be16(slice, 0x00F284U - kElementalBladeStartAddr) == 0x1B60U);
    CHECK(read_be32(slice, 0x00F382U - kElementalBladeStartAddr) == 0x0000C000U);
    CHECK(read_be16(slice, 0x00F394U - kElementalBladeStartAddr) == 0x4E75U);
}

} // namespace

int main() {
    test_constants_and_coexistence();
    test_tip_offset_tables();
    test_sword_parameter_decoding();
    test_anchor_and_8way_spawn();
    test_ram_window_spawn_and_guards();
    test_optional_local_rom_tables();

    std::cout << "[PASS] All M16.202 elemental blade tests succeeded!\n";
    return 0;
}
