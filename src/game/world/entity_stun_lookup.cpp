#include "game/world/entity_stun_lookup.hpp"

#include <algorithm>
#include <array>
#include <optional>

namespace thor::world {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
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

[[nodiscard]] std::optional<std::size_t> normalize_ram_offset(
    std::uint32_t ram_addr,
    std::size_t required_bytes,
    std::size_t ram_size) noexcept
{
    std::uint32_t offset = ram_addr;
    if (ram_addr >= 0x00FF0000U && ram_addr <= 0x00FFFFFFU) {
        offset = ram_addr - 0x00FF0000U;
    } else if (ram_addr >= 0x10000U) {
        return std::nullopt;
    }

    const std::size_t off = static_cast<std::size_t>(offset);
    if (off > ram_size || (ram_size - off) < required_bytes) {
        return std::nullopt;
    }
    return off;
}

[[nodiscard]] constexpr std::int32_t scale_diagonal_075(std::int32_t v) noexcept {
    const std::int32_t half = v >> 1;
    const std::int32_t quarter = half >> 1;
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(half) + static_cast<std::uint32_t>(quarter));
}

[[nodiscard]] StunBroadcastEntity read_entity_at_offset(
    std::span<const std::uint8_t> ram,
    std::size_t off) noexcept
{
    StunBroadcastEntity entity{};
    entity.type = static_cast<std::int16_t>(read_be16(ram, off + kStunEntityTypeOffset));
    entity.facing_16 = read_be16(ram, off + kStunEntityFacingOffset);
    entity.flags_38 = ram[off + kStunEntityFlags38Offset];
    entity.flags_3a = ram[off + kStunEntityFlags3AOffset];
    entity.vel_x_fp = static_cast<std::int32_t>(read_be32(ram, off + kStunEntityVelXFpOffset));
    entity.vel_y_fp = static_cast<std::int32_t>(read_be32(ram, off + kStunEntityVelYFpOffset));
    entity.vel_z_fp = static_cast<std::int32_t>(read_be32(ram, off + kStunEntityVelZFpOffset));
    entity.hitstun_82 = ram[off + kStunEntityHitstunOffset];
    entity.launch_dir_84 = read_be16(ram, off + kStunEntityLaunchDirOffset);
    entity.damage_86 = read_be16(ram, off + kStunEntityDamageOffset);
    entity.hit_timer_8c = read_be16(ram, off + kStunEntityHitTimerOffset);
    return entity;
}

void write_stun_mutations_at_offset(
    std::span<std::uint8_t> ram,
    std::size_t off,
    const StunBroadcastEntity& entity) noexcept
{
    write_be32(ram, off + kStunEntityVelXFpOffset, static_cast<std::uint32_t>(entity.vel_x_fp));
    write_be32(ram, off + kStunEntityVelYFpOffset, static_cast<std::uint32_t>(entity.vel_y_fp));
    write_be32(ram, off + kStunEntityVelZFpOffset, static_cast<std::uint32_t>(entity.vel_z_fp));
    write_be16(ram, off + kStunEntityLaunchDirOffset, entity.launch_dir_84);
    write_be16(ram, off + kStunEntityDamageOffset, entity.damage_86);
    write_be16(ram, off + kStunEntityHitTimerOffset, entity.hit_timer_8c);
}

} // namespace

