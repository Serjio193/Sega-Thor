#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "game/combat/hit_spark_generator.hpp"

using namespace oasis::game::combat;

namespace {

void test_constants() {
    static_assert(HitSparkGeneratorConfig::kHitSparkGeneratorAddr == 0x00C012U);
    static_assert(HitSparkGeneratorConfig::kHitSparkInitAddr == 0x00C038U);
    static_assert(HitSparkGeneratorConfig::kExitRtsAddr == 0x00C0BCU);
    static_assert(HitSparkGeneratorConfig::kRangeEndAddr == 0x00C0BEU);
    static_assert(HitSparkGeneratorConfig::kRoutineSizeBytes == 172U);

    static_assert(HitSparkGeneratorConfig::kUnblockedHitCallerAddr == 0x00BEFEU);
    static_assert(HitSparkGeneratorConfig::kBlockedHitCallerAddr == 0x00BFF6U);

    static_assert(HitSparkGeneratorConfig::kSparkRingBufferRam == 0x00FF19C8U);
    static_assert(HitSparkGeneratorConfig::kSlotCount == 4U);
    static_assert(HitSparkGeneratorConfig::kSlotSizeBytes == 8U);
    static_assert(HitSparkGeneratorConfig::kSparkType == 2U);
    static_assert(HitSparkGeneratorConfig::kDefaultLifetime == 5U);

    static_assert(HitSparkGeneratorConfig::kSparkTypeOffset == 0x00U);
    static_assert(HitSparkGeneratorConfig::kSparkXOffset == 0x02U);
    static_assert(HitSparkGeneratorConfig::kSparkYOffset == 0x04U);
    static_assert(HitSparkGeneratorConfig::kSparkLifetimeOffset == 0x06U);

    static_assert(HitSparkGeneratorConfig::kTargetWorldXOffset == 0x08U);
    static_assert(HitSparkGeneratorConfig::kTargetWorldYOffset == 0x0CU);
    static_assert(HitSparkGeneratorConfig::kTargetElevZOffset == 0x10U);
    static_assert(HitSparkGeneratorConfig::kTargetGroundHOffset == 0x14U);
    static_assert(HitSparkGeneratorConfig::kTargetRadXOffset == 0x42U);
    static_assert(HitSparkGeneratorConfig::kTargetRadYOffset == 0x44U);
    static_assert(HitSparkGeneratorConfig::kTargetHeightZOffset == 0x4AU);

    std::cout << "[PASS] test_constants\n";
}

void test_symmetric_midpoint() {
    const AttackHitbox3D hitbox{
        .z_min = 20,
        .z_max = 40,
        .x_min = 100,
        .y_min = 200,
        .x_max = 140,
        .y_max = 240
    };

    const SparkTargetState target{
        .address = 0x00FF1CD8U,
        .world_x = 120,
        .world_y = 220,
        .elev_z = 10,
        .ground_h = 15,
        .rad_x = 20,
        .rad_y = 20,
        .height_z = 25
    };

    const SparkMidpointResult result = compute_spark_midpoints(hitbox, target);

    // Z: tz_min = 25, tz_max = 50. z0 = max(20, 25)=25, z1 = min(40, 50)=40. mid_z = 25 + (15 >> 1) = 32
    assert(result.mid_z == 32);

    // X: tx_min = 100, tx_max = 140. x0 = max(100, 100)=100, x1 = min(140, 140)=140. mid_x = 100 + (40 >> 1) = 120
    assert(result.mid_x == 120);

    // Y: ty_base = 235, ty_min = 215, ty_max = 255. y0 = max(200, 215)=215, y1 = min(240, 255)=240. mid_y = 215 + (25 >> 1) = 227
    assert(result.mid_y == 227);

    // Projected: spark_x = mid_x, spark_y = mid_y - mid_z
    assert(result.projected_x == 120);
    assert(result.projected_y == 227 - 32); // 195

    std::cout << "[PASS] test_symmetric_midpoint\n";
}

void test_negative_coordinates() {
    const AttackHitbox3D hitbox{
        .z_min = -10,
        .z_max = 10,
        .x_min = -60,
        .y_min = -90,
        .x_max = -40,
        .y_max = -70
    };

    const SparkTargetState target{
        .address = 0x00FF19E8U,
        .world_x = -50,
        .world_y = -80,
        .elev_z = 0,
        .ground_h = -5,
        .rad_x = 16,
        .rad_y = 16,
        .height_z = 20
    };

    const SparkMidpointResult result = compute_spark_midpoints(hitbox, target);

    // Z: tz_min = -5, tz_max = 15. z0 = max(-10, -5)=-5, z1 = min(10, 15)=10. mid_z = -5 + (15 >> 1) = 2
    assert(result.mid_z == 2);

    // X: tx_min = -66, tx_max = -34. x0 = max(-60, -66)=-60, x1 = min(-40, -34)=-40. mid_x = -60 + (20 >> 1) = -50
    assert(result.mid_x == -50);

    // Y: ty_base = -85, ty_min = -101, ty_max = -69. y0 = max(-90, -101)=-90, y1 = min(-70, -69)=-70. mid_y = -90 + (20 >> 1) = -80
    assert(result.mid_y == -80);

    assert(result.projected_x == -50);
    assert(result.projected_y == -80 - 2); // -82

    std::cout << "[PASS] test_negative_coordinates\n";
}

void test_ring_buffer_allocation() {
    std::array<HitSparkParticle, 4> slots{};

    const AttackHitbox3D hitbox{
        .z_min = 0, .z_max = 20,
        .x_min = 10, .y_min = 10,
        .x_max = 30, .y_max = 30
    };

    const SparkTargetState target{
        .address = 0x00FF1CD8U,
        .world_x = 20, .world_y = 20,
        .elev_z = 0, .ground_h = 0,
        .rad_x = 10, .rad_y = 10,
        .height_z = 20
    };

    // Slot 0 should be claimed first
    auto res0 = spawn_hit_spark(hitbox, target, slots);
    assert(res0.spawned);
    assert(res0.slot_index == 0U);
    assert(slots[0].type == 2U);
    assert(slots[0].lifetime == 5U);
    assert(slots[1].type == 0U);

    // Slot 1 should be claimed next
    auto res1 = spawn_hit_spark(hitbox, target, slots);
    assert(res1.spawned);
    assert(res1.slot_index == 1U);
    assert(slots[1].type == 2U);

    // Slot 2 should be claimed next
    auto res2 = spawn_hit_spark(hitbox, target, slots);
    assert(res2.spawned);
    assert(res2.slot_index == 2U);
    assert(slots[2].type == 2U);

    // Slot 3 should be claimed next
    auto res3 = spawn_hit_spark(hitbox, target, slots);
    assert(res3.spawned);
    assert(res3.slot_index == 3U);
    assert(slots[3].type == 2U);

    // Buffer is now full (all 4 slots occupied) -> should fail to spawn and drop particle
    auto res4 = spawn_hit_spark(hitbox, target, slots);
    assert(!res4.spawned);
    assert(res4.slot_index == 4U);

    std::cout << "[PASS] test_ring_buffer_allocation\n";
}

void test_lifetime_and_expiry() {
    std::array<HitSparkParticle, 4> slots{};

    const AttackHitbox3D hitbox{
        .z_min = 0, .z_max = 20,
        .x_min = 0, .y_min = 0,
        .x_max = 20, .y_max = 20
    };
    const SparkTargetState target{
        .address = 0x00FF1CD8U,
        .world_x = 10, .world_y = 10,
        .elev_z = 0, .ground_h = 0,
        .rad_x = 10, .rad_y = 10,
        .height_z = 20
    };

    auto res = spawn_hit_spark(hitbox, target, slots);
    assert(res.spawned);
    assert(slots[0].lifetime == 5U);
    assert(slots[0].is_active());

    // Step 4 frames
    for (int f = 4; f >= 1; --f) {
        update_hit_sparks(slots);
        assert(slots[0].lifetime == static_cast<std::uint16_t>(f));
        assert(slots[0].type == 2U);
        assert(slots[0].is_active());
    }

    // 5th frame step -> expires and frees slot
    update_hit_sparks(slots);
    assert(slots[0].lifetime == 0U);
    assert(slots[0].type == 0U);
    assert(!slots[0].is_active());

    // Slot 0 is now free again and can be re-claimed
    auto res_new = spawn_hit_spark(hitbox, target, slots);
    assert(res_new.spawned);
    assert(res_new.slot_index == 0U);
    assert(slots[0].lifetime == 5U);

    std::cout << "[PASS] test_lifetime_and_expiry\n";
}

void test_manager_lifecycle() {
    HitSparkManager mgr;

    const AttackHitbox3D hitbox{
        .z_min = 10, .z_max = 30,
        .x_min = 50, .y_min = 50,
        .x_max = 70, .y_max = 70
    };
    const SparkTargetState target{
        .address = 0x00FF1CD8U,
        .world_x = 60, .world_y = 60,
        .elev_z = 5, .ground_h = 5,
        .rad_x = 10, .rad_y = 10,
        .height_z = 20
    };

    auto res = mgr.spawn_spark(hitbox, target);
    assert(res.spawned);
    assert(res.slot_index == 0U);
    assert(mgr.sparks()[0].type == 2U);

    mgr.update_frame();
    assert(mgr.sparks()[0].lifetime == 4U);

    mgr.reset();
    assert(mgr.sparks()[0].type == 0U);
    assert(mgr.sparks()[0].lifetime == 0U);

    std::cout << "[PASS] test_manager_lifecycle\n";
}

void test_raw_memory_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00);

    const std::uint32_t target_addr = 0x00FF1CD8U;
    const std::size_t target_off = static_cast<std::size_t>(target_addr - 0x00FF0000U);

    // Setup target entity in raw RAM
    // world_x at +0x08 = 200
    ram[target_off + 0x08] = 0x00;
    ram[target_off + 0x09] = 200;
    // world_y at +0x0C = 300 (0x012C)
    ram[target_off + 0x0C] = 0x01;
    ram[target_off + 0x0D] = 0x2C;
    // elev_z at +0x10 = 10
    ram[target_off + 0x10] = 0x00;
    ram[target_off + 0x11] = 10;
    // ground_h at +0x14 = 20
    ram[target_off + 0x14] = 0x00;
    ram[target_off + 0x15] = 20;
    // rad_x at +0x42 = 15
    ram[target_off + 0x42] = 0x00;
    ram[target_off + 0x43] = 15;
    // rad_y at +0x44 = 15
    ram[target_off + 0x44] = 0x00;
    ram[target_off + 0x45] = 15;
    // height_z at +0x4A = 30
    ram[target_off + 0x4A] = 0x00;
    ram[target_off + 0x4B] = 30;

    // Prefill slot 0 as active in ring buffer ($FF19C8)
    const std::size_t ring_off = static_cast<std::size_t>(0x00FF19C8U - 0x00FF0000U);
    ram[ring_off + 0] = 0x00;
    ram[ring_off + 1] = 0x02; // type = 2 (busy)

    const AttackHitbox3D hitbox{
        .z_min = 25,
        .z_max = 55,
        .x_min = 190,
        .y_min = 310,
        .x_max = 220,
        .y_max = 340
    };

    // Slot 0 busy -> should allocate into slot 1 ($FF19D0)
    auto res = spawn_hit_spark_raw(ram, hitbox, target_addr);
    assert(res.spawned);
    assert(res.slot_index == 1U);

    // Read back particle from slot 1 in raw RAM
    const HitSparkParticle p1 = read_hit_spark_particle_raw(ram, 1U);
    assert(p1.type == 2U);
    assert(p1.lifetime == 5U);
    assert(p1.x == res.midpoint.projected_x);
    assert(p1.y == res.midpoint.projected_y);

    // Verify slot 0 in raw RAM is unchanged
    const HitSparkParticle p0 = read_hit_spark_particle_raw(ram, 0U);
    assert(p0.type == 2U);

    std::cout << "[PASS] test_raw_memory_integration\n";
}

} // namespace

int main() {
    test_constants();
    test_symmetric_midpoint();
    test_negative_coordinates();
    test_ring_buffer_allocation();
    test_lifetime_and_expiry();
    test_manager_lifecycle();
    test_raw_memory_integration();

    std::cout << "\nAll M16.173 Hit Spark Generator tests passed successfully!\n";
    return 0;
}
