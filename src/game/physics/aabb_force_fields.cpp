#include "game/physics/aabb_force_fields.hpp"

#include <algorithm>

namespace thor::physics {

namespace {

[[nodiscard]] constexpr std::int32_t add_wrapping_i32(
    std::int32_t lhs,
    std::int32_t rhs) noexcept {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(lhs) + static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8U) |
        static_cast<std::uint16_t>(buf[offset + 1U]));
}

[[nodiscard]] std::int16_t read_be16_signed(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return static_cast<std::int16_t>(read_be16(buf, offset));
}

[[nodiscard]] std::int32_t read_be32_signed(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    const std::uint32_t raw =
        (static_cast<std::uint32_t>(buf[offset]) << 24U) |
        (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
        (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
        static_cast<std::uint32_t>(buf[offset + 3U]);
    return static_cast<std::int32_t>(raw);
}

void write_be16_signed(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::int16_t value) noexcept {
    const auto u = static_cast<std::uint16_t>(value);
    buf[offset] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>(u & 0xFFU);
}

void write_be32_signed(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::int32_t value) noexcept {
    const auto u = static_cast<std::uint32_t>(value);
    buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
}

[[nodiscard]] constexpr std::size_t slot_ram_offset(std::size_t slot_index) noexcept {
    return kEntityPoolBaseRamOffset + (slot_index * kEntitySlotStrideBytes);
}

[[nodiscard]] constexpr bool should_apply_force(
    const ForceEntitySlot& ent,
    const BoundingBox3D& box,
    ForceFieldMode mode) noexcept {
    if (!is_entity_active_for_force_field(ent.status)) {
        return false;
    }
    if (mode == ForceFieldMode::ConveyorCurrent && is_conveyor_current_immune(ent.flags_3a)) {
        return false;
    }
    return box.contains(ent.pos_x, ent.pos_y, ent.integer_z());
}

void accumulate_slot_force(
    ForceEntitySlot& ent,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp,
    ForceFieldMode mode) noexcept {
    ent.accum_dx_fp = add_wrapping_i32(ent.accum_dx_fp, force_x_fp);
    ent.accum_dy_fp = add_wrapping_i32(ent.accum_dy_fp, force_y_fp);

    if (mode != ForceFieldMode::Standard && has_elevation_boost(ent.pos_z_fp)) {
        ent.accum_dx_fp = add_wrapping_i32(ent.accum_dx_fp, force_x_fp);
        ent.accum_dy_fp = add_wrapping_i32(ent.accum_dy_fp, force_y_fp);
    }
}

} // namespace

std::size_t apply_force_field(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp,
    ForceFieldMode mode) noexcept {
    const std::size_t count = std::min(entities.size(), kEntityPoolSlotCount);
    std::size_t affected = 0U;

    for (std::size_t i = 0U; i < count; ++i) {
        ForceEntitySlot& ent = entities[i];
        if (!should_apply_force(ent, box, mode)) {
            continue;
        }
        accumulate_slot_force(ent, force_x_fp, force_y_fp, mode);
        ++affected;
    }

    return affected;
}

std::size_t apply_box_force_vector(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept {
    return apply_force_field(entities, box, force_x_fp, force_y_fp, ForceFieldMode::Standard);
}

std::size_t apply_box_force_vector_with_elevation_boost(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept {
    return apply_force_field(entities, box, force_x_fp, force_y_fp, ForceFieldMode::ElevationBoost);
}

std::size_t apply_conveyor_current_force(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept {
    return apply_force_field(entities, box, force_x_fp, force_y_fp, ForceFieldMode::ConveyorCurrent);
}

ForceEntitySlot load_force_entity_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index) noexcept {
    ForceEntitySlot slot{};
    if (slot_index >= kEntityPoolSlotCount) {
        return slot;
    }
    const std::size_t base = slot_ram_offset(slot_index);
    if (base > ram.size() || (ram.size() - base) < kEntitySlotStrideBytes) {
        return slot;
    }

    slot.status = read_be16_signed(ram, base + kEntityStatusOffset);
    slot.pos_x = read_be16_signed(ram, base + kEntityPosXOffset);
    slot.pos_y = read_be16_signed(ram, base + kEntityPosYOffset);
    slot.pos_z_fp = read_be32_signed(ram, base + kEntityPosZFpOffset);
    slot.flags_3a = ram[base + kEntityFlags3AOffset];
    slot.accum_dx_fp = read_be32_signed(ram, base + kEntityAccumDxFpOffset);
    slot.accum_dy_fp = read_be32_signed(ram, base + kEntityAccumDyFpOffset);
    return slot;
}

void store_force_entity_slot_to_ram(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const ForceEntitySlot& slot) noexcept {
    if (slot_index >= kEntityPoolSlotCount) {
        return;
    }
    const std::size_t base = slot_ram_offset(slot_index);
    if (base > ram.size() || (ram.size() - base) < kEntitySlotStrideBytes) {
        return;
    }

    write_be16_signed(ram, base + kEntityStatusOffset, slot.status);
    write_be16_signed(ram, base + kEntityPosXOffset, slot.pos_x);
    write_be16_signed(ram, base + kEntityPosYOffset, slot.pos_y);
    write_be32_signed(ram, base + kEntityPosZFpOffset, slot.pos_z_fp);
    ram[base + kEntityFlags3AOffset] = slot.flags_3a;
    write_be32_signed(ram, base + kEntityAccumDxFpOffset, slot.accum_dx_fp);
    write_be32_signed(ram, base + kEntityAccumDyFpOffset, slot.accum_dy_fp);
}

std::size_t apply_force_field_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp,
    ForceFieldMode mode) noexcept {
    if (ram.size() < kEntityPoolBaseRamOffset + kEntityPoolByteSize) {
        return 0U;
    }

    std::size_t affected = 0U;
    for (std::size_t i = 0U; i < kEntityPoolSlotCount; ++i) {
        ForceEntitySlot ent = load_force_entity_slot_from_ram(ram, i);
        if (!should_apply_force(ent, box, mode)) {
            continue;
        }
        accumulate_slot_force(ent, force_x_fp, force_y_fp, mode);

        const std::size_t base = slot_ram_offset(i);
        write_be32_signed(ram, base + kEntityAccumDxFpOffset, ent.accum_dx_fp);
        write_be32_signed(ram, base + kEntityAccumDyFpOffset, ent.accum_dy_fp);
        ++affected;
    }

    return affected;
}

std::size_t apply_box_force_vector_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept {
    return apply_force_field_from_ram(ram, box, force_x_fp, force_y_fp, ForceFieldMode::Standard);
}

std::size_t apply_box_force_vector_with_elevation_boost_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept {
    return apply_force_field_from_ram(ram, box, force_x_fp, force_y_fp, ForceFieldMode::ElevationBoost);
}

std::size_t apply_conveyor_current_force_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept {
    return apply_force_field_from_ram(ram, box, force_x_fp, force_y_fp, ForceFieldMode::ConveyorCurrent);
}

std::size_t apply_force_field_packed_from_ram(
    std::span<std::uint8_t> ram,
    std::int32_t d1_force_x_fp,
    std::int32_t d2_force_y_fp,
    std::uint32_t d3_min_xy,
    std::uint32_t d4_max_xy,
    std::uint32_t d5_min_max_z,
    ForceFieldMode mode) noexcept {
    const BoundingBox3D box = BoundingBox3D::from_registers(d3_min_xy, d4_max_xy, d5_min_max_z);
    return apply_force_field_from_ram(ram, box, d1_force_x_fp, d2_force_y_fp, mode);
}

} // namespace thor::physics
