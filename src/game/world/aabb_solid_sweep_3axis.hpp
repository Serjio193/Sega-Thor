#pragma once

#include "game/entities/solid_entity_sweep.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::world {

inline constexpr std::uint32_t kSweepNegXAddr = 0x009EBAU;
inline constexpr std::uint32_t kSweepPosYAddr = 0x009FA0U;
inline constexpr std::uint32_t kSweepNegYAddr = 0x00A09AU;
inline constexpr std::uint8_t kSolidContactType = 3U; // 0x6C(a6) = 3

enum class SolidSweepAxis : std::uint8_t {
    NegX = 0, // 0x009EBA: flag 0x40 bit 7, push 0x72(a5) by -1
    PosY,     // 0x009FA0: flag 0x41 bit 7, push 0x76(a5) by +1
    NegY,     // 0x00A09A: flag 0x41 bit 7, push 0x76(a5) by -1
};

struct SolidSweep3AxisResult {
    std::int32_t clamped_delta{0};
    std::optional<std::size_t> blocker_index{}; // 0x68(a6)
    std::uint8_t contact_type{0U};              // 0x6C(a6) = 3 on contact
    bool hit_flag_bit7{false};                  // 0x40 (NegX) or 0x41 (PosY/NegY) bit 7
    std::int8_t push_delta{0};                  // -1, +1, or 0
};

/// Models 0x009EBA (-X), 0x009FA0 (+Y), and 0x00A09A (-Y) across the 21-slot
/// 0xBC-stride entity pool, clamping movement to nearest 3D AABB gap and
/// emitting ±1 push_delta when gap == 0 and push flags allow.
[[nodiscard]] SolidSweep3AxisResult sweep_aabb_solid_3axis(
    SolidSweepAxis axis,
    const entities::SolidSweepBlocker& mover,
    std::size_t mover_index,
    std::int32_t delta,
    std::span<const entities::SolidSweepBlocker> slots,
    bool mover_can_push = true) noexcept;

} // namespace oasis::game::world
