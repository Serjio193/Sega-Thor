#include "game/world/group2_entity_dispatcher.hpp"

#include <algorithm>

namespace thor::world {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
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

void record_dispatch_event(
    Group2DispatchResult& result,
    std::size_t slot_index,
    std::int16_t entity_type,
    std::uint32_t handler_rom_addr,
    bool is_dummy,
    const IsometricFootprint& footprint)
{
    ++result.dispatched_count;
    if (is_dummy) {
        ++result.dummy_count;
    }
    result.events.push_back(Group2DispatchEvent{
        static_cast<std::uint8_t>(slot_index),
        kGroup2PoolRamAddr + static_cast<std::uint32_t>(slot_index * kGroup2SlotStride),
        entity_type,
        handler_rom_addr,
        is_dummy,
        footprint,
    });
}

} // namespace

std::optional<std::uint32_t> lookup_group2_action_handler_in_rom(
    std::span<const std::uint8_t> rom,
    std::int16_t entity_type) noexcept
{
    const auto idx = group2_type_to_table_index(entity_type);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    const std::size_t addr = static_cast<std::size_t>(kGroup2ActionTableAddr) + (*idx * 4U);
    if (addr > rom.size() || (rom.size() - addr) < 4U) {
        return std::nullopt;
    }
    return read_be32(rom, addr);
}

IsometricFootprint apply_post_tick_footprint(Group2Entity& entity) noexcept
{
    const bool swap_xy = (entity.flags_17 & kGroup2FlagsSwapXyBit) != 0U;
    const IsometricFootprint fp =
        project_isometric_footprint(entity.raw_extent_x, entity.raw_extent_y, swap_xy);
    entity.effective_half_w = fp.half_width;
    entity.effective_half_h = fp.half_height;
    return fp;
}

IsometricFootprint apply_post_tick_footprint_to_slot(
    std::span<std::uint8_t, kGroup2SlotStride> slot_bytes) noexcept
{
    const std::uint8_t raw_x = slot_bytes[0x48U];
    const std::uint8_t raw_y = slot_bytes[0x49U];
    const bool swap_xy = (slot_bytes[0x17U] & kGroup2FlagsSwapXyBit) != 0U;
    const IsometricFootprint fp = project_isometric_footprint(raw_x, raw_y, swap_xy);
    write_be16(slot_bytes, 0x42U, static_cast<std::uint16_t>(fp.half_width));
    write_be16(slot_bytes, 0x44U, static_cast<std::uint16_t>(fp.half_height));
    return fp;
}

Group2DispatchResult dispatch_group2_entities(
    std::span<Group2Entity, kGroup2SlotCount> pool,
    bool is_paused,
    const Group2TableFnArray& jump_table)
{
    Group2DispatchResult result{};
    if (is_paused) {
        result.paused = true;
        return result;
    }

    for (std::size_t i = 0U; i < pool.size(); ++i) {
        Group2Entity& entity = pool[i];
        const auto idx = group2_type_to_table_index(entity.type);
        if (!idx.has_value()) {
            ++result.skipped_count;
            continue;
        }

        const std::int16_t dispatched_type = entity.type;
        const std::uint32_t handler_addr = kGroup2ActionHandlerAddrs[*idx];
        const EntityTickFn fn = jump_table[*idx];
        const bool is_dummy = (fn == nullptr) || (handler_addr == kGroup2DummyHandlerAddr);
        if (fn != nullptr) {
            fn(entity);
        }

        const IsometricFootprint fp = apply_post_tick_footprint(entity);
        record_dispatch_event(result, i, dispatched_type, handler_addr, is_dummy, fp);
    }
    return result;
}

Group2DispatchResult dispatch_group2_entities(
    std::span<Group2Entity> pool,
    bool is_paused,
    const Group2TickCallback& tick_cb)
{
    Group2DispatchResult result{};
    if (is_paused) {
        result.paused = true;
        return result;
    }

    const std::size_t count = std::min(pool.size(), kGroup2SlotCount);
    for (std::size_t i = 0U; i < count; ++i) {
        Group2Entity& entity = pool[i];
        const auto handler_opt = lookup_group2_action_handler(entity.type);
        if (!handler_opt.has_value()) {
            ++result.skipped_count;
            continue;
        }

        const std::int16_t dispatched_type = entity.type;
        const std::uint32_t handler_addr = *handler_opt;
        const bool is_dummy = (handler_addr == kGroup2DummyHandlerAddr);
        if (tick_cb) {
            tick_cb(i, entity, handler_addr);
        }

        const IsometricFootprint fp = apply_post_tick_footprint(entity);
        record_dispatch_event(result, i, dispatched_type, handler_addr, is_dummy, fp);
    }
    return result;
}

Group2DispatchResult dispatch_group2_entities_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const Group2RamTickCallback& tick_cb,
    std::span<const std::uint8_t> rom)
{
    Group2DispatchResult result{};
    // 0x00DEEC..0x00DEF4: btst.b #$1, $ff17b8.l; bne.w $df52
    if ((ram[kGroup2PauseGateRamOffset] & kGroup2PauseGateBit) != 0U) {
        result.paused = true;
        return result;
    }

    // 0x00DEF8..0x00DF50: sweep 20 slots ($FF1AA4 + i * 0xBC)
    for (std::size_t i = 0U; i < kGroup2SlotCount; ++i) {
        const std::size_t slot_off =
            static_cast<std::size_t>(kGroup2PoolRamOffset) + (i * kGroup2SlotStride);
        const std::span<std::uint8_t, kGroup2SlotStride> slot_bytes{
            ram.data() + slot_off,
            kGroup2SlotStride};
        const auto entity_type = static_cast<std::int16_t>(read_be16(slot_bytes, 0x00U));

        const std::optional<std::uint32_t> handler_opt = rom.empty()
            ? lookup_group2_action_handler(entity_type)
            : lookup_group2_action_handler_in_rom(rom, entity_type);
        if (!handler_opt.has_value()) {
            ++result.skipped_count;
            continue;
        }

        const std::uint32_t handler_addr = *handler_opt;
        const bool is_dummy = (handler_addr == kGroup2DummyHandlerAddr);
        if (tick_cb) {
            tick_cb(i, slot_bytes, handler_addr);
        }

        // 0x00DF20..0x00DF49 runs unconditionally after jsr (a0) returns
        const IsometricFootprint fp = apply_post_tick_footprint_to_slot(slot_bytes);
        record_dispatch_event(result, i, entity_type, handler_addr, is_dummy, fp);
    }
    return result;
}

} // namespace thor::world
