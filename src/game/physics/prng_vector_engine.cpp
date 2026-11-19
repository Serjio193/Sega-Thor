#include "game/physics/prng_vector_engine.hpp"

#include <algorithm>
#include <optional>

namespace thor::physics {

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

} // namespace

bool step_entity_lifecycle_timer(LifecycleTimerEntity& entity) noexcept {
    // 0x00E25C..0x00E266: subq.w #$1, $2(a6); bne.b $e266; clr.w $0(a6)
    entity.timer = static_cast<std::uint16_t>(entity.timer - 1U);
    if (entity.timer == 0U) {
        entity.status = 0;
        return true;
    }
    return false;
}

std::uint16_t prng_step_16(std::uint16_t& seed) noexcept {
    Prng16State state{seed};
    const std::uint16_t next = state.step();
    seed = state.seed;
    return next;
}

std::uint16_t prng_range_word(std::uint16_t& seed, std::uint16_t limit) noexcept {
    Prng16State state{seed};
    const std::uint16_t value = state.range_word(limit);
    seed = state.seed;
    return value;
}

std::uint8_t prng_range_byte(std::uint16_t& seed, std::uint16_t limit) noexcept {
    Prng16State state{seed};
    const std::uint8_t value = state.range_byte(limit);
    seed = state.seed;
    return value;
}

bool is_entity_on_water_platform(
    const WaterProbeEntity& entity,
    std::span<const WaterPlatformSlot> spark_slots) noexcept
{
    const std::size_t count = std::min(spark_slots.size(), kWaterProbeSparkPoolCount);
    const std::int32_t ent_min_x =
        static_cast<std::int32_t>(entity.pos_x) - static_cast<std::int32_t>(entity.extent_x_42);
    const std::int32_t ent_max_x =
        static_cast<std::int32_t>(entity.pos_x) + static_cast<std::int32_t>(entity.extent_x_42);
    const std::int32_t ent_min_y =
        static_cast<std::int32_t>(entity.pos_y) - static_cast<std::int32_t>(entity.extent_y_44);
    const std::int32_t ent_max_y =
        static_cast<std::int32_t>(entity.pos_y) + static_cast<std::int32_t>(entity.extent_y_44);

    // 0x009C86..0x009CF6: scan up to 6 slots for type == 0x56 overlapping 2D AABB.
    for (std::size_t i = 0U; i < count; ++i) {
        const auto& slot = spark_slots[i];
        if (slot.type != kWaterProbeRaftTypeId) {
            continue;
        }
        const std::int32_t raft_min_x =
            static_cast<std::int32_t>(slot.pos_x) - static_cast<std::int32_t>(slot.extent_x_42);
        const std::int32_t raft_max_x =
            static_cast<std::int32_t>(slot.pos_x) + static_cast<std::int32_t>(slot.extent_x_42);
        if (ent_min_x > raft_max_x || ent_max_x < raft_min_x) {
            continue;
        }
        const std::int32_t raft_min_y =
            static_cast<std::int32_t>(slot.pos_y) - static_cast<std::int32_t>(slot.extent_y_44);
        const std::int32_t raft_max_y =
            static_cast<std::int32_t>(slot.pos_y) + static_cast<std::int32_t>(slot.extent_y_44);
        if (ent_min_y > raft_max_y || ent_max_y < raft_min_y) {
            continue;
        }
        return true;
    }
    return false;
}

bool probe_entity_water_surface(
    WaterProbeEntity& entity,
    std::uint8_t tile_attr,
    bool on_floating_platform) noexcept
{
    // 0x00E36E..0x00E392: clear bit 3; set bit 3 iff (attr & 0x90) == 0x80 && !platform.
    auto flags = static_cast<std::uint8_t>(entity.flags_36 & ~kWaterProbeFlags36WaterBit);
    const bool immersed = is_water_surface_tile_attr(tile_attr) && !on_floating_platform;
    if (immersed) {
        flags = static_cast<std::uint8_t>(flags | kWaterProbeFlags36WaterBit);
    }
    entity.flags_36 = flags;
    return immersed;
}

bool probe_water_surface_with_aux(
    WaterProbeEntity& entity,
    std::span<const std::uint8_t> aux_map,
    std::uint16_t word_stride_shift,
    std::span<const WaterPlatformSlot> spark_slots) noexcept
{
    entity.flags_36 = static_cast<std::uint8_t>(entity.flags_36 & ~kWaterProbeFlags36WaterBit);

    const std::int16_t tile_x = static_cast<std::int16_t>(entity.pos_x >> 3);
    const std::int16_t tile_y = static_cast<std::int16_t>(entity.pos_y >> 3);
    if (tile_x < 0 || tile_y < 0 || word_stride_shift >= 16U) {
        return false;
    }

    const std::uint64_t idx =
        (static_cast<std::uint64_t>(tile_y) << word_stride_shift) +
        static_cast<std::uint64_t>(tile_x);
    if (idx >= aux_map.size()) {
        return false;
    }

    const std::uint8_t tile_attr = aux_map[static_cast<std::size_t>(idx)];
    const bool on_platform = is_entity_on_water_platform(entity, spark_slots);
    return probe_entity_water_surface(entity, tile_attr, on_platform);
}

