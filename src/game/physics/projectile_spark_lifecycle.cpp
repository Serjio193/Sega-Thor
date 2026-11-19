#include "game/physics/projectile_spark_lifecycle.hpp"

#include <algorithm>
#include <optional>

namespace thor::physics {

namespace {

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

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(buf[offset]) << 24U) |
           (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(buf[offset + 3U]);
}

[[nodiscard]] std::int32_t read_be32_signed(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return static_cast<std::int32_t>(read_be32(buf, offset));
}

void write_be16(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::uint16_t value) noexcept {
    buf[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be16_signed(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::int16_t value) noexcept {
    write_be16(buf, offset, static_cast<std::uint16_t>(value));
}

void write_be32(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::uint32_t value) noexcept {
    buf[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32_signed(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::int32_t value) noexcept {
    write_be32(buf, offset, static_cast<std::uint32_t>(value));
}

[[nodiscard]] std::optional<std::size_t> wram_addr_to_offset(
    std::uint32_t ram_addr,
    std::size_t required_bytes,
    std::size_t ram_size) noexcept {
    if ((ram_addr & 0xFFFF0000U) != kMegaDriveWramBase) {
        return std::nullopt;
    }
    const std::size_t off = static_cast<std::size_t>(ram_addr & 0xFFFFU);
    if (off > ram_size || (ram_size - off) < required_bytes) {
        return std::nullopt;
    }
    return off;
}

void apply_gravity_fall_step(ProjectileLifecycleSlot& slot) noexcept {
    if (slot.elevation_fp == 0) {
        return;
    }
    const std::int32_t new_vel = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(slot.velocity_z_fp) -
        static_cast<std::uint32_t>(kProjectileGravityStepFp));
    slot.velocity_z_fp = new_vel;

    const std::int32_t new_elev = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(slot.elevation_fp) +
        static_cast<std::uint32_t>(new_vel));
    slot.elevation_fp = (new_elev < 0) ? 0 : new_elev;
}

void apply_gravity_fall_step_ram(
    std::span<std::uint8_t> ram,
    std::size_t slot_off) noexcept {
    const std::int32_t elev = read_be32_signed(ram, slot_off + kSlotElevationFpOffset);
    if (elev == 0) {
        return;
    }
    const std::int32_t vel = read_be32_signed(ram, slot_off + kSlotVelocityZFpOffset);
    const std::int32_t new_vel = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(vel) - static_cast<std::uint32_t>(kProjectileGravityStepFp));
    write_be32_signed(ram, slot_off + kSlotVelocityZFpOffset, new_vel);

    const std::int32_t new_elev = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(elev) + static_cast<std::uint32_t>(new_vel));
    write_be32_signed(ram, slot_off + kSlotElevationFpOffset, (new_elev < 0) ? 0 : new_elev);
}

void try_unlatch_parent_on_landing(ProjectileLifecycleSlot& slot) noexcept {
    if (slot.slot_ram_addr >= kProjectilePoolBaseRamAddr || slot.elevation_fp != 0) {
        return;
    }
    if (slot.parent != nullptr) {
        slot.parent->flags_36 = static_cast<std::uint8_t>(
            slot.parent->flags_36 & static_cast<std::uint8_t>(~kSlotBlinkBitMask));
        slot.parent->pos_y = slot.pos_y;
    }
    slot.type = 0;
}

void try_unlatch_parent_on_landing_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t slot_ram_addr,
    std::size_t slot_off) noexcept {
    if (slot_ram_addr >= kProjectilePoolBaseRamAddr) {
        return;
    }
    if (read_be32_signed(ram, slot_off + kSlotElevationFpOffset) != 0) {
        return;
    }
    if (slot_off + kSlotParentPtrOffset + 4U <= ram.size()) {
        const std::uint32_t parent_addr = read_be32(ram, slot_off + kSlotParentPtrOffset);
        const auto parent_off = wram_addr_to_offset(parent_addr, kSlotFlags36Offset + 1U, ram.size());
        if (parent_off.has_value()) {
            ram[*parent_off + kSlotFlags36Offset] = static_cast<std::uint8_t>(
                ram[*parent_off + kSlotFlags36Offset] &
                static_cast<std::uint8_t>(~kSlotBlinkBitMask));
            const std::uint16_t slot_y = read_be16(ram, slot_off + kSlotPosYWordOffset);
            write_be16(ram, *parent_off + kSlotPosYWordOffset, slot_y);
        }
    }
    write_be16(ram, slot_off + kSlotTypeOffset, 0U);
}

