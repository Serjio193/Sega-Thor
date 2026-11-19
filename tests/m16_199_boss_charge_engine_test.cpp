#include "game/enemy/boss_charge_engine.hpp"
#include "../include/game/enemy/boss_charge_engine.hpp"
#include "game/enemy/dungeon_enemy_dispatch.hpp"
#include "game/enemy/enemy_ai_0030_0048.hpp"
#include "game/enemy/heavy_enemy_dispatch.hpp"

#include "core/rom.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using namespace oasis::game::enemy;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.199 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void test_constants_and_damping_math() {
    static_assert(kBossChargeState4Addr == 0x00EE94U);
    static_assert(kBossChargeState2Addr == 0x00EF1EU);
    static_assert(kBossChargeState0Addr == 0x00EF38U);
    static_assert(kBossChargeDampingAddr == 0x00EF5EU);
    static_assert(kBossChargeTrajectoryAddr == 0x00EFA2U);
    static_assert(kBossChargeDespawnNotifyAddr == 0x00F038U);
    static_assert(kBossChargeEngineEndAddr == 0x00F054U);
    static_assert(kBossChargeEngineEndAddr - kBossChargeState4Addr == kBossChargeEngineSizeBytes);
    static_assert(kBossChargeSpatialSfxAddr == 0x00E7E8U);

    static_assert(damp_velocity_75_percent(0x00040000) == 0x00030000);
    static_assert(damp_velocity_75_percent(-0x00040000) == -0x00030000);
    static_assert(damp_velocity_75_percent(0) == 0);

    // 0.75x damping step: X/Y add damped vel to +$72/+$76; Z adds half_z (0.5x) to +$10 and clamps >= 0
    BossChargeEntityState ent{};
    ent.vel_x_4e = 0x00040000;
    ent.vel_y_52 = -0x00020000;
    ent.vel_z_56 = 0x00080000;
    ent.elev_z_10 = 0x00100000;
    boss_charge_damping_step(ent);
    CHECK(ent.vel_x_4e == 0x00030000);
    CHECK(ent.accum_x_72 == 0x00030000);
    CHECK(ent.vel_y_52 == -0x00018000);
    CHECK(ent.accum_y_76 == -0x00018000);
    CHECK(ent.vel_z_56 == 0x00060000);
    CHECK(ent.elev_z_10 == 0x00140000); // +0.5 * 0x00080000 = +0x00040000

    // Floor clamp on negative damped Z
    ent.elev_z_10 = 0x00010000;
    ent.vel_z_56 = -0x00080000;
    boss_charge_damping_step(ent);
    CHECK(ent.vel_z_56 == -0x00060000);
    CHECK(ent.elev_z_10 == 0);
}