bool step_entity_lifecycle_timer_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept
{
    const auto off = normalize_ram_offset(entity_ram_addr, 4U, ram.size());
    if (!off.has_value()) {
        return false;
    }

    LifecycleTimerEntity entity{};
    entity.status = static_cast<std::int16_t>(read_be16(ram, *off + kTimerEntityStatusOffset));
    entity.timer = read_be16(ram, *off + kTimerEntityCounterOffset);

    const bool expired = step_entity_lifecycle_timer(entity);
    write_be16(ram, *off + kTimerEntityCounterOffset, entity.timer);
    if (expired) {
        write_be16(ram, *off + kTimerEntityStatusOffset, static_cast<std::uint16_t>(entity.status));
    }
    return expired;
}

std::uint16_t prng_step_16_from_ram(std::span<std::uint8_t> ram) noexcept {
    if (ram.size() < kPrngSeedStateRamOffset + 2U) {
        return 0U;
    }
    std::uint16_t seed = read_be16(ram, kPrngSeedStateRamOffset);
    const std::uint16_t next = prng_step_16(seed);
    write_be16(ram, kPrngSeedStateRamOffset, seed);
    return next;
}

std::uint16_t prng_range_word_from_ram(
    std::span<std::uint8_t> ram,
    std::uint16_t limit) noexcept
{
    if (ram.size() < kPrngSeedStateRamOffset + 2U) {
        return 0U;
    }
    std::uint16_t seed = read_be16(ram, kPrngSeedStateRamOffset);
    const std::uint16_t value = prng_range_word(seed, limit);
    write_be16(ram, kPrngSeedStateRamOffset, seed);
    return value;
}

std::uint8_t prng_range_byte_from_ram(
    std::span<std::uint8_t> ram,
    std::uint16_t limit) noexcept
{
    if (ram.size() < kPrngSeedStateRamOffset + 2U) {
        return 0U;
    }
    std::uint16_t seed = read_be16(ram, kPrngSeedStateRamOffset);
    const std::uint8_t value = prng_range_byte(seed, limit);
    write_be16(ram, kPrngSeedStateRamOffset, seed);
    return value;
}

bool probe_entity_water_surface_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept
{
    const auto ent_off = normalize_ram_offset(entity_ram_addr, 0x46U, ram.size());
    if (!ent_off.has_value() || ram.size() < 0x10000U) {
        return false;
    }

    WaterProbeEntity entity{};
    entity.pos_x = static_cast<std::int16_t>(read_be16(ram, *ent_off + kProbeEntityPosXOffset));
    entity.pos_y = static_cast<std::int16_t>(read_be16(ram, *ent_off + kProbeEntityPosYOffset));
    entity.flags_36 = ram[*ent_off + kProbeEntityFlags36Offset];
    entity.extent_x_42 =
        static_cast<std::int16_t>(read_be16(ram, *ent_off + kProbeEntityExtentXOffset));
    entity.extent_y_44 =
        static_cast<std::int16_t>(read_be16(ram, *ent_off + kProbeEntityExtentYOffset));

    // 0x009C44..0x009C54: $FF16F0 bit 7 chooses Plane A aux ($FF1766) vs Plane B aux ($FF1770).
    const bool use_plane_b =
        (ram[kWaterProbeScrollModeRamOffset] & kWaterProbePlaneBSelectBit) != 0U;
    const std::size_t aux_ctx_off = use_plane_b
        ? kWaterProbeAuxPlaneBRamOffset
        : kWaterProbeAuxPlaneARamOffset;

    const std::uint32_t aux_ptr = read_be32(ram, aux_ctx_off + 0x00U);
    const auto aux_off = (aux_ptr != 0U)
        ? normalize_ram_offset(aux_ptr, 1U, ram.size())
        : std::optional<std::size_t>{};
    if (!aux_off.has_value()) {
        entity.flags_36 = static_cast<std::uint8_t>(entity.flags_36 & ~kWaterProbeFlags36WaterBit);
        ram[*ent_off + kProbeEntityFlags36Offset] = entity.flags_36;
        return false;
    }

    const std::uint16_t word_stride_shift = read_be16(ram, aux_ctx_off + 0x08U);

    // 0x009C80..0x009CF6: load 6 spark/platform slots at $FF2D8C (stride 0x5A).
    std::array<WaterPlatformSlot, kWaterProbeSparkPoolCount> spark_slots{};
    for (std::size_t i = 0U; i < kWaterProbeSparkPoolCount; ++i) {
        const std::size_t slot_off =
            kWaterProbeSparkPoolRamOffset + (i * kWaterProbeSparkSlotStride);
        spark_slots[i].type = static_cast<std::int16_t>(read_be16(ram, slot_off + 0x00U));
        spark_slots[i].pos_x = static_cast<std::int16_t>(read_be16(ram, slot_off + 0x08U));
        spark_slots[i].pos_y = static_cast<std::int16_t>(read_be16(ram, slot_off + 0x0CU));
        spark_slots[i].extent_x_42 =
            static_cast<std::int16_t>(read_be16(ram, slot_off + 0x42U));
        spark_slots[i].extent_y_44 =
            static_cast<std::int16_t>(read_be16(ram, slot_off + 0x44U));
    }

    const bool immersed = probe_water_surface_with_aux(
        entity,
        std::span<const std::uint8_t>(ram).subspan(*aux_off),
        word_stride_shift,
        spark_slots);
    ram[*ent_off + kProbeEntityFlags36Offset] = entity.flags_36;
    return immersed;
}

} // namespace thor::physics