void step_lifetime_blink_and_despawn(
    std::int16_t& type,
    std::int16_t& lifetime,
    std::uint8_t& flags_36) noexcept {
    // 0x00E1D2..0x00E208: d0 = lifetime + 1; writeback skipped when d0 > 300
    const auto next_frame = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(lifetime) + 1U);
    if (next_frame > kProjectileBlinkEndFrame) {
        type = 0;
        return;
    }
    if (next_frame >= kProjectileBlinkStartFrame) {
        if ((static_cast<std::uint16_t>(next_frame) & 1U) != 0U) {
            flags_36 = static_cast<std::uint8_t>(flags_36 | kSlotBlinkBitMask);
        } else {
            flags_36 = static_cast<std::uint8_t>(
                flags_36 & static_cast<std::uint8_t>(~kSlotBlinkBitMask));
        }
    }
    lifetime = next_frame;
}

} // namespace

std::size_t dispatch_spark_effects(
    std::span<SparkEffectSlot> sparks,
    std::uint8_t pause_flags_ff17b8,
    const SparkCallback& callback) {
    // 0x00E0BA..0x00E0C4: btst.b #1, $ff17b8; bne rts
    if ((pause_flags_ff17b8 & kProjectilePauseBitMask) != 0U) {
        return 0U;
    }

    const std::size_t count = std::min(sparks.size(), kSparkPoolSlotCount);
    std::size_t invoked = 0U;
    for (std::size_t i = 0U; i < count; ++i) {
        SparkEffectSlot& slot = sparks[i];
        if (!slot.is_active()) {
            continue;
        }
        if (callback) {
            callback(i, slot);
        }
        ++invoked;
    }
    return invoked;
}

bool step_projectile_gravity_landing(ProjectileLifecycleSlot& slot) noexcept {
    apply_gravity_fall_step(slot);
    try_unlatch_parent_on_landing(slot);
    return slot.type == 0;
}

bool step_projectile_gravity_blink_despawn(ProjectileLifecycleSlot& slot) noexcept {
    apply_gravity_fall_step(slot);
    // 0x00E1B0..0x00E1D0: unlatches parent if < $FF2ABC, then falls through to 0x00E1D2
    try_unlatch_parent_on_landing(slot);
    step_lifetime_blink_and_despawn(slot.type, slot.lifetime, slot.flags_36);
    return slot.type == 0;
}

bool dispatch_projectile_slot_state(ProjectileLifecycleSlot& slot) noexcept {
    const std::uint32_t target = resolve_projectile_jump_target(slot.state);
    if (target == kProjectileGravityLandingAddr) {
        return step_projectile_gravity_landing(slot);
    }
    if (target == kProjectileGravityBlinkDespawnAddr) {
        return step_projectile_gravity_blink_despawn(slot);
    }
    return false;
}

