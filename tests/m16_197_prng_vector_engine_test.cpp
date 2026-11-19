#include "game/physics/prng_vector_engine.hpp"
#include "../include/game/physics/prng_vector_engine.hpp"
#include "game/physics/aabb_force_fields.hpp"
#include "game/physics/projectile_spark_lifecycle.hpp"

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
        std::cerr << "[FAIL] M16.197 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
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

void test_constants_jump_table_and_cross_headers() {
    static_assert(kPrngVectorEngineStartAddr == 0x00E25CU);
    static_assert(kEntityLifecycleTimerCountdownAddr == 0x00E25CU);
    static_assert(kEntityLifecycleTimerCountdownSizeBytes == 12U);
    static_assert(kPrngStep16Addr == 0x00E268U);
    static_assert(kPrngStep16SizeBytes == 58U);
    static_assert(kPrngRangeWordAddr == 0x00E2A2U);
    static_assert(kPrngRangeWordSizeBytes == 24U);
    static_assert(kPrngRangeByteAddr == 0x00E2BAU);
    static_assert(kPrngRangeByteSizeBytes == 26U);
    static_assert(kProjectVelocity8WayAddr == 0x00E2D4U);
    static_assert(kProjectVelocity8WayJumpTableAddr == 0x00E2F2U);
    static_assert(kProjectVelocity8WaySizeBytes == 100U);
    static_assert(kResolveCardinalDirection4WayAddr == 0x00E338U);
    static_assert(kResolveCardinalDirection4WaySizeBytes == 54U);
    static_assert(kWaterSurfaceProbeAddr == 0x00E36EU);
    static_assert(kWaterSurfaceProbeSizeBytes == 42U);
    static_assert(kPrngVectorEngineEndAddr == 0x00E398U);
    static_assert(kPrngVectorEngineSizeBytes == 316U);
    static_assert(kPrngVectorEngineEndAddr - kPrngVectorEngineStartAddr ==
                  kPrngVectorEngineSizeBytes);
    static_assert(kEntityLifecycleTimerCountdownSizeBytes +
                  kPrngStep16SizeBytes +
                  kPrngRangeWordSizeBytes +
                  kPrngRangeByteSizeBytes +
                  kProjectVelocity8WaySizeBytes +
                  kResolveCardinalDirection4WaySizeBytes +
                  kWaterSurfaceProbeSizeBytes == kPrngVectorEngineSizeBytes);

    // PC-relative jump table targets at 0x00E2F2..0x00E302
    static_assert(resolve_velocity_8way_jump_target(0U) == 0x00E302U);
    static_assert(resolve_velocity_8way_jump_target(1U) == 0x00E308U);
    static_assert(resolve_velocity_8way_jump_target(2U) == 0x00E30CU);
    static_assert(resolve_velocity_8way_jump_target(3U) == 0x00E310U);
    static_assert(resolve_velocity_8way_jump_target(4U) == 0x00E316U);
    static_assert(resolve_velocity_8way_jump_target(5U) == 0x00E322U);
    static_assert(resolve_velocity_8way_jump_target(6U) == 0x00E31AU);
    static_assert(resolve_velocity_8way_jump_target(7U) == 0x00E31EU);
    static_assert(resolve_velocity_8way_jump_target(8U) == 0x00E302U);

    // Cross-namespace alias & co-inclusion compatibility with M16.194 and M16.196
    static_assert(oasis::game::physics::kPrngVectorEngineEndAddr == kPrngVectorEngineEndAddr);
    static_assert(sizeof(oasis::game::physics::Prng16State) == sizeof(Prng16State));
    static_assert(kProjectileSparkLifecycleEndAddr == kPrngVectorEngineStartAddr);
    static_assert(kAabbForceFieldsStartAddr == 0x00DDA2U);
}

void test_entity_lifecycle_timer_countdown() {
    LifecycleTimerEntity ent{0x1234, 3U};
    CHECK(!step_entity_lifecycle_timer(ent));
    CHECK(ent.timer == 2U && ent.status == 0x1234);

    CHECK(!step_entity_lifecycle_timer(ent));
    CHECK(ent.timer == 1U && ent.status == 0x1234);

    // Reaching 0 executes clr.w $0(a6) (clears full 16-bit word, not just high byte)
    CHECK(step_entity_lifecycle_timer(ent));
    CHECK(ent.timer == 0U && ent.status == 0);

    // Wrapping 0 -> 0xFFFF does NOT clear status (bne.b skips clr.w)
    ent.status = 0x0055;
    ent.timer = 0U;
    CHECK(!step_entity_lifecycle_timer(ent));
    CHECK(ent.timer == 0xFFFFU && ent.status == 0x0055);

    // Work RAM window helper ($FF19E8)
    std::array<std::uint8_t, 0x10000> ram{};
    write_be16(ram, 0x19E8U, 0x2233U);
    write_be16(ram, 0x19EAU, 1U);
    CHECK(step_entity_lifecycle_timer_from_ram(ram, 0x00FF19E8U));
    CHECK(read_be16(ram, 0x19E8U) == 0x0000U);
    CHECK(read_be16(ram, 0x19EAU) == 0x0000U);
    CHECK(!step_entity_lifecycle_timer_from_ram(ram, 0x00FF19E8U));
    CHECK(read_be16(ram, 0x19EAU) == 0xFFFFU);
    CHECK(!step_entity_lifecycle_timer_from_ram(ram, 0x00FFFFFEU));
}

