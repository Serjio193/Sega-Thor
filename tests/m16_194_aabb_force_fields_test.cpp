#include "game/physics/aabb_force_fields.hpp"
#include "../include/game/physics/aabb_force_fields.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace {

using namespace thor::physics;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.194 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::int32_t read_be32_signed(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    const std::uint32_t raw =
        (static_cast<std::uint32_t>(data[offset]) << 24U) |
        (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
        (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
        static_cast<std::uint32_t>(data[offset + 3U]);
    return static_cast<std::int32_t>(raw);
}

void test_constants_and_aliases() {
    static_assert(kAabbForceFieldsStartAddr == 0x00DDA2U);
    static_assert(kApplyBoxForceVectorAddr == 0x00DDA2U);
    static_assert(kApplyBoxForceVectorEndAddr == 0x00DE00U);
    static_assert(kApplyBoxForceVectorSizeBytes == 94U);

    static_assert(kApplyBoxForceVectorElevationBoostAddr == 0x00DE00U);
    static_assert(kApplyBoxForceVectorElevationBoostEndAddr == 0x00DE6CU);
    static_assert(kApplyBoxForceVectorElevationBoostSizeBytes == 108U);

    static_assert(kApplyConveyorCurrentForceAddr == 0x00DE6CU);
    static_assert(kApplyConveyorCurrentForceEndAddr == 0x00DEECU);
    static_assert(kApplyConveyorCurrentForceSizeBytes == 128U);

    static_assert(kAabbForceFieldsEndAddr == 0x00DEECU);
    static_assert(kAabbForceFieldsSizeBytes == 330U);
    static_assert(kApplyBoxForceVectorSizeBytes +
                  kApplyBoxForceVectorElevationBoostSizeBytes +
                  kApplyConveyorCurrentForceSizeBytes == kAabbForceFieldsSizeBytes);

    static_assert(kEntityPoolBaseRamAddr == 0x00FF19E8U);
    static_assert(kEntityPoolBaseRamOffset == 0x19E8U);
    static_assert(kEntityPoolSlotCount == 21U);
    static_assert(kEntitySlotStrideBytes == 0xBCU);
    static_assert(kEntityPoolByteSize == 21U * 188U);
    static_assert(kEntityPoolBaseRamOffset + kEntityPoolByteSize == 0x2954U);

    static_assert(kEntityStatusOffset == 0x00U);
    static_assert(kEntityPosXOffset == 0x08U);
    static_assert(kEntityPosYOffset == 0x0CU);
    static_assert(kEntityPosZFpOffset == 0x10U);
    static_assert(kEntityFlags3AOffset == 0x3AU);
    static_assert(kEntityAccumDxFpOffset == 0x72U);
    static_assert(kEntityAccumDyFpOffset == 0x76U);

    static_assert(kEntityImmuneBit0Mask == 0x01U);
    static_assert(kEntityImmuneBit3Mask == 0x08U);
    static_assert(kEntityConveyorImmuneMask == 0x09U);
    static_assert(kFixedPointOne == 0x00010000);

    // Namespace alias check (oasis::game::physics == thor::physics).
    static_assert(oasis::game::physics::kApplyBoxForceVectorAddr == kApplyBoxForceVectorAddr);
    static_assert(sizeof(oasis::game::physics::ForceEntitySlot) == sizeof(ForceEntitySlot));
    static_assert(sizeof(oasis::game::physics::BoundingBox3D) == sizeof(BoundingBox3D));

    // Fixed-point 16.16 integer unpacking and packing.
    static_assert(pack_fixed_16_16(0) == 0);
    static_assert(pack_fixed_16_16(3, 0x8000U) == 0x00038000);
    static_assert(pack_fixed_16_16(-1, 0x8000U) == static_cast<std::int32_t>(0xFFFF8000U));
    static_assert(unpack_fixed_integer(0) == 0);
    static_assert(unpack_fixed_integer(0x00008000) == 0);
    static_assert(unpack_fixed_integer(0x00051234) == 5);
    static_assert(unpack_fixed_integer(static_cast<std::int32_t>(0xFFFF8000U)) == -1);
    static_assert(unpack_fixed_integer(static_cast<std::int32_t>(0xFFFB0000U)) == -5);

    // M68K register packing/unpacking against real ROM caller constants (e.g. 0x031C90..0x031C9C).
    constexpr BoundingBox3D conveyor_box = BoundingBox3D::from_registers(
        0x00C00068U, 0x025800D0U, 0x00000040U);
    static_assert(conveyor_box.min_x == 0x00C0);
    static_assert(conveyor_box.max_x == 0x0258);
    static_assert(conveyor_box.min_y == 0x0068);
    static_assert(conveyor_box.max_y == 0x00D0);
    static_assert(conveyor_box.min_z == 0);
    static_assert(conveyor_box.max_z == 0x0040);
    static_assert(conveyor_box.to_registers().d3 == 0x00C00068U);
    static_assert(conveyor_box.to_registers().d4 == 0x025800D0U);
    static_assert(conveyor_box.to_registers().d5 == 0x00000040U);

    constexpr BoundingBox3D signed_box{
        .min_x = -20, .max_x = -5,
        .min_y = -40, .max_y = -10,
        .min_z = -8, .max_z = -1,
    };
    constexpr PackedBoxRegisters packed_signed = signed_box.to_registers();
    static_assert(BoundingBox3D::from_registers(
        packed_signed.d3, packed_signed.d4, packed_signed.d5) == signed_box);
    static_assert(signed_box.contains(-20, -40, -8));
    static_assert(signed_box.contains(-5, -10, -1));
    static_assert(!signed_box.contains(-21, -40, -8));
    static_assert(!signed_box.contains(-4, -10, -1));

    CHECK(conveyor_box.contains(0x00C0, 0x0068, 0));
    CHECK(conveyor_box.contains(0x0258, 0x00D0, 0x0040));
    CHECK(!conveyor_box.contains(0x0259, 0x00D0, 0x0040));
}

void test_active_guard_and_aabb_boundaries() {
    const BoundingBox3D box{
        .min_x = 100, .max_x = 200,
        .min_y = 50, .max_y = 150,
        .min_z = 0, .max_z = 32,
    };

    std::vector<ForceEntitySlot> slots = {
        // 0: Inactive (status == 0) inside box -> skipped
        ForceEntitySlot{.status = 0, .pos_x = 150, .pos_y = 100, .pos_z_fp = 0},
        // 1: Negative status (-1) inside box -> skipped (`tst.w; ble`)
        ForceEntitySlot{.status = -1, .pos_x = 150, .pos_y = 100, .pos_z_fp = 0},
        // 2: Min signed int16 status (-32768) inside box -> skipped
        ForceEntitySlot{.status = -32768, .pos_x = 150, .pos_y = 100, .pos_z_fp = 0},
        // 3: Active at exact inclusive minimum corner (100, 50, 0) -> affected
        ForceEntitySlot{.status = 1, .pos_x = 100, .pos_y = 50, .pos_z_fp = 0},
        // 4: Active at exact inclusive maximum corner (200, 150, 32) -> affected
        ForceEntitySlot{.status = 0x0060, .pos_x = 200, .pos_y = 150, .pos_z_fp = pack_fixed_16_16(32)},
        // 5: Just outside min_x
        ForceEntitySlot{.status = 1, .pos_x = 99, .pos_y = 100, .pos_z_fp = 0},
        // 6: Just outside max_x
        ForceEntitySlot{.status = 1, .pos_x = 201, .pos_y = 100, .pos_z_fp = 0},
        // 7: Just outside min_y
        ForceEntitySlot{.status = 1, .pos_x = 150, .pos_y = 49, .pos_z_fp = 0},
        // 8: Just outside max_y
        ForceEntitySlot{.status = 1, .pos_x = 150, .pos_y = 151, .pos_z_fp = 0},
        // 9: Just outside min_z
        ForceEntitySlot{.status = 1, .pos_x = 150, .pos_y = 100, .pos_z_fp = pack_fixed_16_16(-1)},
        // 10: Just outside max_z
        ForceEntitySlot{.status = 1, .pos_x = 150, .pos_y = 100, .pos_z_fp = pack_fixed_16_16(33)},
        // 11: Active inside box with conveyor immunity bits (0x09) and airborne Z -> 0x00DDA2 ignores +$3A and gives 1x
        ForceEntitySlot{
            .status = 2,
            .pos_x = 150,
            .pos_y = 100,
            .pos_z_fp = pack_fixed_16_16(16),
            .flags_3a = 0x09U,
            .accum_dx_fp = 0x00010000,
            .accum_dy_fp = -0x00008000,
        },
    };

    const std::size_t affected = apply_box_force_vector(slots, box, 0x00030000, -0x00018000);
    CHECK(affected == 3U);

    for (std::size_t i : {0U, 1U, 2U, 5U, 6U, 7U, 8U, 9U, 10U}) {
        CHECK(slots[i].accum_dx_fp == 0);
        CHECK(slots[i].accum_dy_fp == 0);
    }

    CHECK(slots[3].accum_dx_fp == 0x00030000);
    CHECK(slots[3].accum_dy_fp == -0x00018000);
    CHECK(slots[4].accum_dx_fp == 0x00030000);
    CHECK(slots[4].accum_dy_fp == -0x00018000);
    // Entry 1 (0x00DDA2) applies 1x force regardless of non-zero pos_z_fp and flags_3a.
    CHECK(slots[11].accum_dx_fp == 0x00040000);
    CHECK(slots[11].accum_dy_fp == -0x00020000);

    // Inverted bounds (min > max) affect 0 slots.
    const BoundingBox3D inverted{.min_x = 200, .max_x = 100, .min_y = 50, .max_y = 150, .min_z = 0, .max_z = 32};
    CHECK(apply_box_force_vector(slots, inverted, 0x00010000, 0x00010000) == 0U);
}

void test_elevation_boost_and_subpixel_z_parity() {
    // Box with min_z == 0, max_z == 0 (ground plane in integer Z).
    const BoundingBox3D ground_box{
        .min_x = -50, .max_x = 50,
        .min_y = -50, .max_y = 50,
        .min_z = 0, .max_z = 0,
    };

    std::vector<ForceEntitySlot> slots = {
        // 0: z_fp == 0 -> passes [0, 0], 1x force (tst.l $10(a0) == 0)
        ForceEntitySlot{.status = 1, .pos_x = 0, .pos_y = 0, .pos_z_fp = 0},
        // 1: z_fp == 0x00008000 (integer Z = 0, fractional = 0.5) -> passes [0, 0] via move.w $10(a0),
        //    AND gets 2x boost via tst.l $10(a0) != 0!
        ForceEntitySlot{.status = 1, .pos_x = 0, .pos_y = 0, .pos_z_fp = 0x00008000, .flags_3a = 0x09U},
        // 2: z_fp == 0x00010000 (integer Z = 1) -> excluded from [0, 0]
        ForceEntitySlot{.status = 1, .pos_x = 0, .pos_y = 0, .pos_z_fp = pack_fixed_16_16(1)},
    };

    const ForceVector2D force{.force_x_fp = 0x00020000, .force_y_fp = -0x00004000};
    const std::size_t ground_affected =
        apply_box_force_vector_with_elevation_boost(slots, ground_box, force);
    CHECK(ground_affected == 2U);
    CHECK(slots[0].accum_dx_fp == 0x00020000);
    CHECK(slots[0].accum_dy_fp == -0x00004000);
    CHECK(slots[1].accum_dx_fp == 0x00040000);
    CHECK(slots[1].accum_dy_fp == -0x00008000);
    CHECK(slots[2].accum_dx_fp == 0);
    CHECK(slots[2].accum_dy_fp == 0);

    // Negative integer Z (-1.5 -> 0xFFFF8000) in [-5, 5] box gets 2x boost; wrapping 32-bit add.
    const BoundingBox3D neg_z_box{
        .min_x = -50, .max_x = 50,
        .min_y = -50, .max_y = 50,
        .min_z = -5, .max_z = 5,
    };
    std::vector<ForceEntitySlot> wrap_slots = {
        ForceEntitySlot{
            .status = 1,
            .pos_x = 10,
            .pos_y = -10,
            .pos_z_fp = pack_fixed_16_16(-1, 0x8000U),
            .accum_dx_fp = std::numeric_limits<std::int32_t>::max() - 0x0000FFFF,
            .accum_dy_fp = std::numeric_limits<std::int32_t>::min(),
        },
    };
    CHECK(apply_box_force_vector_with_elevation_boost(
        wrap_slots, neg_z_box, 0x00010000, -0x00010000) == 1U);
    const std::int32_t expected_x = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max() - 0x0000FFFF) +
        0x00020000U);
    const std::int32_t expected_y = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::min()) +
        static_cast<std::uint32_t>(-0x00020000));
    CHECK(wrap_slots[0].accum_dx_fp == expected_x);
    CHECK(wrap_slots[0].accum_dy_fp == expected_y);
}