std::size_t dispatch_projectile_entities(
    std::span<ProjectileLifecycleSlot> projectiles,
    std::uint16_t& global_timer_ff1856,
    std::uint8_t pause_flags_ff17b8) noexcept {
    // 0x00E0F4..0x00E104: decrement $FF1856 before checking pause bit
    if (global_timer_ff1856 != 0U) {
        --global_timer_ff1856;
    }
    // 0x00E106..0x00E110: btst.b #1, $ff17b8; bne rts
    if ((pause_flags_ff17b8 & kProjectilePauseBitMask) != 0U) {
        return 0U;
    }

    const std::size_t count = std::min(projectiles.size(), kProjectilePoolSlotCount);
    std::size_t dispatched = 0U;
    for (std::size_t i = 0U; i < count; ++i) {
        ProjectileLifecycleSlot& slot = projectiles[i];
        if (slot.type <= 0 || resolve_projectile_jump_target(slot.state) == 0U) {
            continue;
        }
        (void)dispatch_projectile_slot_state(slot);
        ++dispatched;
    }
    return dispatched;
}

bool step_projectile_bounce_phase(BouncePhaseEntitySlot& entity) noexcept {
    if (entity.bounce_phase_9e == 0U) {
        entity.field_16 = 0U;
        entity.elevation_fp = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(static_cast<std::uint16_t>(kBouncePhaseInitElevationInt)) << 16U) |
            (static_cast<std::uint32_t>(entity.elevation_fp) & 0xFFFFU));
        entity.bounce_phase_9e = kBouncePhaseInitNext;
        return true;
    }
    if (entity.bounce_phase_9e == 1U) {
        entity.field_16 = 0U;
        entity.field_2a = static_cast<std::uint16_t>(entity.field_2a + 2U);
        entity.elevation_fp = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(static_cast<std::uint16_t>(kBouncePhaseTriggerElevationInt)) << 16U) |
            (static_cast<std::uint32_t>(entity.elevation_fp) & 0xFFFFU));
        entity.tile_attr_3d = kBouncePhaseTriggerTileAttr;
        entity.sprite_id_4a = kBouncePhaseTriggerSpriteId;
        entity.radius_46 = kBouncePhaseTriggerRadius;
        entity.extent_x_48 = kBouncePhaseTriggerExtent;
        entity.extent_y_49 = kBouncePhaseTriggerExtent;
        entity.bounce_phase_9e = kBouncePhaseTriggerNext;
        return true;
    }
    return false;
}

SparkEffectSlot load_spark_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index) noexcept {
    SparkEffectSlot slot{};
    if (slot_index >= kSparkPoolSlotCount) {
        return slot;
    }
    const std::size_t base = kSparkPoolBaseRamOffset + (slot_index * kLightSlotStrideBytes);
    if (base > ram.size() || (ram.size() - base) < kLightSlotStrideBytes) {
        return slot;
    }
    slot.type = read_be16_signed(ram, base + kSlotTypeOffset);
    slot.callback_addr = read_be32(ram, base + kSlotVelocityZFpOffset);
    slot.ram_addr = static_cast<std::uint32_t>(kSparkPoolBaseRamAddr + slot_index * kLightSlotStrideBytes);
    return slot;
}

void store_spark_slot_to_ram(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const SparkEffectSlot& slot) noexcept {
    if (slot_index >= kSparkPoolSlotCount) {
        return;
    }
    const std::size_t base = kSparkPoolBaseRamOffset + (slot_index * kLightSlotStrideBytes);
    if (base > ram.size() || (ram.size() - base) < kLightSlotStrideBytes) {
        return;
    }
    write_be16_signed(ram, base + kSlotTypeOffset, slot.type);
    write_be32(ram, base + kSlotVelocityZFpOffset, slot.callback_addr);
}

ProjectileLifecycleSlot load_projectile_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index) noexcept {
    ProjectileLifecycleSlot slot{};
    if (slot_index >= kProjectilePoolSlotCount) {
        return slot;
    }
    const std::size_t base = kProjectilePoolBaseRamOffset + (slot_index * kLightSlotStrideBytes);
    if (base > ram.size() || (ram.size() - base) < kLightSlotStrideBytes) {
        return slot;
    }
    slot.type = read_be16_signed(ram, base + kSlotTypeOffset);
    slot.state = read_be16_signed(ram, base + kSlotStateOffset);
    slot.pos_y = read_be16_signed(ram, base + kSlotPosYWordOffset);
    slot.elevation_fp = read_be32_signed(ram, base + kSlotElevationFpOffset);
    slot.lifetime = read_be16_signed(ram, base + kSlotLifetimeOffset);
    slot.flags_36 = ram[base + kSlotFlags36Offset];
    slot.velocity_z_fp = read_be32_signed(ram, base + kSlotVelocityZFpOffset);
    slot.slot_ram_addr = static_cast<std::uint32_t>(
        kProjectilePoolBaseRamAddr + slot_index * kLightSlotStrideBytes);
    return slot;
}

