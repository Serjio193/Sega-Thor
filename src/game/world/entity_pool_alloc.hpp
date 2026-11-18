#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace thor::world {

// ROM entrypoints and callees (0x00D9A4..0x00DB1A, 374 bytes, 96 instructions).
inline constexpr std::uint32_t kDecompressPaletteBlockAddr = 0x00D9A4U;
inline constexpr std::uint32_t kAllocProjectileEntitySlotAddr = 0x00D9BEU;
inline constexpr std::uint32_t kAllocSparkEffectSlotAddr = 0x00D9F0U;
inline constexpr std::uint32_t kFindFreeGeneralEntitySlot18Addr = 0x00DA1AU;
inline constexpr std::uint32_t kFindFreeGeneralEntitySlot17Addr = 0x00DA2AU;
inline constexpr std::uint32_t kAllocSpawnNpcEntitySlot18Addr = 0x00DA62U;
inline constexpr std::uint32_t kAllocSpawnNpcEntitySlot17Addr = 0x00DA72U;
inline constexpr std::uint32_t kSoundPlayMusicTrackAddr = 0x00DAC2U;
inline constexpr std::uint32_t kSoundResumeMusicTrackAddr = 0x00DACEU;
inline constexpr std::uint32_t kSoundPauseMusicTrackAddr = 0x00DAE6U;
inline constexpr std::uint32_t kSoundStopMusicTrackAddr = 0x00DB00U;
inline constexpr std::uint32_t kEntityAllocSoundDispatchEndAddr = 0x00DB1AU;

// Helper callees invoked by M16.191 routines.
inline constexpr std::uint32_t kDecompressAndDmaHelperAddr = 0x0037D2U;
inline constexpr std::uint32_t kUniversalEntityConstructorAddr = 0x008D06U;
inline constexpr std::uint32_t kSoundDriverEntryAddr = 0x00060004U;

// Staging & VDP DMA parameters (0x00D9A4 -> 0x0037D2).
inline constexpr std::uint32_t kDecompressDmaStagingRamAddr = 0x00FF2FA8U;
inline constexpr std::uint32_t kDecompressDmaStagingRamOffset = 0x2FA8U;
inline constexpr std::size_t kDecompressDmaStagingMaxBytes = 0x1000U;
inline constexpr std::uint32_t kDecompressDmaBusSource = 0x007F97D4U; // $FF2FA8 >> 1
inline constexpr std::uint32_t kVdpDmaQueuePtrRamOffset = 0x1892U;
inline constexpr std::uint32_t kVdpBusyFlagsRamOffset = 0x164DU;
inline constexpr std::uint32_t kVdpVintSyncFlagsRamOffset = 0x164EU;
inline constexpr std::uint8_t kVdpBusyBit4 = 0x10U;
inline constexpr std::uint8_t kVdpDmaSyncBit = 0x02U;
inline constexpr std::size_t kVdpDmaQueueEntryBytes = 8U;

// Entity / effect RAM pools ($FF1C1C, $FF1CD8, $FF2ABC, $FF2D8C).
inline constexpr std::uint32_t kProjectilePoolRamAddr = 0x00FF2ABCU;
inline constexpr std::uint32_t kProjectilePoolRamOffset = 0x2ABCU;
inline constexpr std::size_t kProjectilePoolSlots = 8U;
inline constexpr std::uint32_t kProjectilePoolStride = 0x5AU; // 90 bytes
inline constexpr std::uint16_t kProjectileBaseTileIndex = 0x0694U;
inline constexpr std::uint16_t kProjectileTileStep = 9U;

inline constexpr std::uint32_t kSparkPoolRamAddr = 0x00FF2D8CU;
inline constexpr std::uint32_t kSparkPoolRamOffset = 0x2D8CU;
inline constexpr std::size_t kSparkPoolSlots = 6U;
inline constexpr std::uint32_t kSparkPoolStride = 0x5AU; // 90 bytes

inline constexpr std::uint32_t kGeneralPool18RamAddr = 0x00FF1C1CU;
inline constexpr std::uint32_t kGeneralPool18RamOffset = 0x1C1CU;
inline constexpr std::size_t kGeneralPool18Slots = 18U;
inline constexpr std::uint32_t kGeneralPool17RamAddr = 0x00FF1CD8U;
inline constexpr std::uint32_t kGeneralPool17RamOffset = 0x1CD8U;
inline constexpr std::size_t kGeneralPool17Slots = 17U;
inline constexpr std::uint32_t kGeneralPoolStride = 0xBCU; // 188 bytes

