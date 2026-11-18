#include "game/world/entity_pool_alloc.hpp"

#include "game/graphics_decompress.hpp"
#include "game/player/entity_constructor_lifecycle.hpp"

#include <algorithm>

namespace thor::world {

namespace {

[[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

[[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void write_be16(std::span<std::uint8_t> data, std::size_t offset, std::uint16_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(std::span<std::uint8_t> data, std::size_t offset, std::uint32_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

[[nodiscard]] std::optional<std::size_t> find_first_zero_word(
    std::span<const std::uint16_t> slot_types) noexcept {
    for (std::size_t i = 0U; i < slot_types.size(); ++i) {
        if (slot_types[i] == 0U) {
            return i;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> find_first_zero_slot_in_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t pool_ram_offset,
    std::size_t slot_count,
    std::uint32_t stride) noexcept {
    for (std::size_t i = 0U; i < slot_count; ++i) {
        const std::size_t off = static_cast<std::size_t>(pool_ram_offset) + (i * stride);
        if (off + 2U > ram.size()) {
            return std::nullopt;
        }
        if (read_be16(ram, off) == 0U) {
            return i;
        }
    }
    return std::nullopt;
}

[[nodiscard]] GeneralEntitySlotResult build_general_slot_result(
    std::uint32_t pool_ram_addr,
    std::size_t slot_index,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f) noexcept {
    const std::uint32_t slot_addr =
        pool_ram_addr + static_cast<std::uint32_t>(slot_index) * kGeneralPoolStride;
    bool cleared = false;
    if (enemy_hud_target_ff1850 == slot_addr) {
        enemy_hud_timer_ff184f = 0U;
        cleared = true;
    }
    return {slot_addr, slot_addr - kPlayerEntityRamAddr, static_cast<std::uint8_t>(slot_index), cleared};
}

void initialize_spawn_npc_slot(
    std::span<std::uint8_t> slot_bytes,
    std::span<const std::uint8_t> rom) noexcept {
    if (slot_bytes.size() < kGeneralPoolStride) {
        return;
    }

    // 0x00DAA0..0x00DAAA: type = 0x002E, construct_entity (0x008D06), timer (+0x02) = 0x003C
    if (!rom.empty()) {
        const auto desc = oasis::game::player::read_entity_descriptor(rom, kSpawnNpcEntityType, true);
        if (desc.has_value()) {
            oasis::game::player::construct_entity(slot_bytes, kSpawnNpcEntityType, *desc, false);
            write_be16(slot_bytes, 0x02U, kSpawnNpcInitialTimer);
            return;
        }
    }

    std::fill(slot_bytes.begin() + 2U, slot_bytes.begin() + kGeneralPoolStride, std::uint8_t{0U});
    write_be16(slot_bytes, 0x00U, kSpawnNpcEntityType);
    write_be16(slot_bytes, 0x2CU, 0xFFFFU);
    write_be16(slot_bytes, 0x34U, 0xFFFFU);
    write_be16(slot_bytes, 0x5CU, 0xFFFFU);
    write_be16(slot_bytes, 0x02U, kSpawnNpcInitialTimer);
}

[[nodiscard]] std::optional<GeneralEntitySlotResult> alloc_spawn_npc_in_pool(
    std::span<std::array<std::uint8_t, kGeneralPoolStride>> pool,
    std::uint32_t pool_ram_addr,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f,
    std::span<const std::uint8_t> rom) noexcept {
    for (std::size_t i = 0U; i < pool.size(); ++i) {
        if (read_be16(pool[i], 0x00U) == 0U) {
            auto res = build_general_slot_result(
                pool_ram_addr, i, enemy_hud_target_ff1850, enemy_hud_timer_ff184f);
            initialize_spawn_npc_slot(pool[i], rom);
            return res;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<GeneralEntitySlotResult> alloc_spawn_npc_in_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t pool_ram_addr,
    std::uint32_t pool_ram_offset,
    std::size_t slot_count,
    std::span<const std::uint8_t> rom) noexcept {
    const auto idx = find_first_zero_slot_in_ram(ram, pool_ram_offset, slot_count, kGeneralPoolStride);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    const std::size_t slot_offset = static_cast<std::size_t>(pool_ram_offset) + (*idx * kGeneralPoolStride);
    if (slot_offset + kGeneralPoolStride > ram.size()) {
        return std::nullopt;
    }
    const std::uint32_t hud_target = read_be32(ram, kEnemyHudTargetPtrRamOffset);
    auto res = build_general_slot_result(pool_ram_addr, *idx, hud_target, ram[kEnemyHudTimerRamOffset]);
    initialize_spawn_npc_slot(ram.subspan(slot_offset, kGeneralPoolStride), rom);
    return res;
}

[[nodiscard]] DecompressVramDmaResult decode_to_staging(
    std::span<const std::uint8_t> compressed_stream,
    std::uint16_t vram_dest,
    std::span<std::uint8_t> staging_buffer) noexcept {
    DecompressVramDmaResult result{};
    if (compressed_stream.empty() || staging_buffer.empty()) {
        return result;
    }
    try {
        const auto decoded = oasis::game::decompress_graphics(compressed_stream, staging_buffer);
        if (decoded.output_size == 0U) {
            return result;
        }
        result.ok = true;
        result.source_consumed = decoded.source_consumed;
        result.output_bytes = decoded.output_size;
        result.length_words = static_cast<std::uint16_t>(decoded.output_size >> 1U);
        result.dma_command = {kDecompressDmaBusSource, vram_dest, result.length_words};
    } catch (...) {
        return DecompressVramDmaResult{};
    }
    return result;
}

} // namespace

DecompressVramDmaResult decompress_palette_block(
    std::span<const std::uint8_t> compressed_stream,
    std::uint16_t vram_dest,
    std::span<std::uint8_t> staging_buffer,
    std::vector<VramDmaTransferCommand>& dma_queue,
    std::uint8_t& vint_sync_flags,
    const VintSyncCallback& vint_callback) {
    const DecompressVramDmaResult result = decode_to_staging(compressed_stream, vram_dest, staging_buffer);
    if (!result.ok) {
        return result;
    }
    dma_queue.push_back(result.dma_command);
    vint_sync_flags = static_cast<std::uint8_t>(vint_sync_flags | kVdpDmaSyncBit);
    if (vint_callback) {
        vint_callback(vint_sync_flags);
    }
    return result;
}

DecompressVramDmaResult decompress_palette_block_from_ram(
    std::span<const std::uint8_t> compressed_stream,
    std::uint16_t vram_dest,
    std::span<std::uint8_t, 0x10000> ram,
    bool clear_vint_sync) noexcept {
    const std::uint32_t queue_ptr = read_be32(ram, kVdpDmaQueuePtrRamOffset);
    const std::size_t queue_offset = static_cast<std::size_t>(queue_ptr & 0xFFFFU);
    if (queue_offset > ram.size() - kVdpDmaQueueEntryBytes) {
        return DecompressVramDmaResult{};
    }
    const DecompressVramDmaResult result = decode_to_staging(
        compressed_stream,
        vram_dest,
        ram.subspan(kDecompressDmaStagingRamOffset, kDecompressDmaStagingMaxBytes));
    if (!result.ok) {
        return result;
    }
    write_be32(ram, queue_offset + 0U, result.dma_command.src_bus_addr);
    write_be16(ram, queue_offset + 4U, result.dma_command.vram_dest);
    write_be16(ram, queue_offset + 6U, result.dma_command.length_words);
    write_be32(ram, kVdpDmaQueuePtrRamOffset, queue_ptr + static_cast<std::uint32_t>(kVdpDmaQueueEntryBytes));
    ram[kVdpVintSyncFlagsRamOffset] = static_cast<std::uint8_t>(ram[kVdpVintSyncFlagsRamOffset] | kVdpDmaSyncBit);
    if (clear_vint_sync) {
        ram[kVdpVintSyncFlagsRamOffset] = static_cast<std::uint8_t>(
            ram[kVdpVintSyncFlagsRamOffset] & static_cast<std::uint8_t>(~kVdpDmaSyncBit));
    }
    return result;
}

std::optional<ProjectileSlotAllocation> alloc_projectile_entity_slot(
    std::span<const std::uint16_t, kProjectilePoolSlots> slot_types) noexcept {
    const auto idx = find_first_zero_word(slot_types);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return ProjectileSlotAllocation{
        kProjectilePoolRamAddr + static_cast<std::uint32_t>(*idx) * kProjectilePoolStride,
        static_cast<std::uint16_t>(kProjectileBaseTileIndex + static_cast<std::uint16_t>(*idx) * kProjectileTileStep),
        static_cast<std::uint8_t>(*idx),
    };
}

std::optional<ProjectileSlotAllocation> alloc_projectile_entity_slot_from_ram(
    std::span<const std::uint8_t, 0x10000> ram) noexcept {
    const auto idx = find_first_zero_slot_in_ram(
        ram, kProjectilePoolRamOffset, kProjectilePoolSlots, kProjectilePoolStride);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return ProjectileSlotAllocation{
        kProjectilePoolRamAddr + static_cast<std::uint32_t>(*idx) * kProjectilePoolStride,
        static_cast<std::uint16_t>(kProjectileBaseTileIndex + static_cast<std::uint16_t>(*idx) * kProjectileTileStep),
        static_cast<std::uint8_t>(*idx),
    };
}

std::optional<SparkSlotAllocation> alloc_spark_effect_slot(
    std::span<const std::uint16_t, kSparkPoolSlots> slot_types) noexcept {
    const auto idx = find_first_zero_word(slot_types);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return SparkSlotAllocation{
        kSparkPoolRamAddr + static_cast<std::uint32_t>(*idx) * kSparkPoolStride,
        static_cast<std::uint8_t>(*idx),
    };
}

std::optional<SparkSlotAllocation> alloc_spark_effect_slot_from_ram(
    std::span<const std::uint8_t, 0x10000> ram) noexcept {
    const auto idx = find_first_zero_slot_in_ram(
        ram, kSparkPoolRamOffset, kSparkPoolSlots, kSparkPoolStride);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return SparkSlotAllocation{
        kSparkPoolRamAddr + static_cast<std::uint32_t>(*idx) * kSparkPoolStride,
        static_cast<std::uint8_t>(*idx),
    };
}

std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_18(
    std::span<const std::uint16_t, kGeneralPool18Slots> slot_types,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f) noexcept {
    const auto idx = find_first_zero_word(slot_types);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return build_general_slot_result(kGeneralPool18RamAddr, *idx, enemy_hud_target_ff1850, enemy_hud_timer_ff184f);
}

std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_17(
    std::span<const std::uint16_t, kGeneralPool17Slots> slot_types,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f) noexcept {
    const auto idx = find_first_zero_word(slot_types);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return build_general_slot_result(kGeneralPool17RamAddr, *idx, enemy_hud_target_ff1850, enemy_hud_timer_ff184f);
}

std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_18_from_ram(
    std::span<std::uint8_t, 0x10000> ram) noexcept {
    const auto idx = find_first_zero_slot_in_ram(
        ram, kGeneralPool18RamOffset, kGeneralPool18Slots, kGeneralPoolStride);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return build_general_slot_result(
        kGeneralPool18RamAddr, *idx, read_be32(ram, kEnemyHudTargetPtrRamOffset), ram[kEnemyHudTimerRamOffset]);
}

std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_17_from_ram(
    std::span<std::uint8_t, 0x10000> ram) noexcept {
    const auto idx = find_first_zero_slot_in_ram(
        ram, kGeneralPool17RamOffset, kGeneralPool17Slots, kGeneralPoolStride);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return build_general_slot_result(
        kGeneralPool17RamAddr, *idx, read_be32(ram, kEnemyHudTargetPtrRamOffset), ram[kEnemyHudTimerRamOffset]);
}

std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_18(
    std::span<std::array<std::uint8_t, kGeneralPoolStride>, kGeneralPool18Slots> pool,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f,
    std::span<const std::uint8_t> rom) noexcept {
    return alloc_spawn_npc_in_pool(pool, kGeneralPool18RamAddr, enemy_hud_target_ff1850, enemy_hud_timer_ff184f, rom);
}

std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_17(
    std::span<std::array<std::uint8_t, kGeneralPoolStride>, kGeneralPool17Slots> pool,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f,
    std::span<const std::uint8_t> rom) noexcept {
    return alloc_spawn_npc_in_pool(pool, kGeneralPool17RamAddr, enemy_hud_target_ff1850, enemy_hud_timer_ff184f, rom);
}

std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_18_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::span<const std::uint8_t> rom) noexcept {
    return alloc_spawn_npc_in_ram(ram, kGeneralPool18RamAddr, kGeneralPool18RamOffset, kGeneralPool18Slots, rom);
}

std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_17_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::span<const std::uint8_t> rom) noexcept {
    return alloc_spawn_npc_in_ram(ram, kGeneralPool17RamAddr, kGeneralPool17RamOffset, kGeneralPool17Slots, rom);
}

SoundCommandDispatchResult sound_play_music_track(
    std::int16_t& active_track_ff16f2,
    std::uint16_t track_cmd,
    const SoundDriverCallback& driver_cb) {
    active_track_ff16f2 = static_cast<std::int16_t>(track_cmd);
    if (driver_cb) {
        driver_cb(track_cmd);
    }
    return {SoundDispatchAction::PlayTrack, true, track_cmd, active_track_ff16f2};
}

SoundCommandDispatchResult sound_resume_music_track(
    std::int16_t active_track_ff16f2,
    const SoundDriverCallback& driver_cb) {
    if (active_track_ff16f2 < 0) {
        return {SoundDispatchAction::ResumeTrack, false, 0U, active_track_ff16f2};
    }
    const auto cmd = static_cast<std::uint16_t>(active_track_ff16f2);
    if (driver_cb) {
        driver_cb(cmd);
    }
    return {SoundDispatchAction::ResumeTrack, true, cmd, active_track_ff16f2};
}

SoundCommandDispatchResult sound_pause_music_track(
    std::int16_t& active_track_ff16f2,
    const SoundDriverCallback& driver_cb) {
    active_track_ff16f2 = kSoundTrackStopped;
    if (driver_cb) {
        driver_cb(kSoundCmdPause);
    }
    return {SoundDispatchAction::PauseTrack, true, kSoundCmdPause, active_track_ff16f2};
}

SoundCommandDispatchResult sound_stop_music_track(
    std::int16_t& active_track_ff16f2,
    const SoundDriverCallback& driver_cb) {
    active_track_ff16f2 = kSoundTrackStopped;
    if (driver_cb) {
        driver_cb(kSoundCmdStop);
    }
    return {SoundDispatchAction::StopTrack, true, kSoundCmdStop, active_track_ff16f2};
}

SoundCommandDispatchResult sound_play_music_track_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint16_t track_cmd,
    const SoundDriverCallback& driver_cb) {
    auto active_track = static_cast<std::int16_t>(read_be16(ram, kActiveMusicTrackRamOffset));
    const auto res = sound_play_music_track(active_track, track_cmd, driver_cb);
    write_be16(ram, kActiveMusicTrackRamOffset, static_cast<std::uint16_t>(active_track));
    return res;
}

SoundCommandDispatchResult sound_resume_music_track_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    const SoundDriverCallback& driver_cb) {
    const auto active_track = static_cast<std::int16_t>(read_be16(ram, kActiveMusicTrackRamOffset));
    return sound_resume_music_track(active_track, driver_cb);
}

SoundCommandDispatchResult sound_pause_music_track_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const SoundDriverCallback& driver_cb) {
    auto active_track = static_cast<std::int16_t>(read_be16(ram, kActiveMusicTrackRamOffset));
    const auto res = sound_pause_music_track(active_track, driver_cb);
    write_be16(ram, kActiveMusicTrackRamOffset, static_cast<std::uint16_t>(active_track));
    return res;
}

SoundCommandDispatchResult sound_stop_music_track_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const SoundDriverCallback& driver_cb) {
    auto active_track = static_cast<std::int16_t>(read_be16(ram, kActiveMusicTrackRamOffset));
    const auto res = sound_stop_music_track(active_track, driver_cb);
    write_be16(ram, kActiveMusicTrackRamOffset, static_cast<std::uint16_t>(active_track));
    return res;
}

} // namespace thor::world