void store_projectile_slot_to_ram(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const ProjectileLifecycleSlot& slot) noexcept {
    if (slot_index >= kProjectilePoolSlotCount) {
        return;
    }
    const std::size_t base = kProjectilePoolBaseRamOffset + (slot_index * kLightSlotStrideBytes);
    if (base > ram.size() || (ram.size() - base) < kLightSlotStrideBytes) {
        return;
    }
    write_be16_signed(ram, base + kSlotTypeOffset, slot.type);
    write_be16_signed(ram, base + kSlotStateOffset, slot.state);
    write_be16_signed(ram, base + kSlotPosYWordOffset, slot.pos_y);
    write_be32_signed(ram, base + kSlotElevationFpOffset, slot.elevation_fp);
    write_be16_signed(ram, base + kSlotLifetimeOffset, slot.lifetime);
    ram[base + kSlotFlags36Offset] = slot.flags_36;
    write_be32_signed(ram, base + kSlotVelocityZFpOffset, slot.velocity_z_fp);
}

std::size_t dispatch_spark_effects_from_ram(
    std::span<std::uint8_t> ram,
    const SparkRamCallback& callback) {
    if (ram.size() < (kSparkPoolBaseRamOffset + kSparkPoolByteSize)) {
        return 0U;
    }
    if ((ram[kProjectilePauseFlagsRamOffset] & kProjectilePauseBitMask) != 0U) {
        return 0U;
    }

    std::size_t invoked = 0U;
    for (std::size_t i = 0U; i < kSparkPoolSlotCount; ++i) {
        const std::size_t base = kSparkPoolBaseRamOffset + (i * kLightSlotStrideBytes);
        const std::int16_t type = read_be16_signed(ram, base + kSlotTypeOffset);
        const std::uint32_t cb_addr = read_be32(ram, base + kSlotVelocityZFpOffset);
        if (type == 0 || cb_addr == 0U) {
            continue;
        }
        if (callback) {
            const auto slot_addr = static_cast<std::uint32_t>(
                kSparkPoolBaseRamAddr + i * kLightSlotStrideBytes);
            callback(i, slot_addr, cb_addr, ram);
        }
        ++invoked;
    }
    return invoked;
}

bool step_projectile_gravity_landing_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t slot_ram_addr) noexcept {
    const auto slot_off = wram_addr_to_offset(slot_ram_addr, kLightSlotStrideBytes, ram.size());
    if (!slot_off.has_value()) {
        return false;
    }
    apply_gravity_fall_step_ram(ram, *slot_off);
    try_unlatch_parent_on_landing_ram(ram, slot_ram_addr, *slot_off);
    return read_be16_signed(ram, *slot_off + kSlotTypeOffset) == 0;
}

bool step_projectile_gravity_blink_despawn_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t slot_ram_addr) noexcept {
    const auto slot_off = wram_addr_to_offset(slot_ram_addr, kLightSlotStrideBytes, ram.size());
    if (!slot_off.has_value()) {
        return false;
    }
    apply_gravity_fall_step_ram(ram, *slot_off);
    try_unlatch_parent_on_landing_ram(ram, slot_ram_addr, *slot_off);

    std::int16_t type = read_be16_signed(ram, *slot_off + kSlotTypeOffset);
    std::int16_t lifetime = read_be16_signed(ram, *slot_off + kSlotLifetimeOffset);
    std::uint8_t flags_36 = ram[*slot_off + kSlotFlags36Offset];
    step_lifetime_blink_and_despawn(type, lifetime, flags_36);
    write_be16_signed(ram, *slot_off + kSlotTypeOffset, type);
    write_be16_signed(ram, *slot_off + kSlotLifetimeOffset, lifetime);
    ram[*slot_off + kSlotFlags36Offset] = flags_36;
    return type == 0;
}