inline constexpr std::uint32_t kPlayerEntityRamAddr = 0x00FF19E8U;
inline constexpr std::uint32_t kPlayerEntityRamOffset = 0x19E8U;
inline constexpr std::uint32_t kEnemyHudTimerRamOffset = 0x184FU;
inline constexpr std::uint32_t kEnemyHudTargetPtrRamOffset = 0x1850U;
inline constexpr std::uint16_t kSpawnNpcEntityType = 0x002EU;
inline constexpr std::uint16_t kSpawnNpcInitialTimer = 0x003CU; // 60 frames

// Sound driver command registers & constants (0x00DAC2..0x00DB18).
inline constexpr std::uint32_t kActiveMusicTrackRamOffset = 0x16F2U;
inline constexpr std::uint16_t kSoundCmdStop = 0x0000U;
inline constexpr std::uint16_t kSoundCmdPause = 0x0007U;
inline constexpr std::int16_t kSoundTrackStopped = static_cast<std::int16_t>(-1);

using VintSyncCallback = std::function<void(std::uint8_t& vint_sync_flags)>;
using SoundDriverCallback = std::function<void(std::uint16_t cmd_d0)>;

/// 8-byte VDP DMA descriptor queued by 0x00D9A4 -> 0x0037D2 at ($FF1892).
struct VramDmaTransferCommand {
    std::uint32_t src_bus_addr{kDecompressDmaBusSource};
    std::uint16_t vram_dest{0U};
    std::uint16_t length_words{0U};

    [[nodiscard]] constexpr bool operator==(const VramDmaTransferCommand&) const noexcept = default;
};

/// Result of decompressing compressed graphics into $FF2FA8 and queueing VRAM DMA.
struct DecompressVramDmaResult {
    bool ok{false};
    std::size_t source_consumed{0U};
    std::size_t output_bytes{0U};
    std::uint16_t length_words{0U};
    VramDmaTransferCommand dma_command{};
};

/// Result of 0x00D9BE: first free 90-byte projectile/effect slot in $FF2ABC[0..7].
struct ProjectileSlotAllocation {
    std::uint32_t slot_ram_addr{0U};   // a6
    std::uint16_t vram_tile_index{0U}; // d0 = 0x0694 + 9 * slot_index
    std::uint8_t slot_index{0U};

    [[nodiscard]] constexpr bool operator==(const ProjectileSlotAllocation&) const noexcept = default;
};

/// Result of 0x00D9F0: first free 90-byte spark/effect slot in $FF2D8C[0..5].
struct SparkSlotAllocation {
    std::uint32_t slot_ram_addr{0U}; // a6
    std::uint8_t slot_index{0U};

    [[nodiscard]] constexpr bool operator==(const SparkSlotAllocation&) const noexcept = default;
};

/// Result of 0x00DA1A/0x00DA2A/0x00DA62/0x00DA72 on 188-byte entity pools ($FF1C1C/$FF1CD8).
struct GeneralEntitySlotResult {
    std::uint32_t slot_ram_addr{0U};     // $FF1C1C/$FF1CD8 + index * 0xBC
    std::uint32_t player_rel_offset{0U}; // d0 in 0x00DA62/72: slot_ram_addr - $FF19E8
    std::uint8_t slot_index{0U};
    bool cleared_enemy_hud_timer{false}; // true if $FF1850 == slot_ram_addr -> $FF184F = 0

    [[nodiscard]] constexpr bool operator==(const GeneralEntitySlotResult&) const noexcept = default;
};

enum class SoundDispatchAction : std::uint8_t {
    PlayTrack = 0,
    ResumeTrack = 1,
    PauseTrack = 2,
    StopTrack = 3,
};

struct SoundCommandDispatchResult {
    SoundDispatchAction action{SoundDispatchAction::PlayTrack};
    bool driver_called{false};
    std::uint16_t dispatched_cmd{0U};
    std::int16_t active_track_ff16f2{kSoundTrackStopped};

    [[nodiscard]] constexpr bool operator==(const SoundCommandDispatchResult&) const noexcept = default;
};

/// 0x00D9A4 -> 0x0037D2: Decompresses stream into staging_buffer ($FF2FA8), computes
/// length_words = output_bytes >> 1, queues VRAM DMA command, and signals $FF164E bit 1.
DecompressVramDmaResult decompress_palette_block(
    std::span<const std::uint8_t> compressed_stream,
    std::uint16_t vram_dest,
    std::span<std::uint8_t> staging_buffer,
    std::vector<VramDmaTransferCommand>& dma_queue,
    std::uint8_t& vint_sync_flags,
    const VintSyncCallback& vint_callback = {});