void apply_default_stun_launch_physics(
    StunBroadcastEntity& entity,
    std::uint16_t status_mask_d7) noexcept
{
    // 0x00E4D0..0x00E500: abort if (d7 & 0xC000) != 0; else project (0x0300, 0x02E0) along dir & 7.
    if ((status_mask_d7 & 0xC000U) != 0U) {
        return;
    }

    const std::int32_t sx =
        static_cast<std::int32_t>(static_cast<std::uint32_t>(kStunLaunchBaseSpeedX) << 8U);
    const std::int32_t sy =
        static_cast<std::int32_t>(static_cast<std::uint32_t>(kStunLaunchBaseSpeedY) << 8U);

    switch (entity.launch_dir_84 & 0x0007U) {
    case 0U: // North
        entity.vel_x_fp = 0;
        entity.vel_y_fp = -sy;
        break;
    case 1U: // East
        entity.vel_x_fp = sx;
        entity.vel_y_fp = 0;
        break;
    case 2U: // South
        entity.vel_x_fp = 0;
        entity.vel_y_fp = sy;
        break;
    case 3U: // West
        entity.vel_x_fp = -sx;
        entity.vel_y_fp = 0;
        break;
    case 4U: // North-East
        entity.vel_x_fp = scale_diagonal_075(sx);
        entity.vel_y_fp = scale_diagonal_075(-sy);
        break;
    case 5U: // South-East
        entity.vel_x_fp = scale_diagonal_075(sx);
        entity.vel_y_fp = scale_diagonal_075(sy);
        break;
    case 6U: // South-West
        entity.vel_x_fp = scale_diagonal_075(-sx);
        entity.vel_y_fp = scale_diagonal_075(sy);
        break;
    case 7U: // North-West
        entity.vel_x_fp = scale_diagonal_075(-sx);
        entity.vel_y_fp = scale_diagonal_075(-sy);
        break;
    default:
        break;
    }
    entity.vel_z_fp = kStunLaunchUpwardVelZFp;
}

StunBroadcastResult broadcast_pool2_knockback_stun(
    std::span<StunBroadcastEntity> pool2_slots,
    std::uint16_t caller_slot_index,
    std::uint32_t& script_event_ptr_ff17b2,
    const StunLaunchCallback& launch_cb)
{
    StunBroadcastResult res{};
    if (caller_slot_index >= kStunBroadcastPool2SlotCount ||
        caller_slot_index >= pool2_slots.size())
    {
        return res;
    }

    // 0x00F1C4..0x00F1C8: tst.w $0(a5); bne.b $f22a (caller slot must be free / == 0).
    if (pool2_slots[caller_slot_index].type != 0) {
        return res;
    }

    const std::size_t scan_count =
        std::min(pool2_slots.size(), kStunBroadcastPool2SlotCount);
    for (std::size_t i = 0U; i < scan_count; ++i) {
        if (i == caller_slot_index) {
            continue;
        }

        auto& slot = pool2_slots[i];
        if (!is_stun_broadcast_eligible(slot)) {
            continue;
        }

        // 0x00F1EE..0x00F1F6: read +$16(a6), toggle bit 1, write +$84(a6) without mutating +$16.
        slot.launch_dir_84 = static_cast<std::uint16_t>(slot.facing_16 ^ kStunReverseFacingBit1Mask);

        // 0x00F1FA..0x00F1FC: clr.w d7; bsr.w $e4d0.
        if (launch_cb) {
            launch_cb(i, slot, 0U);
        } else {
            apply_default_stun_launch_physics(slot, 0U);
        }

        // 0x00F200..0x00F210: +$86 = $CFFF; if +$8C == 0 -> +$8C = $FFFF.
        slot.damage_86 = kStunKnockbackDamageWord;
        if (slot.hit_timer_8c == 0U) {
            slot.hit_timer_8c = kStunHitTimerLatchWord;
        }
        res.affected_slots.push_back(i);
    }

    // 0x00F21A..0x00F224: $FF17B2 = 0x0004382E; ori.b #$1, ccr.
    script_event_ptr_ff17b2 = kStunScriptEventPtrValue;
    res.carry_set = true;
    res.script_event_ptr_ff17b2 = kStunScriptEventPtrValue;
    return res;
}

GlobalTypeLookupResult find_entity_by_type_id_global(
    std::span<const std::int16_t> pool1_types,
    std::int16_t target_type) noexcept
{
    const std::size_t scan_count =
        std::min(pool1_types.size(), kGlobalTypeLookupPool1SlotCount);
    for (std::size_t i = 0U; i < scan_count; ++i) {
        if (pool1_types[i] == target_type) {
            return GlobalTypeLookupResult{
                .found = true,
                .slot_index = i,
                .slot_ram_addr = kGlobalTypeLookupPool1RamAddr +
                                 static_cast<std::uint32_t>(i * kStunEntitySlotStrideBytes),
            };
        }
    }
    return GlobalTypeLookupResult{};
}