std::size_t dispatch_projectile_entities_from_ram(std::span<std::uint8_t> ram) noexcept {
    if (ram.size() < (kProjectilePoolBaseRamOffset + kProjectilePoolByteSize)) {
        return 0U;
    }
    const std::uint16_t timer = read_be16(ram, kProjectileGlobalTimerRamOffset);
    if (timer != 0U) {
        write_be16(ram, kProjectileGlobalTimerRamOffset, static_cast<std::uint16_t>(timer - 1U));
    }
    if ((ram[kProjectilePauseFlagsRamOffset] & kProjectilePauseBitMask) != 0U) {
        return 0U;
    }

    std::size_t dispatched = 0U;
    for (std::size_t i = 0U; i < kProjectilePoolSlotCount; ++i) {
        const auto slot_addr = static_cast<std::uint32_t>(
            kProjectilePoolBaseRamAddr + i * kLightSlotStrideBytes);
        const std::size_t base = kProjectilePoolBaseRamOffset + (i * kLightSlotStrideBytes);
        const std::int16_t type = read_be16_signed(ram, base + kSlotTypeOffset);
        if (type <= 0) {
            continue;
        }
        const std::int16_t state = read_be16_signed(ram, base + kSlotStateOffset);
        const std::uint32_t target = resolve_projectile_jump_target(state);
        if (target == kProjectileGravityLandingAddr) {
            (void)step_projectile_gravity_landing_from_ram(ram, slot_addr);
            ++dispatched;
        } else if (target == kProjectileGravityBlinkDespawnAddr) {
            (void)step_projectile_gravity_blink_despawn_from_ram(ram, slot_addr);
            ++dispatched;
        }
    }
    return dispatched;
}

bool step_projectile_bounce_phase_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept {
    const auto off = wram_addr_to_offset(entity_ram_addr, kSlotBouncePhaseOffset + 1U, ram.size());
    if (!off.has_value()) {
        return false;
    }

    const std::uint8_t phase = ram[*off + kSlotBouncePhaseOffset];
    if (phase == 0U) {
        write_be16(ram, *off + kSlotTimer16Offset, 0U);
        write_be16_signed(ram, *off + kSlotElevationFpOffset, kBouncePhaseInitElevationInt);
        ram[*off + kSlotBouncePhaseOffset] = kBouncePhaseInitNext;
        return true;
    }
    if (phase == 1U) {
        write_be16(ram, *off + kSlotTimer16Offset, 0U);
        const std::uint16_t prev_2a = read_be16(ram, *off + kSlotFrameDirOffset);
        write_be16(ram, *off + kSlotFrameDirOffset, static_cast<std::uint16_t>(prev_2a + 2U));
        write_be16_signed(ram, *off + kSlotElevationFpOffset, kBouncePhaseTriggerElevationInt);
        ram[*off + kSlotTileAttrByteOffset] = kBouncePhaseTriggerTileAttr;
        write_be16(ram, *off + kSlotSpriteIdOffset, kBouncePhaseTriggerSpriteId);
        write_be16(ram, *off + kSlotRadiusWordOffset, kBouncePhaseTriggerRadius);
        ram[*off + kSlotExtentXOffset] = kBouncePhaseTriggerExtent;
        ram[*off + kSlotExtentYOffset] = kBouncePhaseTriggerExtent;
        ram[*off + kSlotBouncePhaseOffset] = kBouncePhaseTriggerNext;
        return true;
    }
    return false;
}

} // namespace thor::physics