void test_trajectory_bounce_water_and_culling() {
    const BossChargeViewport vp{100, 100, kBossChargeViewportMarginPx};

    // 1. Grounded downward-gravity branch stops velocity without bouncing or SFX
    BossChargeEntityState grounded{};
    grounded.type_00 = 0x003A;
    grounded.world_x_08 = 160;
    grounded.world_y_0c = 160;
    grounded.elev_z_10 = 0;
    grounded.vel_x_4e = 0x00010000;
    grounded.vel_y_52 = 0x00020000;
    grounded.vel_z_56 = 0x00004000; // 0x4000 - 0xC000 = -0x8000 < 0
    const auto g_res = boss_charge_trajectory_step(grounded, vp);
    CHECK(!g_res.bounced);
    CHECK(g_res.velocity_stopped);
    CHECK(!g_res.despawned);
    CHECK(grounded.vel_x_4e == 0);
    CHECK(grounded.vel_y_52 == 0);
    CHECK(grounded.vel_z_56 == 0);

    // 2. Floor impact bounce: 50% Z recoil, 50% X/Y friction, toggles h-flip bit 11, SFX 0x400376
    BossChargeEntityState air{};
    air.type_00 = 0x003A;
    air.world_x_08 = 200;
    air.world_y_0c = 200;
    air.elev_z_10 = 0x00008000;
    air.vel_x_4e = 0x00040000;
    air.vel_y_52 = -0x00020000;
    air.vel_z_56 = -0x00024000; // -0x24000 - 0xC000 = -0x30000 -> elev < 0 -> recoil = 0x18000
    air.render_attr_18 = 0x0010U;
    const auto b_res = boss_charge_trajectory_step(air, vp);
    CHECK(b_res.bounced);
    CHECK(!b_res.velocity_stopped); // 0x18000 >= 0x4000
    CHECK(air.elev_z_10 == 0);
    CHECK(air.vel_z_56 == 0x00018000);
    CHECK(air.vel_x_4e == 0x00020000);
    CHECK(air.vel_y_52 == -0x00010000);
    CHECK(air.render_attr_18 == 0x0810U);
    CHECK((b_res.sfx_emitted == std::vector<std::uint32_t>{kBossChargeSfxBounce}));

    // 3. Low-energy bounce (< 0x4000) stops all velocities after playing bounce SFX
    air.elev_z_10 = 0x00001000;
    air.vel_x_4e = 0x00010000;
    air.vel_z_56 = 0x00008000; // 0x8000 - 0xC000 = -0x4000 -> elev -0x3000 -> recoil = 0x2000 < 0x4000
    const auto rest_res = boss_charge_trajectory_step(air, vp);
    CHECK(rest_res.bounced);
    CHECK(rest_res.velocity_stopped);
    CHECK(air.vel_x_4e == 0);
    CHECK(air.vel_y_52 == 0);
    CHECK(air.vel_z_56 == 0);
    CHECK(air.render_attr_18 == 0x0010U);

    // 4. Water surface bit 3 on +$36 triggers despawn & spawner notification
    BossChargeEntityState water{};
    water.type_00 = 0x003A;
    water.world_x_08 = 160;
    water.world_y_0c = 160;
    water.elev_z_10 = 0x00002000;
    water.vel_z_56 = 0; // -0xC000 -> impacts floor
    water.spawner_ptr_aa = 0x00FF1C1CU;
    water.self_ram_addr = 0x00FF1CD8U;
    const auto w_res = boss_charge_trajectory_step(water, vp, true);
    CHECK(w_res.despawned);
    CHECK(w_res.spawner_notified);
    CHECK(w_res.notified_spawner_addr == 0x00FF1C1CU);
    CHECK(w_res.spawner_notify_msg == 0x0010U);
    CHECK(water.type_00 == 0);

    // 5. Viewport culling outside [cam - 48, cam + 320x224 + 48] despawns; exact boundary stays alive
    BossChargeEntityState edge{};
    edge.type_00 = 0x003A;
    edge.world_x_08 = 100 + 320 + 48; // 468 inclusive -> alive
    edge.world_y_0c = 100 - 48;       // 52 inclusive -> alive
    CHECK(!boss_charge_trajectory_step(edge, vp).despawned);
    edge.world_x_08 = 100 + 320 + 49; // 469 -> culled
    const auto cull_res = boss_charge_trajectory_step(edge, vp);
    CHECK(cull_res.offscreen_culled);
    CHECK(cull_res.despawned);
    CHECK(edge.type_00 == 0);
}

