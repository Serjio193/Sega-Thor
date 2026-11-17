#include "game/combat/resumable_hitbox_iter.hpp"

#include <cassert>
#include <vector>

using namespace oasis::game::combat;

static void test_rom_constants() {
    static_assert(ResumableHitboxConfig::kFindFirst3dPointHitAddr == 0x00B852U);
    static_assert(ResumableHitboxConfig::kFindNext3dPointHitAddr == 0x00B912U);
    static_assert(ResumableHitboxConfig::kFindFirst3dBoxHitAddr == 0x00B922U);

    static_assert(ResumableHitboxConfig::kEntityPoolRam == 0x00FF19E8U);
    static_assert(ResumableHitboxConfig::kEntitySlotCount == 21U);
    static_assert(ResumableHitboxConfig::kEntityStrideBytes == 0x00BCU);

    static_assert(ResumableHitboxConfig::kIterSaveRamStart == 0x00FF194AU);
    static_assert(ResumableHitboxConfig::kIterSaveRamEnd == 0x00FF196AU);
    static_assert(ResumableHitboxConfig::kIterSaveBytes == 32U);
}

static void test_evidence_reference_matching() {
    std::vector<HitboxEntity> entities = {
        {.state = 1, .pos_x = 100, .pos_y = 100, .pos_z = 0, .elevation = 0, .half_width_x = 10, .half_width_y = 10, .height_z = 20},
        {.state = 1, .pos_x = 105, .pos_y = 105, .pos_z = 5, .elevation = 0, .half_width_x = 10, .half_width_y = 10, .height_z = 20},
        {.state = 0, .pos_x = 105, .pos_y = 105, .pos_z = 5, .elevation = 0, .half_width_x = 10, .half_width_y = 10, .height_z = 20}, // Inactive
        {.state = 1, .pos_x = 105, .pos_y = 105, .pos_z = 5, .elevation = 0, .half_width_x = 10, .half_width_y = 10, .height_z = 20},
    };

    // Probe (98, 98, 112, 112, 10) with mask 0b1110 (slots 1, 2, 3), self 99
    auto hits = collect_all_3d_point_hits(0b1110U, 99U, 98, 98, 112, 112, 10, entities);
    assert(hits.size() == 2U);
    assert(hits[0] == 1U);
    assert(hits[1] == 3U);
}

static void test_resumable_iteration_step_by_step() {
    std::vector<HitboxEntity> entities(21);
    for (std::size_t i = 0; i < 21; ++i) {
        entities[i] = {
            .state = 1,
            .pos_x = 100,
            .pos_y = 100,
            .pos_z = 0,
            .elevation = 0,
            .half_width_x = 10,
            .half_width_y = 10,
            .height_z = 20
        };
    }

    HitboxIteratorState state{};
    // Mask with bits 1, 5, 8 set (0x0122)
    const std::uint32_t mask = (1U << 1) | (1U << 5) | (1U << 8);

    auto res1 = find_first_3d_point_hit_abs(mask, 99U, 95, 95, 105, 105, 5, entities, state);
    assert(res1.hit);
    assert(res1.entity_index == 1U);
    assert(res1.ram_addr == ResumableHitboxConfig::kEntityPoolRam + 1U * 0xBCU);

    auto res2 = find_next_3d_hit(entities, state);
    assert(res2.hit);
    assert(res2.entity_index == 5U);
    assert(res2.ram_addr == ResumableHitboxConfig::kEntityPoolRam + 5U * 0xBCU);

    auto res3 = find_next_3d_hit(entities, state);
    assert(res3.hit);
    assert(res3.entity_index == 8U);
    assert(res3.ram_addr == ResumableHitboxConfig::kEntityPoolRam + 8U * 0xBCU);

    auto res4 = find_next_3d_hit(entities, state);
    assert(!res4.hit);
    assert(!state.active);

    // Calling again returns no hit safely
    auto res5 = find_next_3d_hit(entities, state);
    assert(!res5.hit);
}

static void test_self_exclusion_and_inactive_state() {
    std::vector<HitboxEntity> entities = {
        {.state = 1, .pos_x = 100, .pos_y = 100, .pos_z = 0, .elevation = 0, .half_width_x = 10, .half_width_y = 10, .height_z = 20}, // Slot 0
        {.state = 1, .pos_x = 100, .pos_y = 100, .pos_z = 0, .elevation = 0, .half_width_x = 10, .half_width_y = 10, .height_z = 20}, // Slot 1
    };

    // Self index 0 skips slot 0 even when mask bit 0 is set
    HitboxIteratorState state{};
    auto res = find_first_3d_point_hit_abs(0b11U, 0U, 95, 95, 105, 105, 5, entities, state);
    assert(res.hit);
    assert(res.entity_index == 1U);

    // Negative state (< 0) skipped
    entities[1].state = -1;
    HitboxIteratorState state2{};
    auto res2 = find_first_3d_point_hit_abs(0b11U, 0U, 95, 95, 105, 105, 5, entities, state2);
    assert(!res2.hit);
}

static void test_relative_caller_offsets_and_elevation() {
    HitboxEntity caller{
        .state = 1,
        .pos_x = 200,
        .pos_y = 300,
        .pos_z = 10,
        .elevation = 4, // Y and Z elevation bias
        .half_width_x = 8,
        .half_width_y = 8,
        .height_z = 24
    };

    // Target at x=210, y=308, z=14 -> rel: dx=+10, dy=+4, dz=0
    std::vector<HitboxEntity> entities = {
        {.state = 1, .pos_x = 210, .pos_y = 304, .pos_z = 10, .elevation = 4, .half_width_x = 4, .half_width_y = 4, .height_z = 10}
    };

    PointProbe3D rel_probe{
        .x1 = 5,
        .y1 = -5,
        .x2 = 15,
        .y2 = 10,
        .z = 2 // absolute pz = 10 + 4 + 2 = 16
    };

    HitboxIteratorState state{};
    auto res = find_first_3d_point_hit(0b1U, 99U, caller, rel_probe, entities, state);
    assert(res.hit);
    assert(res.entity_index == 0U);
}

static void test_3d_box_z_interval_probe() {
    std::vector<HitboxEntity> entities = {
        // Entity Z interval: pos_z 10 + elevation 5 = 15; height 20 -> [15, 35]
        {.state = 1, .pos_x = 50, .pos_y = 50, .pos_z = 10, .elevation = 5, .half_width_x = 10, .half_width_y = 10, .height_z = 20}
    };

    HitboxIteratorState state{};

    // Probe 1: Z interval [0, 14] -> strictly below 15 (miss)
    auto res1 = find_first_3d_box_hit_abs(0b1U, 99U, 45, 45, 55, 55, 0, 14, entities, state);
    assert(!res1.hit);

    // Probe 2: Z interval [14, 20] -> overlaps [15, 35] (hit!)
    auto res2 = find_first_3d_box_hit_abs(0b1U, 99U, 45, 45, 55, 55, 14, 20, entities, state);
    assert(res2.hit);
    assert(res2.entity_index == 0U);

    // Probe 3: Z interval [36, 50] -> strictly above 35 (miss)
    auto res3 = find_first_3d_box_hit_abs(0b1U, 99U, 45, 45, 55, 55, 36, 50, entities, state);
    assert(!res3.hit);
}

int main() {
    test_rom_constants();
    test_evidence_reference_matching();
    test_resumable_iteration_step_by_step();
    test_self_exclusion_and_inactive_state();
    test_relative_caller_offsets_and_elevation();
    test_3d_box_z_interval_probe();
    return 0;
}