GlobalTypeLookupResult find_entity_by_type_id_global(
    std::span<const StunBroadcastEntity> pool1_slots,
    std::int16_t target_type) noexcept
{
    const std::size_t scan_count =
        std::min(pool1_slots.size(), kGlobalTypeLookupPool1SlotCount);
    for (std::size_t i = 0U; i < scan_count; ++i) {
        if (pool1_slots[i].type == target_type) {
            return GlobalTypeLookupResult{
                .found = true,
                .slot_index = i,
                .slot_ram_addr = kGlobalTypeLookupPool1RamAddr +
                                 static_cast<std::uint32_t>(i * kStunEntitySlotStrideBytes),
            };
        }
    }
    return GlobalTypeLookupResult{};
}

StunBroadcastEntity load_stun_entity_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept
{
    const auto off = normalize_ram_offset(entity_ram_addr, kStunEntitySlotStrideBytes, ram.size());
    if (!off.has_value()) {
        return StunBroadcastEntity{};
    }
    return read_entity_at_offset(ram, *off);
}

void store_stun_entity_to_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    const StunBroadcastEntity& entity) noexcept
{
    const auto off = normalize_ram_offset(entity_ram_addr, kStunEntitySlotStrideBytes, ram.size());
    if (!off.has_value()) {
        return;
    }

    write_be16(ram, *off + kStunEntityTypeOffset, static_cast<std::uint16_t>(entity.type));
    write_be16(ram, *off + kStunEntityFacingOffset, entity.facing_16);
    ram[*off + kStunEntityFlags38Offset] = entity.flags_38;
    ram[*off + kStunEntityFlags3AOffset] = entity.flags_3a;
    ram[*off + kStunEntityHitstunOffset] = entity.hitstun_82;
    write_stun_mutations_at_offset(ram, *off, entity);
}

StunBroadcastResult broadcast_pool2_knockback_stun_from_ram(
    std::span<std::uint8_t> ram,
    std::uint16_t caller_slot_index,
    const StunLaunchCallback& launch_cb)
{
    if (ram.size() < kStunPoolMemoryEndOffset ||
        caller_slot_index >= kStunBroadcastPool2SlotCount)
    {
        return StunBroadcastResult{};
    }

    std::array<StunBroadcastEntity, kStunBroadcastPool2SlotCount> slots{};
    for (std::size_t i = 0U; i < kStunBroadcastPool2SlotCount; ++i) {
        const std::size_t slot_off =
            kStunBroadcastPool2RamOffset + (i * kStunEntitySlotStrideBytes);
        slots[i] = read_entity_at_offset(ram, slot_off);
    }

    std::uint32_t script_event_ptr = read_be32(ram, kStunScriptEventPtrRamOffset);
    StunBroadcastResult res = broadcast_pool2_knockback_stun(
        slots, caller_slot_index, script_event_ptr, launch_cb);
    if (!res.carry_set) {
        return res;
    }

    for (const std::size_t idx : res.affected_slots) {
        const std::size_t slot_off =
            kStunBroadcastPool2RamOffset + (idx * kStunEntitySlotStrideBytes);
        write_stun_mutations_at_offset(ram, slot_off, slots[idx]);
    }
    write_be32(ram, kStunScriptEventPtrRamOffset, script_event_ptr);
    return res;
}

GlobalTypeLookupResult find_entity_by_type_id_global_from_ram(
    std::span<const std::uint8_t> ram,
    std::int16_t target_type) noexcept
{
    if (ram.size() < kStunPoolMemoryEndOffset) {
        return GlobalTypeLookupResult{};
    }

    for (std::size_t i = 0U; i < kGlobalTypeLookupPool1SlotCount; ++i) {
        const std::size_t slot_off =
            kGlobalTypeLookupPool1RamOffset + (i * kStunEntitySlotStrideBytes);
        const auto slot_type =
            static_cast<std::int16_t>(read_be16(ram, slot_off + kStunEntityTypeOffset));
        if (slot_type == target_type) {
            return GlobalTypeLookupResult{
                .found = true,
                .slot_index = i,
                .slot_ram_addr = kGlobalTypeLookupPool1RamAddr +
                                 static_cast<std::uint32_t>(i * kStunEntitySlotStrideBytes),
            };
        }
    }
    return GlobalTypeLookupResult{};
}

} // namespace thor::world