void test_prng_16bit_lfsr_and_scaling() {
    // Compile-time + runtime verification of exact 4-round M68K LFSR sequences
    constexpr auto seq_7f28 = []() {
        Prng16State p{0x7F28U};
        return std::array<std::uint16_t, 4>{p.step(), p.step(), p.step(), p.step()};
    }();
    static_assert(seq_7f28 == std::array<std::uint16_t, 4>{0xF28AU, 0x28ABU, 0x8AB9U, 0xAB9EU});
    CHECK((seq_7f28 == std::array<std::uint16_t, 4>{0xF28AU, 0x28ABU, 0x8AB9U, 0xAB9EU}));

    Prng16State p1234{0x1234U};
    CHECK(p1234.step() == 0x2343U);
    CHECK(p1234.step() == 0x3438U);
    CHECK(p1234.step() == 0x4384U);
    CHECK(p1234.step() == 0x384FU);

    std::uint16_t seed0001 = 0x0001U;
    CHECK(prng_step_16(seed0001) == 0x001FU);
    CHECK(prng_step_16(seed0001) == 0x01FFU);
    CHECK(prng_step_16(seed0001) == 0x1FFFU);
    CHECK(prng_step_16(seed0001) == 0xFFFDU);

    // 0x8000 collapses to 0x0000 fixed point (taps at bit 14 and bit 0 only)
    std::uint16_t seed8000 = 0x8000U;
    CHECK(prng_step_16(seed8000) == 0x0000U);
    CHECK(prng_step_16(seed8000) == 0x0000U);

    // Limit == 0 (or masked byte == 0 for 0x00E2BA) returns 0 WITHOUT stepping PRNG
    std::uint16_t seed_guard = 0x7F28U;
    CHECK(prng_range_word(seed_guard, 0U) == 0U);
    CHECK(seed_guard == 0x7F28U);
    CHECK(prng_range_byte(seed_guard, 0U) == 0U);
    CHECK(seed_guard == 0x7F28U);
    CHECK(prng_range_byte(seed_guard, 0x0100U) == 0U);
    CHECK(seed_guard == 0x7F28U);

    // Scaled range_word & range_byte exact arithmetic: (step() * limit) >> 16
    // First step from 0x7F28 is 0xF28A: (0xF28A * 100) >> 16 = 94
    CHECK(prng_range_word(seed_guard, 100U) == static_cast<std::uint16_t>((0xF28AU * 100U) >> 16U));
    CHECK(seed_guard == 0xF28AU);
    // Next step is 0x28AB; limit 0x0110 masks to 0x10: (0x28AB * 16) >> 16 = 2
    CHECK(prng_range_byte(seed_guard, 0x0110U) == static_cast<std::uint8_t>((0x28ABU * 16U) >> 16U));
    CHECK(seed_guard == 0x28ABU);

    constexpr std::array<std::uint16_t, 7> kLimits{1U, 3U, 7U, 16U, 64U, 255U, 1000U};
    for (const std::uint16_t lim : kLimits) {
        for (int i = 0; i < 64; ++i) {
            const std::uint16_t v = prng_range_word(seed_guard, lim);
            CHECK(v < lim);
        }
    }

    // Work RAM $FF1928: mutates ONLY 16-bit word at 0x1928..0x1929 (leaving +2..+3 intact)
    std::array<std::uint8_t, 0x10000> ram{};
    write_be16(ram, kPrngSeedStateRamOffset, 0x7F28U);
    write_be16(ram, kPrngSeedStateRamOffset + 2U, 0xBEEFU);
    CHECK(prng_step_16_from_ram(ram) == 0xF28AU);
    CHECK(read_be16(ram, kPrngSeedStateRamOffset) == 0xF28AU);
    CHECK(read_be16(ram, kPrngSeedStateRamOffset + 2U) == 0xBEEFU);
    CHECK(prng_range_word_from_ram(ram, 10U) == static_cast<std::uint16_t>((0x28ABU * 10U) >> 16U));
    CHECK(prng_range_byte_from_ram(ram, 0x0320U) ==
          static_cast<std::uint8_t>((0x8AB9U * 0x20U) >> 16U));
    std::array<std::uint8_t, 16> small_ram{};
    CHECK(prng_step_16_from_ram(small_ram) == 0U);
}

