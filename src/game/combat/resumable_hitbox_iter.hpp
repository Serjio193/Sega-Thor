#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::combat {

struct ResumableHitboxConfig {
    static constexpr std::uint32_t kFindFirst3dPointHitAddr = 0x00B852U;
    static constexpr std::uint32_t kFindNext3dPointHitAddr = 0x00B912U;
    static constexpr std::uint32_t kFindFirst3dBoxHitAddr = 0x00B922U;

    static constexpr std::uint32_t kEntityPoolRam = 0x00FF19E8U;
    static constexpr std::size_t kEntitySlotCount = 21U;
    static constexpr std::size_t kEntityStrideBytes = 0x00BCU; // 188 bytes

    static constexpr std::uint32_t kIterSaveRamStart = 0x00FF194AU;
    static constexpr std::uint32_t kIterSaveRamEnd = 0x00FF196AU;
    static constexpr std::size_t kIterSaveBytes = 32U; // 8 registers d0..d7 (32 bytes)
};

struct HitboxEntity {
    std::int16_t state{0};        // +0x00: state/type (> 0 active, <= 0 inactive)
    std::int16_t pos_x{0};        // +0x08: X position
    std::int16_t pos_y{0};        // +0x0C: Y position
    std::int16_t pos_z{0};        // +0x10: Z position
    std::int16_t elevation{0};    // +0x14: visual Z/elevation bias
    std::int16_t half_width_x{0}; // +0x42: rx
    std::int16_t half_width_y{0}; // +0x44: ry
    std::int16_t height_z{0};     // +0x4A: hz
};

struct PointProbe3D {
    std::int16_t x1{0};
    std::int16_t y1{0};
    std::int16_t x2{0};
    std::int16_t y2{0};
    std::int16_t z{0};
};

struct BoxProbe3D {
    std::int16_t x1{0};
    std::int16_t y1{0};
    std::int16_t x2{0};
    std::int16_t y2{0};
    std::int16_t z1{0};
    std::int16_t z2{0};
};

struct HitboxMatchResult {
    bool hit{false};
    std::size_t entity_index{0U};
    std::uint32_t ram_addr{0U};
};

struct HitboxIteratorState {
    std::uint32_t slot_mask{0U};
    std::size_t next_slot_index{0U};
    std::size_t self_index{ResumableHitboxConfig::kEntitySlotCount};
    std::int16_t px1{0};
    std::int16_t py1{0};
    std::int16_t px2{0};
    std::int16_t py2{0};
    std::int16_t pz1{0};
    std::int16_t pz2{0};
    bool is_box_probe{false};
    bool active{false};
};

/// ROM 0x00B852..0x00B910:
/// Starts a 3D point-hit probe scan across 21 entity slots using relative probe box and caller position.
[[nodiscard]] HitboxMatchResult find_first_3d_point_hit(
    std::uint32_t slot_mask,
    std::size_t self_index,
    const HitboxEntity& caller,
    const PointProbe3D& rel_probe,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept;

/// Absolute coordinate version of point-hit probe.
[[nodiscard]] HitboxMatchResult find_first_3d_point_hit_abs(
    std::uint32_t slot_mask,
    std::size_t self_index,
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept;

/// ROM 0x00B912..0x00B920:
/// Resumes scanning remaining slots in the 21-slot entity pool.
[[nodiscard]] HitboxMatchResult find_next_3d_hit(
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept;

/// ROM 0x00B922..0x00B99E:
/// Starts a 3D box-hit probe scan with full Z interval [pz1, pz2].
[[nodiscard]] HitboxMatchResult find_first_3d_box_hit(
    std::uint32_t slot_mask,
    std::size_t self_index,
    const HitboxEntity& caller,
    const BoxProbe3D& rel_probe,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept;

/// Absolute coordinate version of box-hit probe.
[[nodiscard]] HitboxMatchResult find_first_3d_box_hit_abs(
    std::uint32_t slot_mask,
    std::size_t self_index,
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz1,
    std::int16_t pz2,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept;

/// Utility: gathers all matching entity indices using the resumable iterator.
[[nodiscard]] std::vector<std::size_t> collect_all_3d_point_hits(
    std::uint32_t slot_mask,
    std::size_t self_index,
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz,
    std::span<const HitboxEntity> entities);

} // namespace oasis::game::combat