/// 64KB RAM window helper for 0x00D9A4 -> 0x0037D2 ($FF2FA8, $FF1892, $FF164E).
DecompressVramDmaResult decompress_palette_block_from_ram(
    std::span<const std::uint8_t> compressed_stream,
    std::uint16_t vram_dest,
    std::span<std::uint8_t, 0x10000> ram,
    bool clear_vint_sync = true) noexcept;

/// 0x00D9BE: Scans 8 slots ($FF2ABC, stride 0x5A) for +0x00 == 0 (read-only scan).
[[nodiscard]] std::optional<ProjectileSlotAllocation> alloc_projectile_entity_slot(
    std::span<const std::uint16_t, kProjectilePoolSlots> slot_types) noexcept;

[[nodiscard]] std::optional<ProjectileSlotAllocation> alloc_projectile_entity_slot_from_ram(
    std::span<const std::uint8_t, 0x10000> ram) noexcept;

/// 0x00D9F0: Scans 6 slots ($FF2D8C, stride 0x5A) for +0x00 == 0 (read-only scan).
[[nodiscard]] std::optional<SparkSlotAllocation> alloc_spark_effect_slot(
    std::span<const std::uint16_t, kSparkPoolSlots> slot_types) noexcept;

[[nodiscard]] std::optional<SparkSlotAllocation> alloc_spark_effect_slot_from_ram(
    std::span<const std::uint8_t, 0x10000> ram) noexcept;

/// 0x00DA1A / 0x00DA2A: Scans 18 slots ($FF1C1C) or 17 slots ($FF1CD8) for +0x00 == 0;
/// clears enemy_hud_timer_ff184f if slot_ram_addr == enemy_hud_target_ff1850.
[[nodiscard]] std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_18(
    std::span<const std::uint16_t, kGeneralPool18Slots> slot_types,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f) noexcept;

[[nodiscard]] std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_17(
    std::span<const std::uint16_t, kGeneralPool17Slots> slot_types,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f) noexcept;

[[nodiscard]] std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_18_from_ram(
    std::span<std::uint8_t, 0x10000> ram) noexcept;

[[nodiscard]] std::optional<GeneralEntitySlotResult> find_free_general_entity_slot_17_from_ram(
    std::span<std::uint8_t, 0x10000> ram) noexcept;

/// 0x00DA62 / 0x00DA72: Finds first free 188-byte slot, clears HUD timer on match,
/// sets +0x00 = 0x002E, runs constructor 0x008D06, sets +0x02 = 0x003C, and computes
/// relative offset from $FF19E8.
[[nodiscard]] std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_18(
    std::span<std::array<std::uint8_t, kGeneralPoolStride>, kGeneralPool18Slots> pool,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f,
    std::span<const std::uint8_t> rom = {}) noexcept;

[[nodiscard]] std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_17(
    std::span<std::array<std::uint8_t, kGeneralPoolStride>, kGeneralPool17Slots> pool,
    std::uint32_t enemy_hud_target_ff1850,
    std::uint8_t& enemy_hud_timer_ff184f,
    std::span<const std::uint8_t> rom = {}) noexcept;

[[nodiscard]] std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_18_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::span<const std::uint8_t> rom = {}) noexcept;

[[nodiscard]] std::optional<GeneralEntitySlotResult> alloc_spawn_npc_entity_slot_17_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::span<const std::uint8_t> rom = {}) noexcept;

/// 0x00DAC2..0x00DB18: Sound driver command dispatchers ($FF16F2, $060004).
SoundCommandDispatchResult sound_play_music_track(
    std::int16_t& active_track_ff16f2,
    std::uint16_t track_cmd,
    const SoundDriverCallback& driver_cb = {});

SoundCommandDispatchResult sound_resume_music_track(
    std::int16_t active_track_ff16f2,
    const SoundDriverCallback& driver_cb = {});

SoundCommandDispatchResult sound_pause_music_track(
    std::int16_t& active_track_ff16f2,
    const SoundDriverCallback& driver_cb = {});

SoundCommandDispatchResult sound_stop_music_track(
    std::int16_t& active_track_ff16f2,
    const SoundDriverCallback& driver_cb = {});

SoundCommandDispatchResult sound_play_music_track_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint16_t track_cmd,
    const SoundDriverCallback& driver_cb = {});

SoundCommandDispatchResult sound_resume_music_track_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    const SoundDriverCallback& driver_cb = {});

SoundCommandDispatchResult sound_pause_music_track_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const SoundDriverCallback& driver_cb = {});

SoundCommandDispatchResult sound_stop_music_track_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const SoundDriverCallback& driver_cb = {});

} // namespace thor::world

namespace oasis::game::world {
using namespace thor::world;
} // namespace oasis::game::world