void test_velocity_projection_8way() {
    // 0=N, 1=E, 2=S, 3=W, 4=NE, 5=SE, 6=SW, 7=NW; 8.8 u16 -> 16.16 s32 (<< 8)
    // Example: (0x0180, 0x0180) -> (0x00018000, 0x00018000); 0.75x = 0x00012000
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 0U) == VelocityFixed2D{0, -0x00018000});
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 1U) == VelocityFixed2D{0x00018000, 0});
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 2U) == VelocityFixed2D{0, 0x00018000});
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 3U) == VelocityFixed2D{-0x00018000, 0});
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 4U) ==
                  VelocityFixed2D{0x00012000, -0x00012000});
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 5U) ==
                  VelocityFixed2D{0x00012000, 0x00012000});
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 6U) ==
                  VelocityFixed2D{-0x00012000, 0x00012000});
    static_assert(project_velocity_8way(0x0180U, 0x0180U, 7U) ==
                  VelocityFixed2D{-0x00012000, -0x00012000});

    // Asymmetric speed_x/speed_y & direction & 7 wrapping (ROM 0x007504 uses 0x0800, 0x0780)
    const auto v_ne = project_velocity_8way(0x0800U, 0x0780U, 4U + 8U);
    CHECK(v_ne.vx == 0x00060000);  // 0x080000 * 0.75
    CHECK(v_ne.vy == -0x0005A000); // -0x078000 * 0.75

    // Unsigned 16-bit input check: 0x8000 -> +0x00800000 (swap; clr.w; lsr.l #8 zero-extends!)
    const auto v_u16 = project_velocity_8way(0x8000U, 0x0100U, 1U);
    CHECK(v_u16.vx == 0x00800000 && v_u16.vy == 0);

    // Exact arithmetic shift right for non-multiple-of-4 values: (-1 >> 1) + ((-1 >> 1) >> 1) = -2
    static_assert(scale_diagonal_three_quarters(-1) == -2);
    static_assert(scale_diagonal_three_quarters(0x00030000) == 0x00024000);
}

void test_cardinal_direction_resolver_4way() {
    // 0x00E338..0x00E36C returns 0=N, 1=E, 2=S, 3=W (NOT 0,2,4,6)
    static_assert(resolve_cardinal_direction_4way_index(100, 20) == 1U);   // East
    static_assert(resolve_cardinal_direction_4way_index(-100, 20) == 3U);  // West
    static_assert(resolve_cardinal_direction_4way_index(10, 50) == 2U);    // South
    static_assert(resolve_cardinal_direction_4way_index(10, -50) == 0U);   // North

    // Equal magnitudes: cmp.l d4, d3; bhi.w -> |dx| <= |dy| resolves to N/S
    static_assert(resolve_cardinal_direction_4way_index(0, 0) == 2U);
    static_assert(resolve_cardinal_direction_4way_index(25, 25) == 2U);
    static_assert(resolve_cardinal_direction_4way_index(-25, 25) == 2U);
    static_assert(resolve_cardinal_direction_4way_index(25, -25) == 0U);
    static_assert(resolve_cardinal_direction_4way_index(-25, -25) == 0U);

    // 32-bit INT32_MIN magnitude check (neg.l leaves 0x80000000; unsigned bhi compares cleanly)
    constexpr std::int32_t kMinI32 = std::numeric_limits<std::int32_t>::min();
    static_assert(resolve_cardinal_direction_4way_index(kMinI32, 100) == 3U);
    static_assert(resolve_cardinal_direction_4way_index(kMinI32, kMinI32) == 0U);
    static_assert(resolve_cardinal_direction_4way_index(kMinI32 + 1, kMinI32) == 0U);
    CHECK(resolve_cardinal_direction_4way(0x00020000, -0x00010000) == Direction8Way::East);
}

