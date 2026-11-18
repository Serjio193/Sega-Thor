#include "game/combat/hit_spark_generator.hpp"

namespace oasis::game::combat {

namespace {

[[nodiscard]] inline std::uint16_t read_be16(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1U >= buf.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8U) |
        static_cast<std::uint16_t>(buf[offset + 1U]));
}

[[nodiscard]] inline std::int16_t read_be16_signed(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    return static_cast<std::int16_t>(read_be16(buf, offset));
}

inline void write_be16(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t value) noexcept {
    if (offset + 1U < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
    }
}

inline void write_be16_signed(std::span<std::uint8_t> buf, std::size_t offset, std::int16_t value) noexcept {
    write_be16(buf, offset, static_cast<std::uint16_t>(value));
}

[[nodiscard]] inline std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        return static_cast<std::size_t>(addr - 0x00FF0000U);
    }
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

} // namespace

SpawnSparkResult spawn_hit_spark(
    const AttackHitbox3D& hitbox,
    const SparkTargetState& target,
    std::span<HitSparkParticle, 4> slots) noexcept {
    // 0x00C012..0x00C036: find first free slot with slot[0] == 0
    std::size_t free_slot = HitSparkGeneratorConfig::kSlotCount;
    for (std::size_t i = 0U; i < HitSparkGeneratorConfig::kSlotCount; ++i) {
        if (slots[i].type == 0U) {
            free_slot = i;
            break;
        }
    }

    if (free_slot >= HitSparkGeneratorConfig::kSlotCount) {
        // All 4 slots are occupied: exit immediately with RTS (drop particle on saturated buffer)
        return SpawnSparkResult{
            .spawned = false,
            .slot_index = HitSparkGeneratorConfig::kSlotCount,
            .midpoint = {},
            .particle = {}
        };
    }

    // 0x00C038..0x00C0B6: calculate 3D overlap midpoint and project to screen coordinates
    const SparkMidpointResult midpoint = compute_spark_midpoints(hitbox, target);

    HitSparkParticle particle{
        .type = HitSparkGeneratorConfig::kSparkType,         // 2 (0x00C03C)
        .x = midpoint.projected_x,                          // mid_x (0x00C0AE)
        .y = midpoint.projected_y,                          // mid_y - mid_z (0x00C0B4)
        .lifetime = HitSparkGeneratorConfig::kDefaultLifetime // 5 frames (0x00C040)
    };

    slots[free_slot] = particle;

    return SpawnSparkResult{
        .spawned = true,
        .slot_index = free_slot,
        .midpoint = midpoint,
        .particle = particle
    };
}

void update_hit_sparks(std::span<HitSparkParticle, 4> slots) noexcept {
    for (auto& slot : slots) {
        if (slot.is_active()) {
            --slot.lifetime;
            if (slot.lifetime == 0U) {
                slot.type = 0U; // Free slot when lifetime expires
            }
        }
    }
}

HitSparkParticle read_hit_spark_particle_raw(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index) {
    if (slot_index >= HitSparkGeneratorConfig::kSlotCount) {
        return HitSparkParticle{};
    }

    const std::uint32_t addr = HitSparkGeneratorConfig::kSparkRingBufferRam +
        static_cast<std::uint32_t>(slot_index * HitSparkGeneratorConfig::kSlotSizeBytes);
    const std::size_t off = to_ram_offset(addr);
    if (off + HitSparkGeneratorConfig::kSlotSizeBytes > ram.size()) {
        return HitSparkParticle{};
    }

    return HitSparkParticle{
        .type = read_be16(ram, off + HitSparkGeneratorConfig::kSparkTypeOffset),
        .x = read_be16_signed(ram, off + HitSparkGeneratorConfig::kSparkXOffset),
        .y = read_be16_signed(ram, off + HitSparkGeneratorConfig::kSparkYOffset),
        .lifetime = read_be16(ram, off + HitSparkGeneratorConfig::kSparkLifetimeOffset)
    };
}

void write_hit_spark_particle_raw(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const HitSparkParticle& particle) {
    if (slot_index >= HitSparkGeneratorConfig::kSlotCount) {
        return;
    }

    const std::uint32_t addr = HitSparkGeneratorConfig::kSparkRingBufferRam +
        static_cast<std::uint32_t>(slot_index * HitSparkGeneratorConfig::kSlotSizeBytes);
    const std::size_t off = to_ram_offset(addr);
    if (off + HitSparkGeneratorConfig::kSlotSizeBytes > ram.size()) {
        return;
    }

    write_be16(ram, off + HitSparkGeneratorConfig::kSparkTypeOffset, particle.type);
    write_be16_signed(ram, off + HitSparkGeneratorConfig::kSparkXOffset, particle.x);
    write_be16_signed(ram, off + HitSparkGeneratorConfig::kSparkYOffset, particle.y);
    write_be16(ram, off + HitSparkGeneratorConfig::kSparkLifetimeOffset, particle.lifetime);
}

SparkTargetState read_spark_target_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t target_addr) {
    const std::size_t off = to_ram_offset(target_addr);
    if (off + 0x4CU > ram.size()) {
        return SparkTargetState{.address = target_addr};
    }

    return SparkTargetState{
        .address = target_addr,
        .world_x = read_be16_signed(ram, off + HitSparkGeneratorConfig::kTargetWorldXOffset),
        .world_y = read_be16_signed(ram, off + HitSparkGeneratorConfig::kTargetWorldYOffset),
        .elev_z = read_be16_signed(ram, off + HitSparkGeneratorConfig::kTargetElevZOffset),
        .ground_h = read_be16_signed(ram, off + HitSparkGeneratorConfig::kTargetGroundHOffset),
        .rad_x = read_be16_signed(ram, off + HitSparkGeneratorConfig::kTargetRadXOffset),
        .rad_y = read_be16_signed(ram, off + HitSparkGeneratorConfig::kTargetRadYOffset),
        .height_z = read_be16_signed(ram, off + HitSparkGeneratorConfig::kTargetHeightZOffset)
    };
}

SpawnSparkResult spawn_hit_spark_raw(
    std::span<std::uint8_t> ram,
    const AttackHitbox3D& hitbox,
    std::uint32_t target_addr) {
    const SparkTargetState target = read_spark_target_raw(ram, target_addr);

    std::array<HitSparkParticle, 4> slots{};
    for (std::size_t i = 0U; i < HitSparkGeneratorConfig::kSlotCount; ++i) {
        slots[i] = read_hit_spark_particle_raw(ram, i);
    }

    const SpawnSparkResult result = spawn_hit_spark(hitbox, target, slots);
    if (result.spawned) {
        write_hit_spark_particle_raw(ram, result.slot_index, result.particle);
    }
    return result;
}

} // namespace oasis::game::combat