void test_conveyor_immunity_and_21_slot_limit() {
    const BoundingBox3D box = BoundingBox3D::from_registers(
        0x00600038U, 0x019800A8U, 0x00000040U);

    std::vector<ForceEntitySlot> slots(24U);
    for (auto& slot : slots) {
        slot.status = 1;
        slot.pos_x = 0x0100;
        slot.pos_y = 0x0070;
        slot.pos_z_fp = 0;
        slot.flags_3a = 0U;
    }

    slots[0].pos_z_fp = 0;                          // Grounded, eligible -> 1x
    slots[1].pos_z_fp = pack_fixed_16_16(12);       // Airborne, eligible -> 2x
    slots[2].flags_3a = kEntityImmuneBit0Mask;      // Bit 0 set -> skipped
    slots[3].flags_3a = kEntityImmuneBit3Mask;      // Bit 3 set -> skipped
    slots[4].flags_3a = kEntityConveyorImmuneMask;  // Bits 0 & 3 set -> skipped
    slots[5].flags_3a = 0xF6U;                      // All other bits (1,2,4,5,6,7) -> NOT immune -> 1x
    slots[6].flags_3a = 0xF6U;
    slots[6].pos_z_fp = 0x00000001;                 // Minimal subpixel Z -> 2x
    slots[7].status = 0;                            // Inactive -> skipped

    const std::size_t affected = apply_conveyor_current_force(
        slots, box, -0x00020000, 0x00010000);
    // Slots 0..20 = 21 total minus 4 skipped (2,3,4,7) = 17 affected; slots 21..23 beyond pool cap.
    CHECK(affected == 17U);

    CHECK(slots[0].accum_dx_fp == -0x00020000);
    CHECK(slots[0].accum_dy_fp == 0x00010000);
    CHECK(slots[1].accum_dx_fp == -0x00040000);
    CHECK(slots[1].accum_dy_fp == 0x00020000);
    CHECK(slots[2].accum_dx_fp == 0);
    CHECK(slots[3].accum_dx_fp == 0);
    CHECK(slots[4].accum_dx_fp == 0);
    CHECK(slots[5].accum_dx_fp == -0x00020000);
    CHECK(slots[5].accum_dy_fp == 0x00010000);
    CHECK(slots[6].accum_dx_fp == -0x00040000);
    CHECK(slots[6].accum_dy_fp == 0x00020000);
    CHECK(slots[7].accum_dx_fp == 0);
    CHECK(slots[20].accum_dx_fp == -0x00020000);

    // Slots 21, 22, 23 must NOT be touched (ROM loop is dbra d0 from #$14 = 21 iterations).
    for (std::size_t i = 21U; i < slots.size(); ++i) {
        CHECK(slots[i].accum_dx_fp == 0);
        CHECK(slots[i].accum_dy_fp == 0);
    }
}

