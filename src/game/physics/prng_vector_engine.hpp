#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace thor::physics {

// ROM entrypoints and boundaries (0x00E25C..0x00E398, 316 bytes, 122 instructions).
inline constexpr std::uint32_t kPrngVectorEngineStartAddr = 0x00E25CU;
inline constexpr std::uint32_t kEntityLifecycleTimerCountdownAddr = 0x00E25CU;
inline constexpr std::uint32_t kEntityLifecycleTimerCountdownEndAddr = 0x00E268U;
inline constexpr std::size_t kEntityLifecycleTimerCountdownSizeBytes = 12U;

inline constexpr std::uint32_t kPrngStep16Addr = 0x00E268U;
inline constexpr std::uint32_t kPrngStep16EndAddr = 0x00E2A2U;
inline constexpr std::size_t kPrngStep16SizeBytes = 58U;

inline constexpr std::uint32_t kPrngRangeWordAddr = 0x00E2A2U;
inline constexpr std::uint32_t kPrngRangeWordEndAddr = 0x00E2BAU;
inline constexpr std::size_t kPrngRangeWordSizeBytes = 24U;

inline constexpr std::uint32_t kPrngRangeByteAddr = 0x00E2BAU;
inline constexpr std::uint32_t kPrngRangeByteEndAddr = 0x00E2D4U;
inline constexpr std::size_t kPrngRangeByteSizeBytes = 26U;

inline constexpr std::uint32_t kProjectVelocity8WayAddr = 0x00E2D4U;
inline constexpr std::uint32_t kProjectVelocity8WayJumpTableAddr = 0x00E2F2U;
inline constexpr std::array<std::uint16_t, 8> kProjectVelocity8WayJumpTableWords = {
    0x0010U, 0x0014U, 0x0016U, 0x0018U, 0x001CU, 0x0026U, 0x001CU, 0x001EU};
inline constexpr std::uint32_t kProjectVelocity8WayEndAddr = 0x00E338U;
inline constexpr std::size_t kProjectVelocity8WaySizeBytes = 100U;

inline constexpr std::uint32_t kResolveCardinalDirection4WayAddr = 0x00E338U;
inline constexpr std::uint32_t kResolveCardinalDirection4WayEndAddr = 0x00E36EU;
inline constexpr std::size_t kResolveCardinalDirection4WaySizeBytes = 54U;

inline constexpr std::uint32_t kWaterSurfaceProbeAddr = 0x00E36EU;
inline constexpr std::uint32_t kWaterSurfaceProbeEndAddr = 0x00E398U;
inline constexpr std::size_t kWaterSurfaceProbeSizeBytes = 42U;

inline constexpr std::uint32_t kPrngVectorEngineEndAddr = 0x00E398U;
inline constexpr std::size_t kPrngVectorEngineSizeBytes = 316U;

// 16-bit PRNG state ($FF1928) and LFSR constants.
inline constexpr std::uint32_t kPrngSeedStateRamAddr = 0x00FF1928U;
inline constexpr std::size_t kPrngSeedStateRamOffset = 0x1928U;
inline constexpr std::uint16_t kPrngInitialSeed16 = 0x7F28U;
inline constexpr std::uint32_t kPrngLfsrRoundsPerStep = 4U;

// 8-way direction mask and water-probe RAM / tile attribute constants (0x00E36E / 0x009C40 / 0x009C7C).
inline constexpr std::uint8_t kDirectionMask8Way = 0x07U;
inline constexpr std::size_t kWaterProbeScrollModeRamOffset = 0x16F0U;
inline constexpr std::uint8_t kWaterProbePlaneBSelectBit = 0x80U;
inline constexpr std::size_t kWaterProbeAuxPlaneARamOffset = 0x1766U;
inline constexpr std::size_t kWaterProbeAuxPlaneBRamOffset = 0x1770U;
inline constexpr std::size_t kWaterProbeSparkPoolRamOffset = 0x2D8CU;
inline constexpr std::size_t kWaterProbeSparkPoolCount = 6U;
inline constexpr std::size_t kWaterProbeSparkSlotStride = 0x5AU;
inline constexpr std::int16_t kWaterProbeRaftTypeId = 0x0056;
inline constexpr std::uint8_t kWaterProbeTileAttrMask = 0x90U;
inline constexpr std::uint8_t kWaterProbeTileAttrWater = 0x80U;
inline constexpr std::uint8_t kWaterProbeFlags36WaterBit = 0x08U;