void test_states_0_2_4_and_spawner_guards() {
    const BossChargeViewport vp{0, 0, kBossChargeViewportMarginPx};

    // State 0 countdown -> State 2 when timer decrements to 0
    BossChargeEntityState s0{};
    s0.type_00 = 0x003A;
    s0.world_x_08 = 100;
    s0.world_y_0c = 100;
    s0.timer_02 = 2U;
    CHECK(!boss_charge_state0_step(s0, vp).state_transitioned);
    CHECK(s0.timer_02 == 1U);
    CHECK(s0.state_04 == 0U);
    CHECK(boss_charge_state0_step(s0, vp).state_transitioned);
    CHECK(s0.timer_02 == 0U);
    CHECK(s0.state_04 == 2U);
    CHECK(s0.anim_2a == 2U);

    // State 2 -> State 4 on +$37 bit 1
    CHECK(!boss_charge_state2_step(s0, vp).state_transitioned);
    s0.flags_37 = kBossChargeFlagAnimDoneBit;
    CHECK(boss_charge_state2_step(s0, vp).state_transitioned);
    CHECK(s0.state_04 == 4U);
    CHECK(s0.anim_2a == 4U);

    // State 4 subtypes 0, 1, and >1
    for (const std::uint16_t sub : {std::uint16_t{0U}, std::uint16_t{1U}, std::uint16_t{2U}}) {
        BossChargeEntityState s4{};
        s4.type_00 = 0x003A;
        s4.world_x_08 = 100;
        s4.world_y_0c = 100;
        s4.state_04 = 4U;
        s4.render_attr_18 = 0xE8FFU; // bits 11, 13, 14 set -> cleared to 0x80FF
        s4.flags_37 = kBossChargeFlagAnimDoneBit;
        s4.subtype_a6 = sub;
        const auto r4 = boss_charge_state4_step(s4, vp);
        CHECK(r4.state_transitioned);
        CHECK(s4.render_attr_18 == 0x80FFU);
        CHECK((s4.flags_36 & kBossChargeFlagWaterBit) != 0U);
        CHECK((s4.flags_38 & kBossChargeFlag38Bit3) != 0U);
        CHECK((s4.flags_3c & kBossChargeFlag3cBit1) != 0U);
        if (sub == 0U) {
            CHECK(s4.state_04 == 6U);
            CHECK(s4.anim_2a == 6U);
            CHECK((r4.sfx_emitted == std::vector<std::uint32_t>{kBossChargeSfxSwing}));
            CHECK(r4.screen_shake_frames == 0U);
        } else if (sub == 1U) {
            CHECK(s4.state_04 == 6U);
            CHECK(s4.anim_2a == 10U);
            CHECK((r4.sfx_emitted == std::vector<std::uint32_t>{kBossChargeSfxSwing}));
            CHECK(r4.screen_shake_frames == 0U);
        } else {
            CHECK(s4.state_04 == 8U);
            CHECK(s4.anim_2a == 8U);
            CHECK((r4.sfx_emitted == std::vector<std::uint32_t>{kBossChargeSfxRoar, kBossChargeSfxQuake}));
            CHECK(r4.screen_shake_frames == 4U);
        }
    }

    // Spawner self / Ali guards ($FF19E8) suppress notification
    BossChargeEntityState self_ent{};
    self_ent.type_00 = 0x003A;
    self_ent.self_ram_addr = 0x00FF1CD8U;
    self_ent.spawner_ptr_aa = 0x00FF1CD8U;
    const auto self_res = boss_charge_despawn_and_notify(self_ent);
    CHECK(self_res.despawned);
    CHECK(!self_res.spawner_notified);

    BossChargeEntityState ali_ent{};
    ali_ent.type_00 = 0x003A;
    ali_ent.self_ram_addr = 0x00FF1CD8U;
    ali_ent.spawner_ptr_aa = kBossChargePlayerAliRamAddr;
    const auto ali_res = boss_charge_despawn_and_notify(ali_ent);
    CHECK(ali_res.despawned);
    CHECK(!ali_res.spawner_notified);
}