void test_ram_window_roundtrip_and_rom_callers() {
    std::array<std::uint8_t, 0x10000> ram{};
    ram.fill(0xA5U);

    // Slot 0 (Player Ali at $FF19E8): inside water flume & conveyor boxes, grounded
    store_force_entity_slot_to_ram(ram, 0U, ForceEntitySlot{
        .status = 1,
        .pos_x = 0x0010,
        .pos_y = 0x0090,
        .pos_z_fp = 0,
        .flags_3a = 0U,
        .accum_dx_fp = 0x00004000,
        .accum_dy_fp = 0,
    });

    // Slot 1 (Airborne enemy): inside Area 7 conveyor box with subpixel Z = 10.25
    store_force_entity_slot_to_ram(ram, 1U, ForceEntitySlot{
        .status = 0x002C,
        .pos_x = 0x0150,
        .pos_y = 0x00A0,
        .pos_z_fp = pack_fixed_16_16(10, 0x4000U),
        .flags_3a = 0x02U,
        .accum_dx_fp = 0,
        .accum_dy_fp = 0,
    });

    // Slot 2 (Spirit / bit-0 immune): inside Area 7 conveyor box
    store_force_entity_slot_to_ram(ram, 2U, ForceEntitySlot{
        .status = 0x0010,
        .pos_x = 0x0150,
        .pos_y = 0x00A0,
        .pos_z_fp = pack_fixed_16_16(8),
        .flags_3a = kEntityImmuneBit0Mask,
        .accum_dx_fp = 0,
        .accum_dy_fp = 0,
    });

    // Slot 20 (Last pool slot at $FF2898): inside Area 7 conveyor box, grounded
    store_force_entity_slot_to_ram(ram, 20U, ForceEntitySlot{
        .status = 3,
        .pos_x = 0x0200,
        .pos_y = 0x00B0,
        .pos_z_fp = 0,
        .flags_3a = 0U,
        .accum_dx_fp = 0,
        .accum_dy_fp = 0,
    });

    // Mark remaining slots 3..19 inactive so 0xA5 sentinel stays verifiable, and put an active decoy at slot 21 ($FF2954).
    for (std::size_t i = 3U; i < 20U; ++i) {
        const std::size_t off = kEntityPoolBaseRamOffset + i * kEntitySlotStrideBytes;
        ram[off] = 0x00U;
        ram[off + 1U] = 0x00U;
    }
    const std::size_t slot21_off = kEntityPoolBaseRamOffset + 21U * kEntitySlotStrideBytes;
    ram[slot21_off] = 0x00U;
    ram[slot21_off + 1U] = 0x01U;

    // Verify load roundtrip and preservation of untouched subpixel/metadata bytes.
    const ForceEntitySlot loaded0 = load_force_entity_slot_from_ram(ram, 0U);
    CHECK(loaded0.status == 1);
    CHECK(loaded0.pos_x == 0x0010);
    CHECK(loaded0.pos_y == 0x0090);
    CHECK(loaded0.pos_z_fp == 0);
    CHECK(loaded0.accum_dx_fp == 0x00004000);
    CHECK(ram[kEntityPoolBaseRamOffset + kEntityPosXOffset + 2U] == 0xA5U); // +0x0A fractional X untouched
    CHECK(ram[kEntityPoolBaseRamOffset + kEntityPosYOffset + 2U] == 0xA5U); // +0x0E fractional Y untouched
    CHECK(ram[kEntityPoolBaseRamOffset + 0x20U] == 0xA5U);

    // 1. ROM 0x03045A..0x030474 water flume: d1=0, d2=0x00030000, d3=0x00000078, d4=0x001F00C7, d5=0
    const std::size_t flume_hits = apply_force_field_packed_from_ram(
        ram, 0, 0x00030000, 0x00000078U, 0x001F00C7U, 0x00000000U, ForceFieldMode::Standard);
    CHECK(flume_hits == 1U);
    const ForceEntitySlot after_flume0 = load_force_entity_slot_from_ram(ram, 0U);
    CHECK(after_flume0.accum_dx_fp == 0x00004000);
    CHECK(after_flume0.accum_dy_fp == 0x00030000);

    // 2. ROM 0x031C88..0x031CA2 conveyor: d1=-0x20000, d2=0, d3=0x00C00068, d4=0x025800D0, d5=0x00000040
    const std::size_t conv_hits = apply_force_field_packed_from_ram(
        ram, -0x00020000, 0, 0x00C00068U, 0x025800D0U, 0x00000040U, ForceFieldMode::ConveyorCurrent);
    CHECK(conv_hits == 2U); // Slots 1 (2x) and 20 (1x); slot 2 skipped by bit 0
    CHECK(read_be32_signed(ram, kEntityPoolBaseRamOffset + kEntitySlotStrideBytes + kEntityAccumDxFpOffset) ==
          -0x00040000);
    CHECK(read_be32_signed(ram, kEntityPoolBaseRamOffset + 2U * kEntitySlotStrideBytes + kEntityAccumDxFpOffset) == 0);
    CHECK(read_be32_signed(ram, kEntityPoolBaseRamOffset + 20U * kEntitySlotStrideBytes + kEntityAccumDxFpOffset) ==
          -0x00020000);

    // 3. Named RAM wrappers + ElevationBoost mode check
    const BoundingBox3D all_box{.min_x = -1000, .max_x = 1000, .min_y = -1000, .max_y = 1000, .min_z = -100, .max_z = 100};
    CHECK(apply_box_force_vector_from_ram(ram, all_box, 0x00010000, 0) == 4U);
    CHECK(apply_box_force_vector_with_elevation_boost_from_ram(ram, all_box, 0, 0x00010000) == 4U);
    CHECK(apply_conveyor_current_force_from_ram(ram, all_box, 0x00008000, 0) == 3U);

    // Slot 0 (z=0): dx = 0x4000 + 0x10000 + 0x8000 = 0x1C000; dy = 0x30000 + 0x10000 = 0x40000
    const ForceEntitySlot final0 = load_force_entity_slot_from_ram(ram, 0U);
    CHECK(final0.accum_dx_fp == 0x0001C000);
    CHECK(final0.accum_dy_fp == 0x00040000);
    // Slot 2 (immune to conveyor, but affected by Standard and ElevationBoost with z!=0 -> 2x):
    const ForceEntitySlot final2 = load_force_entity_slot_from_ram(ram, 2U);
    CHECK(final2.accum_dx_fp == 0x00010000);
    CHECK(final2.accum_dy_fp == 0x00020000);

    // Untouched RAM sentinels outside +$72/+$76 and beyond slot 20 must remain 0xA5.
    CHECK(ram[kEntityPoolBaseRamOffset - 1U] == 0xA5U);
    CHECK(ram[kEntityPoolBaseRamOffset + kEntityPosXOffset + 2U] == 0xA5U);
    CHECK(ram[slot21_off + kEntityAccumDxFpOffset] == 0xA5U);

    // Out-of-bounds guards
    std::array<std::uint8_t, 128> tiny_ram{};
    CHECK(apply_box_force_vector_from_ram(tiny_ram, all_box, 1, 1) == 0U);
    CHECK(load_force_entity_slot_from_ram(ram, kEntityPoolSlotCount) == ForceEntitySlot{});
    store_force_entity_slot_to_ram(ram, kEntityPoolSlotCount, final0);
    CHECK(ram[slot21_off] == 0x00U && ram[slot21_off + 1U] == 0x01U);
}

} // namespace

int main() {
    test_constants_and_aliases();
    test_active_guard_and_aabb_boundaries();
    test_elevation_boost_and_subpixel_z_parity();
    test_conveyor_immunity_and_21_slot_limit();
    test_ram_window_roundtrip_and_rom_callers();

    std::cout << "[PASS] All M16.194 AABB force fields tests succeeded!\n";
    return 0;
}