// Entity slot field offsets accessed by 0x00E25C and 0x00E36E / 0x009C40 / 0x009C7C.
inline constexpr std::size_t kTimerEntityStatusOffset = 0x00U;
inline constexpr std::size_t kTimerEntityCounterOffset = 0x02U;
inline constexpr std::size_t kProbeEntityPosXOffset = 0x08U;
inline constexpr std::size_t kProbeEntityPosYOffset = 0x0CU;
inline constexpr std::size_t kProbeEntityFlags36Offset = 0x36U;
inline constexpr std::size_t kProbeEntityExtentXOffset = 0x42U;
inline constexpr std::size_t kProbeEntityExtentYOffset = 0x44U;

/// 8-way direction indices matching ROM jump table 0x00E2F2 (0..3 cardinal, 4..7 diagonal).
enum class Direction8Way : std::uint8_t {
    North = 0U,     // 0: (0, -sy)
    East = 1U,      // 1: (+sx, 0)
    South = 2U,     // 2: (0, +sy)
    West = 3U,      // 3: (-sx, 0)
    NorthEast = 4U, // 4: (+0.75*sx, -0.75*sy)
    SouthEast = 5U, // 5: (+0.75*sx, +0.75*sy)
    SouthWest = 6U, // 6: (-0.75*sx, +0.75*sy)
    NorthWest = 7U, // 7: (-0.75*sx, -0.75*sy)
};

/// Entity lifecycle timer fields at +$00(a6) and +$02(a6) mutated by 0x00E25C..0x00E266.
struct LifecycleTimerEntity {
    std::int16_t status{0};  // +$00.w (cleared to 0 when timer reaches 0)
    std::uint16_t timer{0U}; // +$02.w (decremented by 1 via subq.w #1)

    [[nodiscard]] constexpr bool operator==(const LifecycleTimerEntity&) const noexcept = default;
};

/// 16-bit left-shifting XOR-tap LFSR state matching $FF1928 and 0x00E268..0x00E2D4.
struct Prng16State {
    std::uint16_t seed{kPrngInitialSeed16};

    /// 0x00E268..0x00E2A0: Advances LFSR by 4 rounds: tap = ((d0 >> 14) ^ d0) & 1; d0 = (d0 << 1) | tap.
    [[nodiscard]] constexpr std::uint16_t step() noexcept {
        std::uint16_t d0 = seed;
        for (std::uint32_t round = 0U; round < kPrngLfsrRoundsPerStep; ++round) {
            const auto rol2 = static_cast<std::uint16_t>((d0 << 2U) | (d0 >> 14U));
            const auto tap = static_cast<std::uint16_t>((rol2 ^ d0) & 1U);
            d0 = static_cast<std::uint16_t>((d0 << 1U) | tap);
        }
        seed = d0;
        return d0;
    }

    /// 0x00E2A2..0x00E2B8: Returns 0 if limit == 0 (without stepping), else (step() * limit) >> 16.
    [[nodiscard]] constexpr std::uint16_t range_word(std::uint16_t limit) noexcept {
        if (limit == 0U) {
            return 0U;
        }
        const std::uint32_t r = step();
        return static_cast<std::uint16_t>((r * static_cast<std::uint32_t>(limit)) >> 16U);
    }

    /// 0x00E2BA..0x00E2D2: Masks limit &= 0xFF; returns 0 if 0 (without stepping), else scaled byte.
    [[nodiscard]] constexpr std::uint8_t range_byte(std::uint16_t limit) noexcept {
        const auto masked = static_cast<std::uint16_t>(limit & 0x00FFU);
        if (masked == 0U) {
            return 0U;
        }
        return static_cast<std::uint8_t>(range_word(masked));
    }

    [[nodiscard]] constexpr bool operator==(const Prng16State&) const noexcept = default;
};

/// Signed 32-bit 16.16 fixed-point velocity pair returned in M68K d1.l / d2.l by 0x00E2D4.
struct VelocityFixed2D {
    std::int32_t vx{0};
    std::int32_t vy{0};

    [[nodiscard]] constexpr bool operator==(const VelocityFixed2D&) const noexcept = default;
};