void test_water_surface_probe() {
    // 0x00E37A..0x00E37E: (attr & 0x90) == 0x80
    static_assert(is_water_surface_tile_attr(0x80U));
    static_assert(is_water_surface_tile_attr(0x8FU));
    static_assert(is_water_surface_tile_attr(0xE0U));
    static_assert(!is_water_surface_tile_attr(0x00U));
    static_assert(!is_water_surface_tile_attr(0x10U));
    static_assert(!is_water_surface_tile_attr(0x90U)); // Bit 4 blocks water immersion
    static_assert(!is_water_surface_tile_attr(0xF0U));

    WaterProbeEntity ent{32, 48, 0xA5U, 8, 8}; // Non-bit-3 bits (0xA5) must be preserved
    CHECK(probe_entity_water_surface(ent, 0x82U, false));
    CHECK(ent.flags_36 == 0xADU);

    // Standing on a floating platform suppresses water immersion and clears bit 3
    CHECK(!probe_entity_water_surface(ent, 0x82U, true));
    CHECK(ent.flags_36 == 0xA5U);

    // Full Work RAM probe: $FF16F0 bit 7 selects aux table, $FF2D8C checks 6 raft slots
    std::array<std::uint8_t, 0x10000> ram{};
    constexpr std::size_t kAuxOff = 0x4000U;
    constexpr std::uint32_t kEntityAddr = 0x00FF19E8U;
    ram[kWaterProbeScrollModeRamOffset] = 0x00U; // Plane A aux ($FF1766)
    write_be32(ram, kWaterProbeAuxPlaneARamOffset + 0x00U, 0x00FF4000U);
    write_be16(ram, kWaterProbeAuxPlaneARamOffset + 0x08U, 5U); // stride = 32 tiles

    // Tile (4, 6) at pixel (36, 52) -> attr 0x81 (water)
    ram[kAuxOff + (6U << 5U) + 4U] = 0x81U;
    write_be16(ram, 0x19E8U + kProbeEntityPosXOffset, 36U);
    write_be16(ram, 0x19E8U + kProbeEntityPosYOffset, 52U);
    ram[0x19E8U + kProbeEntityFlags36Offset] = 0x40U;
    write_be16(ram, 0x19E8U + kProbeEntityExtentXOffset, 8U);
    write_be16(ram, 0x19E8U + kProbeEntityExtentYOffset, 8U);

    CHECK(probe_entity_water_surface_from_ram(ram, kEntityAddr));
    CHECK(ram[0x19E8U + kProbeEntityFlags36Offset] == 0x48U);

    // Place overlapping type-0x0056 raft in slot 5 of $FF2D8C -> clears bit 3
    const std::size_t slot5 = kWaterProbeSparkPoolRamOffset + (5U * kWaterProbeSparkSlotStride);
    write_be16(ram, slot5 + 0x00U, static_cast<std::uint16_t>(kWaterProbeRaftTypeId));
    write_be16(ram, slot5 + 0x08U, 44U);
    write_be16(ram, slot5 + 0x0CU, 58U);
    write_be16(ram, slot5 + 0x42U, 6U);
    write_be16(ram, slot5 + 0x44U, 6U);
    CHECK(!probe_entity_water_surface_from_ram(ram, kEntityAddr));
    CHECK(ram[0x19E8U + kProbeEntityFlags36Offset] == 0x40U);

    // Switch to Plane B aux ($FF1770) via $FF16F0 bit 7 with non-water attr 0x90 (bit 4 set)
    write_be16(ram, slot5 + 0x00U, 0x0012U); // Not raft type 0x56
    ram[kWaterProbeScrollModeRamOffset] = kWaterProbePlaneBSelectBit;
    write_be32(ram, kWaterProbeAuxPlaneBRamOffset + 0x00U, 0x00FF5000U);
    write_be16(ram, kWaterProbeAuxPlaneBRamOffset + 0x08U, 6U);
    ram[0x5000U + (6U << 6U) + 4U] = 0x90U;
    ram[0x19E8U + kProbeEntityFlags36Offset] = 0x48U;
    CHECK(!probe_entity_water_surface_from_ram(ram, kEntityAddr));
    CHECK(ram[0x19E8U + kProbeEntityFlags36Offset] == 0x40U);

    // Negative coordinate / OOB guard clears bit 3 safely
    write_be16(ram, 0x19E8U + kProbeEntityPosXOffset, static_cast<std::uint16_t>(-8));
    ram[0x19E8U + kProbeEntityFlags36Offset] = 0x08U;
    CHECK(!probe_entity_water_surface_from_ram(ram, kEntityAddr));
    CHECK(ram[0x19E8U + kProbeEntityFlags36Offset] == 0x00U);
}

} // namespace

int main() {
    test_constants_jump_table_and_cross_headers();
    test_entity_lifecycle_timer_countdown();
    test_prng_16bit_lfsr_and_scaling();
    test_velocity_projection_8way();
    test_cardinal_direction_resolver_4way();
    test_water_surface_probe();

    std::cout << "[PASS] All M16.197 PRNG & vector engine tests succeeded!\n";
    return 0;
}
