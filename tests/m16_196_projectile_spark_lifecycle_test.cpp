#include "game/physics/projectile_spark_lifecycle.hpp"
#include "game/physics/aabb_force_fields.hpp"
#include "../include/game/physics/projectile_spark_lifecycle.hpp"

#include "core/rom.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using namespace thor::physics;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.196 check failed at line " << line << ": " << expr << '\n';
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

[[nodiscard]] std::int16_t read_be16_signed(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return static_cast<std::int16_t>(read_be16(data, offset));
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

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) {
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void test_constants_and_jump_table() {
    static_assert(kProjectileSparkLifecycleStartAddr == 0x00E0BAU);
    static_assert(kDispatchSparkEffectsSizeBytes == 58U);
    static_assert(kDispatchProjectileEntitiesSizeBytes == 76U);
    static_assert(kProjectileGravityLandingSizeBytes == 66U);
    static_assert(kProjectileGravityBlinkDespawnSizeBytes == 120U);
    static_assert(kProjectileBouncePhaseInitSizeBytes == 82U);
    static_assert(kProjectileSparkLifecycleEndAddr == 0x00E25CU);
    static_assert(kProjectileSparkLifecycleSizeBytes == 418U);
    static_assert(kDispatchSparkEffectsSizeBytes +
                  kDispatchProjectileEntitiesSizeBytes +
                  4U + 12U +
                  kProjectileGravityLandingSizeBytes +
                  kProjectileGravityBlinkDespawnSizeBytes +
                  kProjectileBouncePhaseInitSizeBytes == kProjectileSparkLifecycleSizeBytes);

    static_assert(kProjectilePoolBaseRamOffset + kProjectilePoolByteSize == kSparkPoolBaseRamOffset);
    static_assert(kSparkPoolBaseRamOffset + kSparkPoolByteSize == 0x2FA8U);
    static_assert(kProjectileGravityStepFp == 0x0000C000);
    static_assert(kProjectileBlinkStartFrame == 240);
    static_assert(kProjectileBlinkEndFrame == 300);

    static_assert(kProjectileStateJumpTableWords[0] == 0x0010U);
    static_assert(kProjectileStateJumpTableWords[1] == 0x0050U);
    static_assert(resolve_projectile_jump_target(0) == kProjectileGravityLandingAddr);
    static_assert(resolve_projectile_jump_target(2) == kProjectileGravityBlinkDespawnAddr);
    static_assert(resolve_projectile_jump_target(1) == 0U);
    static_assert(resolve_projectile_jump_target(4) == 0U);
    static_assert(resolve_projectile_jump_target(-2) == 0U);

    static_assert(oasis::game::physics::kDispatchSparkEffectsAddr == kDispatchSparkEffectsAddr);
    static_assert(sizeof(oasis::game::physics::ProjectileLifecycleSlot) == sizeof(ProjectileLifecycleSlot));
}

void test_dispatch_spark_effects() {
    std::array<SparkEffectSlot, 7> sparks{};
    sparks[0] = {1, 0x00012340U, kSparkPoolBaseRamAddr};
    sparks[1] = {0, 0x00012340U, kSparkPoolBaseRamAddr + 0x5AU};  // inactive (type == 0)
    sparks[2] = {-3, 0x00012400U, kSparkPoolBaseRamAddr + 0xB4U}; // negative type != 0 runs!
    sparks[3] = {5, 0x00000000U, kSparkPoolBaseRamAddr + 0x10EU}; // null callback skipped
    sparks[5] = {2, 0x00012500U, kSparkPoolBaseRamAddr + 0x1C2U};
    sparks[6] = {9, 0x00012600U, kSparkPoolBaseRamAddr + 0x21CU}; // 7th slot beyond capacity 6

    std::vector<std::size_t> visited;
    // Bit 1 set -> paused -> 0 callbacks invoked
    CHECK(dispatch_spark_effects(sparks, 0x02U, [&](std::size_t idx, SparkEffectSlot&) {
              visited.push_back(idx);
          }) == 0U);
    CHECK(visited.empty());

    // Bit 0 only (bit 1 clear) -> dispatches active slots 0, 2, 5
    const std::size_t count = dispatch_spark_effects(
        sparks, 0x01U, [&](std::size_t idx, SparkEffectSlot& s) {
            visited.push_back(idx);
            CHECK(s.callback_addr != 0U);
        });
    CHECK(count == 3U);
    CHECK((visited == std::vector<std::size_t>{0U, 2U, 5U}));
}

void test_projectile_gravity_landing_and_parent_unlatch() {
    // 1. Grounded projectile (elevation == 0) does NOT decrement velocity_z
    ProjectileLifecycleSlot grounded{};
    grounded.type = 4;
    grounded.elevation_fp = 0;
    grounded.velocity_z_fp = 0x00010000;
    grounded.slot_ram_addr = kProjectilePoolBaseRamAddr;
    CHECK(!step_projectile_gravity_landing(grounded));
    CHECK(grounded.velocity_z_fp == 0x00010000);
    CHECK(grounded.elevation_fp == 0);
    CHECK(grounded.type == 4); // >= $FF2ABC stays active

    // 2. Airborne $FF2ABC projectile integrates gravity & clamps negative elevation to 0
    ProjectileLifecycleSlot proj{};
    proj.type = 7;
    proj.elevation_fp = 0x00010000;  // +1.0 px
    proj.velocity_z_fp = 0x00004000; // +0.25 px/f
    proj.slot_ram_addr = kProjectilePoolBaseRamAddr + 0x5AU;

    // Frame 1: vel = 0x4000 - 0xC000 = -0x8000 (-0.5); elev = 0x10000 - 0x8000 = 0x8000 (+0.5)
    CHECK(!step_projectile_gravity_landing(proj));
    CHECK(proj.velocity_z_fp == -0x00008000);
    CHECK(proj.elevation_fp == 0x00008000);

    // Frame 2: vel = -0x8000 - 0xC000 = -0x14000 (-1.25); elev = 0x8000 - 0x14000 < 0 -> clamp 0
    CHECK(!step_projectile_gravity_landing(proj));
    CHECK(proj.velocity_z_fp == -0x00014000);
    CHECK(proj.elevation_fp == 0);
    CHECK(proj.type == 7); // >= $FF2ABC does not clear type

    // 3. Pre-$FF2ABC entity (< $FF2ABC) unlatches parent and frees slot on landing
    ParentEntityTarget parent{};
    parent.pos_y = 100;
    parent.flags_36 = 0x87U; // bit 1 set alongside bits 0, 2, 7

    ProjectileLifecycleSlot carried{};
    carried.type = 12;
    carried.pos_y = 248;
    carried.elevation_fp = 0x00002000;
    carried.velocity_z_fp = 0;
    carried.slot_ram_addr = kEntityPoolBaseRamAddr; // $FF19E8 < $FF2ABC
    carried.parent = &parent;

    CHECK(step_projectile_gravity_landing(carried));
    CHECK(carried.elevation_fp == 0);
    CHECK(carried.type == 0);
    CHECK(parent.flags_36 == 0x85U); // Only bit 1 cleared
    CHECK(parent.pos_y == 248);
}

void test_projectile_gravity_blink_and_despawn() {
    ProjectileLifecycleSlot drop{};
    drop.type = 3;
    drop.state = 2;
    drop.elevation_fp = 0;
    drop.lifetime = 238;
    drop.flags_36 = 0x40U;
    drop.slot_ram_addr = kProjectilePoolBaseRamAddr;

    // Frame 238 -> 239 (< 240): bit 1 untouched
    CHECK(!step_projectile_gravity_blink_despawn(drop));
    CHECK(drop.lifetime == 239);
    CHECK(drop.flags_36 == 0x40U);

    // Frame 239 -> 240 (even): clears bit 1
    drop.flags_36 = 0x42U;
    CHECK(!step_projectile_gravity_blink_despawn(drop));
    CHECK(drop.lifetime == 240);
    CHECK(drop.flags_36 == 0x40U);

    // Frame 240 -> 241 (odd): sets bit 1
    CHECK(!step_projectile_gravity_blink_despawn(drop));
    CHECK(drop.lifetime == 241);
    CHECK(drop.flags_36 == 0x42U);

    // Advance to 300 (even): clears bit 1, stays alive
    drop.lifetime = 299;
    CHECK(!step_projectile_gravity_blink_despawn(drop));
    CHECK(drop.lifetime == 300);
    CHECK(drop.flags_36 == 0x40U);
    CHECK(drop.type == 3);

    // Frame 300 -> > 300: clears type = 0 (despawns) and leaves lifetime at 300!
    CHECK(step_projectile_gravity_blink_despawn(drop));
    CHECK(drop.type == 0);
    CHECK(drop.lifetime == 300);

    // < $FF2ABC entity in state 2 falls through after clearing type = 0 into lifetime step!
    ParentEntityTarget parent{50, 0x03U};
    ProjectileLifecycleSlot fallthrough{};
    fallthrough.type = 5;
    fallthrough.state = 2;
    fallthrough.pos_y = 140;
    fallthrough.elevation_fp = 0x00001000;
    fallthrough.velocity_z_fp = 0;
    fallthrough.lifetime = 240;
    fallthrough.flags_36 = 0x00U;
    fallthrough.slot_ram_addr = kEntityPoolBaseRamAddr + 0xBCU;
    fallthrough.parent = &parent;

    CHECK(step_projectile_gravity_blink_despawn(fallthrough));
    CHECK(fallthrough.type == 0);
    CHECK(parent.flags_36 == 0x01U);
    CHECK(parent.pos_y == 140);
    CHECK(fallthrough.lifetime == 241);      // fell through 0x00E1CE -> 0x00E1D2!
    CHECK(fallthrough.flags_36 == 0x02U);    // odd frame 241 set bit 1!
}

void test_dispatch_projectile_entities_and_bounce() {
    std::array<ProjectileLifecycleSlot, 9> pool{};
    pool[0].type = 0;  // inactive
    pool[1].type = -1; // <= 0 skipped (ble)
    pool[2].type = 2;
    pool[2].state = 0;
    pool[2].elevation_fp = 0x00020000;
    pool[2].velocity_z_fp = 0x00010000;
    pool[3].type = 3;
    pool[3].state = 2;
    pool[3].lifetime = 240;
    pool[4].type = 4;
    pool[4].state = 6; // invalid jump table offset ignored safely
    pool[8].type = 5;  // 9th slot beyond capacity 8

    std::uint16_t timer = 3U;
    // Paused (bit 1 set): timer still decrements, 0 slots dispatched
    CHECK(dispatch_projectile_entities(pool, timer, 0x02U) == 0U);
    CHECK(timer == 2U);
    CHECK(pool[2].elevation_fp == 0x00020000);

    // Unpaused: timer decrements again, slots 2 and 3 dispatched
    CHECK(dispatch_projectile_entities(pool, timer, 0x00U) == 2U);
    CHECK(timer == 1U);
    CHECK(pool[2].velocity_z_fp == 0x00004000);
    CHECK(pool[2].elevation_fp == 0x00024000);
    CHECK(pool[3].lifetime == 241);
    CHECK(pool[3].flags_36 == 0x02U);
    CHECK(pool[8].elevation_fp == 0);

    // Bounce phase state machine (0x00E20A..0x00E25A)
    BouncePhaseEntitySlot b{};
    b.elevation_fp = 0x0001ABCD; // fractional low 16 bits 0xABCD must survive
    b.field_16 = 0x1234U;
    b.field_2a = 4U;
    b.bounce_phase_9e = 0U;

    CHECK(step_projectile_bounce_phase(b));
    CHECK(b.field_16 == 0U);
    CHECK(b.elevation_fp == 0x0008ABCD);
    CHECK(b.bounce_phase_9e == 0x63U);

    // Phase 0x63 is a no-op (does not auto-decrement +$9E!)
    CHECK(!step_projectile_bounce_phase(b));
    CHECK(b.bounce_phase_9e == 0x63U);

    // Phase 1 configures bounce sprite & 24px extents
    b.bounce_phase_9e = 1U;
    b.field_16 = 0x0099U;
    CHECK(step_projectile_bounce_phase(b));
    CHECK(b.field_16 == 0U);
    CHECK(b.field_2a == 6U);
    CHECK(b.elevation_fp == 0x000CABCD);
    CHECK(b.tile_attr_3d == 0x18U);
    CHECK(b.sprite_id_4a == 0x0038U);
    CHECK(b.radius_46 == 0x0018U);
    CHECK(b.extent_x_48 == 0x18U);
    CHECK(b.extent_y_49 == 0x18U);
    CHECK(b.bounce_phase_9e == 2U);
    CHECK(!step_projectile_bounce_phase(b));
}

void test_64kb_ram_window_roundtrip() {
    std::array<std::uint8_t, 0x10000> ram{};

    // Sparks at $FF2D8C (slots 0 and 4)
    store_spark_slot_to_ram(ram, 0U, {1, 0x00018000U, kSparkPoolBaseRamAddr});
    store_spark_slot_to_ram(ram, 4U, {-2, 0x00018120U, kSparkPoolBaseRamAddr + 4U * kLightSlotStrideBytes});
    ram[kProjectilePauseFlagsRamOffset] = 0x00U;

    std::vector<std::uint32_t> fired_cbs;
    CHECK(dispatch_spark_effects_from_ram(
              ram, [&](std::size_t, std::uint32_t, std::uint32_t cb, std::span<std::uint8_t>) {
                  fired_cbs.push_back(cb);
              }) == 2U);
    CHECK((fired_cbs == std::vector<std::uint32_t>{0x00018000U, 0x00018120U}));

    // Projectiles at $FF2ABC (slot 1 in state 2 near despawn) & entity at $FF19E8 (state 0 with parent at $FF1AA4)
    write_be16(ram, kProjectileGlobalTimerRamOffset, 5U);
    ProjectileLifecycleSlot p1{};
    p1.type = 2;
    p1.state = 2;
    p1.elevation_fp = 0x00008000;
    p1.velocity_z_fp = -0x00008000;
    p1.lifetime = 300;
    store_projectile_slot_to_ram(ram, 1U, p1);
    CHECK(dispatch_projectile_entities_from_ram(ram) == 1U);
    CHECK(read_be16(ram, kProjectileGlobalTimerRamOffset) == 4U);
    const auto p1_after = load_projectile_slot_from_ram(ram, 1U);
    CHECK(p1_after.elevation_fp == 0);
    CHECK(p1_after.type == 0);      // despawned at > 300
    CHECK(p1_after.lifetime == 300);

    // Pre-$FF2ABC entity landing unlatch in RAM
    const std::uint32_t child_addr = 0x00FF19E8U;
    const std::uint32_t parent_addr = 0x00FF1AA4U;
    write_be16(ram, 0x19E8U + kSlotTypeOffset, 9U);
    write_be16(ram, 0x19E8U + kSlotPosYWordOffset, 312U);
    write_be32(ram, 0x19E8U + kSlotElevationFpOffset, 0x00001000U);
    write_be32(ram, 0x19E8U + kSlotVelocityZFpOffset, 0U);
    write_be32(ram, 0x19E8U + kSlotParentPtrOffset, parent_addr);
    ram[0x1AA4U + kSlotFlags36Offset] = 0x07U;
    write_be16(ram, 0x1AA4U + kSlotPosYWordOffset, 100U);

    CHECK(step_projectile_gravity_landing_from_ram(ram, child_addr));
    CHECK(read_be16_signed(ram, 0x19E8U + kSlotTypeOffset) == 0);
    CHECK(ram[0x1AA4U + kSlotFlags36Offset] == 0x05U);
    CHECK(read_be16(ram, 0x1AA4U + kSlotPosYWordOffset) == 312U);

    // Bounce phase in RAM ($FF1B60)
    const std::uint32_t bounce_addr = 0x00FF1B60U;
    write_be32(ram, 0x1B60U + kSlotElevationFpOffset, 0x00012222U);
    ram[0x1B60U + kSlotBouncePhaseOffset] = 0U;
    CHECK(step_projectile_bounce_phase_from_ram(ram, bounce_addr));
    CHECK(read_be32(ram, 0x1B60U + kSlotElevationFpOffset) == 0x00082222U);
    CHECK(ram[0x1B60U + kSlotBouncePhaseOffset] == 0x63U);

    ram[0x1B60U + kSlotBouncePhaseOffset] = 1U;
    CHECK(step_projectile_bounce_phase_from_ram(ram, bounce_addr));
    CHECK(read_be32(ram, 0x1B60U + kSlotElevationFpOffset) == 0x000C2222U);
    CHECK(read_be16(ram, 0x1B60U + kSlotSpriteIdOffset) == 0x0038U);
    CHECK(ram[0x1B60U + kSlotExtentXOffset] == 0x18U);
    CHECK(ram[0x1B60U + kSlotExtentYOffset] == 0x18U);
    CHECK(ram[0x1B60U + kSlotBouncePhaseOffset] == 2U);

    // Out-of-bounds / invalid WRAM address guards
    CHECK(!step_projectile_gravity_landing_from_ram(ram, 0x00123456U));
    CHECK(!step_projectile_bounce_phase_from_ram(ram, 0x00FFFFE0U));
    std::array<std::uint8_t, 64> tiny_ram{};
    CHECK(dispatch_spark_effects_from_ram(tiny_ram) == 0U);
    CHECK(dispatch_projectile_entities_from_ram(tiny_ram) == 0U);
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
    const auto& bytes = rom.bytes();
    CHECK(read_be16(bytes, kProjectileStateJumpTableAddr + 0U) == kProjectileStateJumpTableWords[0]);
    CHECK(read_be16(bytes, kProjectileStateJumpTableAddr + 2U) == kProjectileStateJumpTableWords[1]);
    CHECK(read_be32_signed(bytes, 0x00E15CU) == kProjectileGravityStepFp);
    CHECK(read_be16_signed(bytes, 0x00E1DAU) == kProjectileBlinkStartFrame);
    CHECK(read_be16_signed(bytes, 0x00E1E2U) == kProjectileBlinkEndFrame);
    CHECK(read_be32(bytes, 0x00DFFCU) == kDispatchSingleProjectileSlotAddr);
    CHECK(read_be32(bytes, 0x00DF68U) == kProjectileBouncePhaseInitAddr);
}

} // namespace

int main() {
    test_constants_and_jump_table();
    test_dispatch_spark_effects();
    test_projectile_gravity_landing_and_parent_unlatch();
    test_projectile_gravity_blink_and_despawn();
    test_dispatch_projectile_entities_and_bounce();
    test_64kb_ram_window_roundtrip();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.196 projectile & spark lifecycle tests succeeded!\n";
    return 0;
}