/// Entity position, extents, and +$36 flags used by 0x00E36E / 0x009C40 / 0x009C7C.
struct WaterProbeEntity {
    std::int16_t pos_x{0};       // +$08.w integer X
    std::int16_t pos_y{0};       // +$0C.w integer Y
    std::uint8_t flags_36{0U};   // +$36.b (bit 3: water surface immersion)
    std::int16_t extent_x_42{0}; // +$42.w horizontal half-extent
    std::int16_t extent_y_44{0}; // +$44.w vertical half-extent

    [[nodiscard]] constexpr bool operator==(const WaterProbeEntity&) const noexcept = default;
};

/// Projection of a 90-byte slot in $FF2D8C checked by 0x009C7C for floating rafts (type == 0x0056).
struct WaterPlatformSlot {
    std::int16_t type{0};        // +$00.w (0x0056 = floating raft/platform)
    std::int16_t pos_x{0};       // +$08.w
    std::int16_t pos_y{0};       // +$0C.w
    std::int16_t extent_x_42{0}; // +$42.w
    std::int16_t extent_y_44{0}; // +$44.w

    [[nodiscard]] constexpr bool operator==(const WaterPlatformSlot&) const noexcept = default;
};

/// Resolves PC-relative jump table target at 0x00E2F2 for direction & 7.
[[nodiscard]] constexpr std::uint32_t resolve_velocity_8way_jump_target(
    std::uint16_t direction) noexcept
{
    const std::size_t idx = static_cast<std::size_t>(direction & kDirectionMask8Way);
    const std::uint32_t entry_addr =
        kProjectVelocity8WayJumpTableAddr + static_cast<std::uint32_t>(idx * 2U);
    return entry_addr + static_cast<std::uint32_t>(kProjectVelocity8WayJumpTableWords[idx]);
}

/// 0x00E322..0x00E330: Exact 0.75x fixed-point scaling via (v >> 1) + ((v >> 1) >> 1).
[[nodiscard]] constexpr std::int32_t scale_diagonal_three_quarters(
    std::int32_t value_fp) noexcept
{
    const std::int32_t half = value_fp >> 1;
    const std::int32_t quarter = half >> 1;
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(half) + static_cast<std::uint32_t>(quarter));
}

/// 0x00E2D4..0x00E336: Converts u16 8.8 magnitudes to 16.16 fixed-point (<< 8) and projects
/// along 8-way direction & 7 (0=N, 1=E, 2=S, 3=W, 4=NE, 5=SE, 6=SW, 7=NW) with 0.75x diagonals.
[[nodiscard]] constexpr VelocityFixed2D project_velocity_8way(
    std::uint16_t speed_x_8_8,
    std::uint16_t speed_y_8_8,
    std::uint16_t direction) noexcept
{
    const std::int32_t sx = static_cast<std::int32_t>(static_cast<std::uint32_t>(speed_x_8_8) << 8U);
    const std::int32_t sy = static_cast<std::int32_t>(static_cast<std::uint32_t>(speed_y_8_8) << 8U);

    switch (static_cast<Direction8Way>(direction & kDirectionMask8Way)) {
    case Direction8Way::North:
        return {0, -sy};
    case Direction8Way::East:
        return {sx, 0};
    case Direction8Way::South:
        return {0, sy};
    case Direction8Way::West:
        return {-sx, 0};
    case Direction8Way::NorthEast:
        return {scale_diagonal_three_quarters(sx), scale_diagonal_three_quarters(-sy)};
    case Direction8Way::SouthEast:
        return {scale_diagonal_three_quarters(sx), scale_diagonal_three_quarters(sy)};
    case Direction8Way::SouthWest:
        return {scale_diagonal_three_quarters(-sx), scale_diagonal_three_quarters(sy)};
    case Direction8Way::NorthWest:
        return {scale_diagonal_three_quarters(-sx), scale_diagonal_three_quarters(-sy)};
    }
    return {};
}

/// 0x00E338..0x00E36C: Resolves 32-bit (dx, dy) to 4-way cardinal direction 0..3
/// (0=North, 1=East, 2=South, 3=West) using unsigned |dx| > |dy|.
[[nodiscard]] constexpr Direction8Way resolve_cardinal_direction_4way(
    std::int32_t dx,
    std::int32_t dy) noexcept
{
    const std::uint32_t abs_x = (dx < 0)
        ? (0U - static_cast<std::uint32_t>(dx))
        : static_cast<std::uint32_t>(dx);
    const std::uint32_t abs_y = (dy < 0)
        ? (0U - static_cast<std::uint32_t>(dy))
        : static_cast<std::uint32_t>(dy);

    if (abs_x > abs_y) {
        return (dx < 0) ? Direction8Way::West : Direction8Way::East;
    }
    return (dy < 0) ? Direction8Way::North : Direction8Way::South;
}

