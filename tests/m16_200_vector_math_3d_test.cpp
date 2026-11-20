#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "game/math/isqrt_atan2.hpp"
#include "game/math/vector_math_3d.hpp"

namespace {

int g_failures = 0;

void check_impl(bool cond, const char* expr, int line) {
    if (!cond) {
        std::cerr << "CHECK FAILED at line " << line << ": " << expr << '\n';
        ++g_failures;
    }
}

#define CHECK(expr) check_impl(static_cast<bool>(expr), #expr, __LINE__)

void write_be16(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t val) {
    buf[off] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

void write_be32(std::vector<std::uint8_t>& buf, std::size_t off, std::uint32_t val) {
    buf[off] = static_cast<std::uint8_t>((val >> 24U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>((val >> 16U) & 0xFFU);
    buf[off + 2U] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[off + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
}

std::uint16_t read_be16(const std::vector<std::uint8_t>& buf, std::size_t off) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[off]) << 8U) |
        static_cast<std::uint16_t>(buf[off + 1U]));
}

std::uint32_t read_be32(const std::vector<std::uint8_t>& buf, std::size_t off) {
    return (static_cast<std::uint32_t>(buf[off]) << 24U) |
           (static_cast<std::uint32_t>(buf[off + 1U]) << 16U) |
           (static_cast<std::uint32_t>(buf[off + 2U]) << 8U) |
           static_cast<std::uint32_t>(buf[off + 3U]);
}

void test_constants_and_table() {
    using namespace oasis::game::math;

    CHECK(kVectorMath3dStartAddr == 0x00F054U);
    CHECK(kBoss3ARockVelocityTableAddr == 0x00F0C2U);
    CHECK(kDistance2dAddr == 0x00F0D2U);
    CHECK(kDistance3dAddr == 0x00F0ECU);
    CHECK(kNormalizeVelocity3dAddr == 0x00F10CU);
    CHECK(kClampToSphere3dAddr == 0x00F160U);
    CHECK(kVectorMath3dEndAddr == 0x00F1B2U);
    CHECK(kVectorMath3dSizeBytes == 350U);

    // Ensure co-inclusion with isqrt_atan2.hpp works seamlessly.
    CHECK(kIsqrtU32Addr == 0x00E6BAU);
    CHECK(isqrt_u32(169U) == 13U);

    CHECK(kBoss3ARockVelocityTable[0] == (Boss3ARockVelocityEntry{0x0300U, 0x0300}));
    CHECK(kBoss3ARockVelocityTable[1] == (Boss3ARockVelocityEntry{0x0500U, 0x0400}));
    CHECK(kBoss3ARockVelocityTable[2] == (Boss3ARockVelocityEntry{0x0700U, 0x0500}));
    CHECK(kBoss3ARockVelocityTable[3] == (Boss3ARockVelocityEntry{0x0000U, 0x0800}));
    CHECK(select_boss3a_rock_velocity_entry(0U) == kBoss3ARockVelocityTable[0]);
    CHECK(select_boss3a_rock_velocity_entry(99U) == kBoss3ARockVelocityTable[3]);
}

void test_distances_2d_and_3d() {
    using namespace thor::math;

    CHECK(distance_2d(0, 0, 0, 0) == 0U);
    CHECK(distance_2d(10, 20, 13, 24) == 5U);
    CHECK(distance_2d(-10, -20, -15, -32) == 13U);
    CHECK(distance_2d(-32768, 0, 32767, 0) == 1U); // 16-bit sub.w wrap: 32767 - (-32768) == -1

    CHECK(distance_3d({0, 0, 0}, {0, 0, 0}) == 0U);
    CHECK(distance_3d({0, 0, 0}, {1, 2, 2}) == 3U);
    CHECK(distance_3d({10, -5, 4}, {12, -2, 10}) == 7U);
    CHECK(distance_3d({-1, -2, -3}, {2, 2, 9}) == 13U);
    CHECK(distance_3d({0, 0, 0}, {400, 400, 400}) == 692U);
    // isqrt_u32 (0x00E6BA) seeds guess cap at kIsqrtGuessCap (0x0400 = 1024) for sum_sq >= 0x00100000.
    CHECK(distance_3d({0, 0, 0}, {32767, 32767, 32767}) == oasis::game::math::kIsqrtGuessCap);
    CHECK(distance_3d({0, 0, 0}, {-32768, -32768, -32768}) == oasis::game::math::kIsqrtGuessCap);
}

void test_normalize_velocity_3d() {
    using namespace thor::math;

    // Zero distance exits with zero vector.
    CHECK(normalize_velocity_3d({42, -17, 9}, {42, -17, 9}, 100) == (Vec3Fixed{0, 0, 0}));

    // Cardinal direction -> exact << 12 fixed-point scaling.
    CHECK(normalize_velocity_3d({0, 0, 0}, {100, 0, 0}, 16) == (Vec3Fixed{0x00010000, 0, 0}));

    // 3-4-12 (dist = 13), speed = 26 -> quotients (6, 8, 24) << 12.
    const Vec3Fixed v1 = normalize_velocity_3d({10, 20, 30}, {13, 24, 42}, 26);
    CHECK(v1 == (Vec3Fixed{6 << 12, 8 << 12, 24 << 12}));

    // Negative delta + truncating divs.w (2-3-6, dist = 7, speed = 10 -> q = (-2, -4, -8)).
    const Vec3Fixed v2 = normalize_velocity_3d({0, 0, 0}, {-2, -3, -6}, 10);
    CHECK(v2 == (Vec3Fixed{-2 * 4096, -4 * 4096, -8 * 4096}));

    // Negative speed reverses direction.
    const Vec3Fixed v3 = normalize_velocity_3d({0, 0, 0}, {2, 3, 6}, -7);
    CHECK(v3 == (Vec3Fixed{-2 * 4096, -3 * 4096, -6 * 4096}));

    // Quotient overflow guard for speed = -32768 on (-dist, 0, 0): (-10 * -32768) / 10 = +32768.
    const Vec3Fixed v_ovf = normalize_velocity_3d({0, 0, 0}, {-10, 0, 0}, -32768);
    CHECK(v_ovf == (Vec3Fixed{0, 0, 0}));
}

void test_clamp_to_sphere_3d() {
    using namespace thor::math;

    // Inside or on sphere boundary (dist <= radius): preserved with clamped=false.
    const SphereClampResult zero = clamp_to_sphere_3d({5, -5, 10}, {5, -5, 10}, 0U);
    CHECK(!zero.clamped && zero.distance == 0U && zero.position == (Vec3i16{5, -5, 10}));

    const SphereClampResult on_edge = clamp_to_sphere_3d({1, 2, 3}, {4, 6, 15}, 13U);
    CHECK(!on_edge.clamped && on_edge.distance == 13U && on_edge.position == (Vec3i16{4, 6, 15}));

    // Unsigned bls compare: radius >= 0x8000 is larger than small dist.
    const SphereClampResult large_u16_rad = clamp_to_sphere_3d({0, 0, 0}, {30, 40, 0}, 40000U);
    CHECK(!large_u16_rad.clamped && large_u16_rad.distance == 50U);

    // Outside sphere: (6, 8, 24) has dist 26; clamp to radius 13 -> (3, 4, 12).
    const SphereClampResult half = clamp_to_sphere_3d({10, -20, 30}, {16, -12, 54}, 13U);
    CHECK(half.clamped && half.distance == 26U && half.position == (Vec3i16{13, -16, 42}));

    // Clamp to radius 0 when dist > 0 collapses onto origin.
    const SphereClampResult zero_rad = clamp_to_sphere_3d({-7, 11, 19}, {3, 21, 19}, 0U);
    CHECK(zero_rad.clamped && zero_rad.distance == 14U && zero_rad.position == (Vec3i16{-7, 11, 19}));
}

void test_boss3a_rock_launch() {
    using namespace thor::math;

    // Close range (max(|dx|, |dy|) == 0x00080000) forces entry 3 (speed_xy=0, vel_z=0x00080000).
    Boss3ASpawnerState spawner{0x01000000, 0x02000000, 1U, 0xCAFEBABEU};
    Boss3AProjectileState proj{};
    proj.x_fp = 0x01000000 + 0x00080000;
    proj.y_fp = 0x02000000 - 0x00020000;
    proj.target_ram_addr = 0x00FF2000U;

    const Boss3ALaunchResult close_res = setup_boss3a_rock_launch(spawner, proj);
    CHECK(close_res.close_range_override);
    CHECK(close_res.effective_table_index == 3U);
    CHECK(close_res.facing_dir_4way == 1U); // East
    CHECK(close_res.vel_x_fp == 0 && close_res.vel_y_fp == 0);
    CHECK(close_res.vel_z_fp == 0x00080000);
    CHECK(close_res.hud_tracker_updated);
    CHECK(proj.param_7e == 0xCAFEBABEU);
    CHECK(proj.facing_dir_4way == 1U);

    // Beyond 0x00080000 uses attack_phase_index across all 4 cardinal directions.
    spawner.attack_phase_index = 2U; // speed_xy = 0x0700 -> 0x00070000, vel_z = 0x00050000
    proj.x_fp = 0x01000000;
    proj.y_fp = 0x02000000 - 0x00080001; // North (dir 0)
    proj.target_ram_addr = kPlayerEntityRamAddr; // Suppresses HUD lock.
    const Boss3ALaunchResult north_res = setup_boss3a_rock_launch(spawner, proj);
    CHECK(!north_res.close_range_override);
    CHECK(!north_res.hud_tracker_updated);
    CHECK(north_res.facing_dir_4way == 0U);
    CHECK(north_res.vel_x_fp == 0 && north_res.vel_y_fp == -0x00070000 && north_res.vel_z_fp == 0x00050000);

    // West (dir 3) with phase 0 and flags_3a bit 0 set (HUD lock suppressed).
    spawner.attack_phase_index = 0U;
    proj.x_fp = 0x01000000 - 0x00100001;
    proj.y_fp = 0x02000000 + 0x00100000;
    proj.target_ram_addr = 0x00FF2400U;
    proj.flags_3a = 0x01U;
    const Boss3ALaunchResult west_res = setup_boss3a_rock_launch(spawner, proj);
    CHECK(!west_res.hud_tracker_updated);
    CHECK(west_res.facing_dir_4way == 3U);
    CHECK(west_res.vel_x_fp == -0x00030000 && west_res.vel_y_fp == 0 && west_res.vel_z_fp == 0x00030000);

    // Tie-break |dx| == |dy| picks South when dy > 0; OOB phase index clamps safely to 3.
    spawner.attack_phase_index = 9U;
    proj.x_fp = 0x01000000 - 0x00100000;
    proj.y_fp = 0x02000000 + 0x00100000;
    const Boss3ALaunchResult oob_res = setup_boss3a_rock_launch(spawner, proj);
    CHECK(oob_res.table_index_clamped && oob_res.effective_table_index == 3U);
    CHECK(oob_res.facing_dir_4way == 2U);
    CHECK(oob_res.vel_z_fp == 0x00080000);
}

void test_wram_helpers() {
    using namespace thor::math;

    std::vector<std::uint8_t> wram(kWramSizeBytes, 0U);
    constexpr std::size_t kSpawnerOff = 0x2200U;
    constexpr std::size_t kProjOff = 0x2400U;

    // Seed positions with non-zero fractional low words (0x1234, 0x5678, 0x9ABC) to verify preservation.
    write_be32(wram, kSpawnerOff + kEntityPosXFpOffset, 0x00641234U); // x=100
    write_be32(wram, kSpawnerOff + kEntityPosYFpOffset, 0x00325678U); // y=50
    write_be32(wram, kSpawnerOff + kEntityPosZFpOffset, 0x000A9ABCU); // z=10
    write_be16(wram, kSpawnerOff + kSpawnerAttackPhaseOffset, 1U);
    write_be32(wram, kSpawnerOff + kSpawnerProjectileParamOffset, 0x11223344U);

    write_be32(wram, kProjOff + kEntityPosXFpOffset, 0x006A1111U); // x=106 (dx=6)
    write_be32(wram, kProjOff + kEntityPosYFpOffset, 0x003A2222U); // y=58  (dy=8)
    write_be32(wram, kProjOff + kEntityPosZFpOffset, 0x00223333U); // z=34  (dz=24, dist=26)

    CHECK(distance_2d_entities_from_ram(wram, kSpawnerOff, kProjOff) == 10U);
    CHECK(distance_3d_entities_from_ram(wram, kSpawnerOff, kProjOff) == 26U);
    CHECK(normalize_entity_velocity_3d_from_ram(wram, kSpawnerOff, kProjOff, 26) ==
          (Vec3Fixed{6 << 12, 8 << 12, 24 << 12}));

    // setup_boss3a_rock_launch_from_ram: |dy| = 8.0 - fractional > 8.0? 0x003A2222 - 0x00325678 = 0x0007CBAA <= 0x80000!
    // Move projectile Y slightly farther so |dy| > 8 px and phase 1 applies.
    write_be32(wram, kProjOff + kEntityPosYFpOffset, 0x003B5678U); // dy = +9.0 px > 8.0 px (South)
    const Boss3ALaunchResult ram_launch = setup_boss3a_rock_launch_from_ram(wram, kSpawnerOff, kProjOff);
    CHECK(!ram_launch.close_range_override);
    CHECK(ram_launch.facing_dir_4way == 2U); // South
    CHECK(ram_launch.hud_tracker_updated);
    CHECK(read_be32(wram, kProjOff + kEntityInheritedParamOffset) == 0x11223344U);
    CHECK(read_be16(wram, kProjOff + kEntityFacingDir4WayOffset) == 2U);
    CHECK(static_cast<std::int32_t>(read_be32(wram, kProjOff + kEntityVelXFpOffset)) == 0);
    CHECK(static_cast<std::int32_t>(read_be32(wram, kProjOff + kEntityVelYFpOffset)) == 0x00050000);
    CHECK(static_cast<std::int32_t>(read_be32(wram, kProjOff + kEntityVelZFpOffset)) == 0x00040000);
    CHECK(wram[kEnemyHudTimerRamOffset] == kEnemyHudLockDurationFrames);
    CHECK(read_be32(wram, kEnemyHudTargetPtrRamOffset) == (kWramBaseAddr + kProjOff));

    // Clamp target position (dx=6, dy=9 -> set back to 6,8,24 first).
    write_be32(wram, kProjOff + kEntityPosYFpOffset, 0x003A2222U);
    const SphereClampResult ram_clamp = clamp_entity_pos_to_sphere_from_ram(wram, kSpawnerOff, kProjOff, 13U);
    CHECK(ram_clamp.clamped && ram_clamp.distance == 26U);
    CHECK(read_entity_pos_i16_from_ram(wram, kProjOff) == (Vec3i16{103, 54, 22}));
    CHECK(read_be16(wram, kProjOff + kEntityPosXFpOffset + 2U) == 0x1111U);
    CHECK(read_be16(wram, kProjOff + kEntityPosYFpOffset + 2U) == 0x2222U);
    CHECK(read_be16(wram, kProjOff + kEntityPosZFpOffset + 2U) == 0x3333U);

    // Out-of-bounds buffer safety.
    std::vector<std::uint8_t> tiny(16U, 0U);
    CHECK(read_entity_pos_i16_from_ram(tiny, 0U) == (Vec3i16{0, 0, 0}));
    CHECK(distance_3d_entities_from_ram(tiny, 0U, 0U) == 0U);
    const Boss3ALaunchResult oob = setup_boss3a_rock_launch_from_ram(tiny, 0U, 0U);
    CHECK(!oob.hud_tracker_updated && oob.vel_z_fp == 0);
}

void test_optional_local_rom_bytes() {
    const std::array<std::filesystem::path, 3> candidates = {
        std::filesystem::path("../Sega-Thor/local-roms/Beyond Oasis (USA).md"),
        std::filesystem::path("local-roms/Beyond Oasis (USA).md"),
        std::filesystem::path("C:/Github/Sega-Thor/local-roms/Beyond Oasis (USA).md"),
    };

    std::filesystem::path rom_path;
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec)) {
            rom_path = candidate;
            break;
        }
    }
    if (rom_path.empty()) {
        std::cout << "[SKIP] Local commercial ROM not present; skipping ROM byte verification.\n";
        return;
    }

    std::ifstream ifs(rom_path, std::ios::binary);
    CHECK(ifs.good());
    std::vector<std::uint8_t> rom(thor::math::kVectorMath3dEndAddr);
    ifs.read(reinterpret_cast<char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
    CHECK(static_cast<std::size_t>(ifs.gcount()) == rom.size());

    // Verify 0x00F054 bsr.w $be98 and 0x00BEAC HUD tracker latch.
    CHECK(read_be32(rom, 0x00F054U) == 0x6100CE42U);
    CHECK(read_be32(rom, 0x00BEACU) == 0x13FC005AU);
    CHECK(read_be32(rom, 0x00BEB0U) == 0x00FF184FU);
    CHECK(read_be32(rom, 0x00BEB6U) == 0x00FF1850U);

    // Verify 4-entry velocity table at 0x00F0C2..0x00F0D2.
    for (std::size_t i = 0U; i < thor::math::kBoss3ARockVelocityTable.size(); ++i) {
        const std::size_t off = thor::math::kBoss3ARockVelocityTableAddr + (i * 4U);
        const std::uint16_t rom_speed = read_be16(rom, off);
        const std::int16_t rom_z = static_cast<std::int16_t>(read_be16(rom, off + 2U));
        CHECK(rom_speed == thor::math::kBoss3ARockVelocityTable[i].speed_xy_8_8);
        CHECK(rom_z == thor::math::kBoss3ARockVelocityTable[i].launch_z_8_8);
    }

    // Verify entry opcodes of 0x00F0D2, 0x00F0EC, 0x00F10C, 0x00F160 and final rts at 0x00F1B0.
    CHECK(read_be32(rom, 0x00F0D2U) == 0x48E71800U);
    CHECK(read_be32(rom, 0x00F0ECU) == 0x48E70E00U);
    CHECK(read_be32(rom, 0x00F10CU) == 0x48E70E00U);
    CHECK(read_be32(rom, 0x00F160U) == 0x48E70E00U);
    CHECK(read_be16(rom, 0x00F1B0U) == 0x4E75U);
}

} // namespace

int main() {
    test_constants_and_table();
    test_distances_2d_and_3d();
    test_normalize_velocity_3d();
    test_clamp_to_sphere_3d();
    test_boss3a_rock_launch();
    test_wram_helpers();
    test_optional_local_rom_bytes();

    if (g_failures != 0) {
        std::cerr << "m16_200_vector_math_3d_test FAILED with " << g_failures << " failure(s).\n";
        return 1;
    }
    std::cout << "m16_200_vector_math_3d_test PASSED.\n";
    return 0;
}