void test_ram_window_helpers_and_oob_guards() {
    std::array<std::uint8_t, 0x10000> ram{};
    constexpr std::uint32_t kBossAddr = 0x00FF1C1CU;
    constexpr std::uint32_t kParentAddr = 0x00FF1CD8U;

    // Camera Plane A ($FF1716) at (50, 40)
    write_be16(ram, kBossChargePlaneACamRamOffset + 0x00U, 50U);
    write_be16(ram, kBossChargePlaneACamRamOffset + 0x04U, 40U);

    // Subtype 2 boss in State 4 with completed action flag
    BossChargeEntityState ent{};
    ent.type_00 = 0x003A;
    ent.state_04 = 4U;
    ent.world_x_08 = 120;
    ent.world_y_0c = 120;
    ent.elev_z_10 = 0x00020000;
    ent.vel_x_4e = 0x00010000;
    ent.vel_y_52 = 0x00020000;
    ent.vel_z_56 = 0;
    ent.render_attr_18 = 0xF801U;
    ent.flags_37 = kBossChargeFlagAnimDoneBit;
    ent.subtype_a6 = 2U;
    ent.spawner_ptr_aa = kParentAddr;
    store_boss_charge_entity_to_ram(ram, kBossAddr, ent);

    const auto r4 = boss_charge_state4_step_from_ram(ram, kBossAddr);
    CHECK(r4.state_transitioned);
    CHECK(r4.screen_shake_frames == 4U);
    CHECK(read_be16(ram, kBossChargeScreenShakeRamOffset) == 4U);
    CHECK(read_be16(ram, 0x1C1CU + 0x04U) == 8U);
    CHECK(read_be16(ram, 0x1C1CU + 0x2AU) == 8U);
    CHECK(read_be32(ram, 0x1C1CU + 0x72U) == 0x00010000U);
    CHECK(read_be32(ram, 0x1C1CU + 0x76U) == 0x00020000U);

    // Damping step via RAM
    boss_charge_damping_step_from_ram(ram, kBossAddr);
    CHECK(read_be32(ram, 0x1C1CU + 0x4EU) == 0x0000C000U); // 0.75 * 0x10000

    // Despawn via aux water surface probe in RAM notifies spawner at $FF1CD8 (+0x06 = 0x0010)
    write_be32(ram, 0x1766U, 0x00FF5000U); // Plane A aux map at $FF5000
    write_be16(ram, 0x176EU, 5U);          // word_stride_shift = 5 (32 cols)
    ram[0x5000U + (static_cast<std::size_t>(120 >> 3) << 5U) + static_cast<std::size_t>(120 >> 3)] = 0x80U;
    write_be32(ram, 0x1C1CU + 0x10U, 0U); // On ground -> water probe triggers despawn
    const auto water_res = boss_charge_trajectory_step_from_ram(ram, kBossAddr);
    CHECK(water_res.despawned);
    CHECK(water_res.spawner_notified);
    CHECK(read_be16(ram, 0x1C1CU + 0x00U) == 0U);
    CHECK(read_be16(ram, 0x1CD8U + 0x06U) == 0x0010U);

    // Out-of-bounds entity address and OOB spawner pointer never crash or corrupt RAM
    CHECK(!boss_charge_state0_step_from_ram(ram, 0x00FFFF80U).state_transitioned);
    CHECK(!boss_charge_despawn_and_notify_from_ram(ram, 0x00FFFFF0U).despawned);
    ent.type_00 = 0x003A;
    ent.spawner_ptr_aa = 0x00FFFFFFU; // OOB for +0x06 write
    store_boss_charge_entity_to_ram(ram, kBossAddr, ent);
    const auto oob_spawner = boss_charge_despawn_and_notify_from_ram(ram, kBossAddr);
    CHECK(oob_spawner.despawned);
    CHECK(read_be16(ram, 0x1C1CU + 0x00U) == 0U);
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
    const auto& bytes = rom.bytes();
    CHECK(read_be32(bytes, 0x00EEDCU + 2U) == kBossChargeSfxRoar);
    CHECK(read_be32(bytes, 0x00EEE6U + 2U) == kBossChargeSfxQuake);
    CHECK(read_be32(bytes, 0x00EF00U + 2U) == kBossChargeSfxSwing);
    CHECK(read_be32(bytes, 0x00F00CU + 2U) == kBossChargeSfxBounce);
    CHECK(read_be32(bytes, 0x00EFBAU + 2U) == static_cast<std::uint32_t>(kBossChargeGravityAccel));
    CHECK(read_be32(bytes, 0x00F018U + 2U) == static_cast<std::uint32_t>(kBossChargeBounceRestThreshold));
    CHECK(read_be16(bytes, 0x00F048U + 2U) == kBossChargeSpawnerNotifyMsg);
    CHECK(read_be16(bytes, 0x00F052U) == 0x4E75U);
}

} // namespace

int main() {
    test_constants_and_damping_math();
    test_trajectory_bounce_water_and_culling();
    test_states_0_2_4_and_spawner_guards();
    test_ram_window_helpers_and_oob_guards();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.199 Boss Charge Engine tests succeeded!\n";
    return 0;
}