/// Raw integer direction index (0..3) returned in M68K d3.w by 0x00E338..0x00E36C.
[[nodiscard]] constexpr std::uint8_t resolve_cardinal_direction_4way_index(
    std::int32_t dx,
    std::int32_t dy) noexcept
{
    return static_cast<std::uint8_t>(resolve_cardinal_direction_4way(dx, dy));
}

/// 0x00E37A..0x00E37E: Checks `(tile_attr & 0x90) == 0x80` (bit 7 set, bit 4 clear).
[[nodiscard]] constexpr bool is_water_surface_tile_attr(std::uint8_t tile_attr) noexcept {
    return (tile_attr & kWaterProbeTileAttrMask) == kWaterProbeTileAttrWater;
}

/// 0x00E25C..0x00E266: Decrements entity.timer (+$02); clears entity.status (+$00) to 0 when 0.
bool step_entity_lifecycle_timer(LifecycleTimerEntity& entity) noexcept;

/// 0x00E268..0x00E2A0: Steps 16-bit LFSR seed in-place by 4 rounds and returns new seed.
std::uint16_t prng_step_16(std::uint16_t& seed) noexcept;

/// 0x00E2A2..0x00E2B8: Returns 0 if limit == 0, else (prng_step_16(seed) * limit) >> 16.
std::uint16_t prng_range_word(std::uint16_t& seed, std::uint16_t limit) noexcept;

/// 0x00E2BA..0x00E2D2: Masks limit &= 0xFF; returns 0 if 0, else scaled byte in [0..(limit&0xFF)-1].
std::uint8_t prng_range_byte(std::uint16_t& seed, std::uint16_t limit) noexcept;

/// 0x009C7C..0x009CFE: Checks whether entity AABB overlaps an active type-0x56 raft slot in $FF2D8C.
[[nodiscard]] bool is_entity_on_water_platform(
    const WaterProbeEntity& entity,
    std::span<const WaterPlatformSlot> spark_slots) noexcept;

/// 0x00E36E..0x00E396: Updates bit 3 (0x08) of entity.flags_36 from tile_attr & platform overlap.
bool probe_entity_water_surface(
    WaterProbeEntity& entity,
    std::uint8_t tile_attr,
    bool on_floating_platform = false) noexcept;

/// 0x00E36E + 0x009C40 + 0x009C7C: Looks up aux tile at (pos_x >> 3, pos_y >> 3) and probes water.
bool probe_water_surface_with_aux(
    WaterProbeEntity& entity,
    std::span<const std::uint8_t> aux_map,
    std::uint16_t word_stride_shift,
    std::span<const WaterPlatformSlot> spark_slots = {}) noexcept;

// ============================================================================
// 64KB Work RAM Window Helpers ($FF0000..$FFFFFF)
// ============================================================================

/// 0x00E25C against Work RAM: decrements +$02(a6) and clears +$00(a6) on zero.
bool step_entity_lifecycle_timer_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept;

/// 0x00E268 against Work RAM: reads/steps/stores 16-bit seed at $FF1928.
std::uint16_t prng_step_16_from_ram(std::span<std::uint8_t> ram) noexcept;

/// 0x00E2A2 against Work RAM: scaled 16-bit PRNG using seed at $FF1928.
std::uint16_t prng_range_word_from_ram(
    std::span<std::uint8_t> ram,
    std::uint16_t limit) noexcept;

/// 0x00E2BA against Work RAM: scaled 8-bit PRNG using seed at $FF1928.
std::uint8_t prng_range_byte_from_ram(
    std::span<std::uint8_t> ram,
    std::uint16_t limit) noexcept;

/// 0x00E36E against Work RAM: reads $FF16F0 bit 7 -> aux ($FF1766/$FF1770), $FF2D8C rafts, updates +$36.
bool probe_entity_water_surface_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept;

} // namespace thor::physics

namespace oasis::game::physics {
using namespace thor::physics;
} // namespace oasis::game::physics
