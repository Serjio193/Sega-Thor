#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and ROM addresses for Master SAT Sprite Dispatcher (0x00A196..0x00A2EE).
struct MasterSatDispatcherConfig {
    static constexpr std::uint32_t kMasterSatDispatcherAddr = 0x00A196U;
    static constexpr std::uint32_t kMasterSatDispatcherEnd = 0x00A2EEU;
    static constexpr std::uint32_t kPreCompositionHelperAddr = 0x00A342U;
    static constexpr std::uint32_t kDemoSpriteHookAddr = 0x00A336U;

    // RAM variables
    static constexpr std::uint32_t kCompositeFlagRam = 0x00FF1651U;
    static constexpr std::uint32_t kDemoActiveFlagRam = 0x00FF1996U;
    static constexpr std::uint32_t kForegroundPlayerFlagRam = 0x00FF1995U;
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kEntityDoneMaskRam = 0x00FF188EU;
    static constexpr std::uint32_t kVdpSyncFlagsRam = 0x00FF164EU;
    static constexpr std::uint8_t kVdpSyncMask = 0x03U;

    // Multi-pool definitions
    static constexpr std::uint32_t kGroup1PoolRam = 0x00FF2954U;
    static constexpr std::size_t kGroup1SlotCount = 4U;
    static constexpr std::size_t kGroup1SlotStride = 0x5AU; // 90 bytes

    static constexpr std::uint32_t kSpecialPoolRam = 0x00FF19C8U;
    static constexpr std::size_t kSpecialSlotCount = 4U;
    static constexpr std::size_t kSpecialSlotStride = 0x08U; // 8 bytes

    static constexpr std::uint32_t kGroup2PoolRam = 0x00FF19E8U;
    static constexpr std::size_t kGroup2SlotCount = 21U;
    static constexpr std::size_t kGroup2SlotStride = 0xBCU; // 188 bytes

    static constexpr std::uint32_t kGroup4PoolRam = 0x00FF2ABCU;
    static constexpr std::size_t kGroup4SlotCount = 8U;
    static constexpr std::size_t kGroup4SlotStride = 0x5AU; // 90 bytes

    static constexpr std::uint32_t kGroup3PoolRam = 0x00FF2D8CU;
    static constexpr std::size_t kGroup3SlotCount = 6U;
    static constexpr std::size_t kGroup3SlotStride = 0x5AU; // 90 bytes

    // Field offsets within entity structures
    static constexpr std::size_t kTypeOffset = 0x00U;       // int16
    static constexpr std::size_t kYOffset = 0x0CU;          // int16
    static constexpr std::size_t kFlags36Offset = 0x36U;    // uint8

    // SAT target entrypoints
    static constexpr std::uint32_t kNormalSatTarget = 0x00AD76U;
    static constexpr std::uint32_t kCustomSatTarget = 0x00A9A0U;
    static constexpr std::uint32_t kNormalSatForegroundTarget = 0x00AD80U;
    static constexpr std::uint32_t kCustomSatForegroundTarget = 0x00A9AAU;
    static constexpr std::uint32_t kSpecialPoolJumpTableTarget = 0x00A33CU;

    // Sorting sentinel in M68K: moveq #-1, d5
    static constexpr std::int16_t kInitialMaxYSentinel = -1;
};

/// SAT dispatch destination routine.
enum class SatDispatchTarget : std::uint32_t {
    NormalSat = MasterSatDispatcherConfig::kNormalSatTarget,
    CustomSat = MasterSatDispatcherConfig::kCustomSatTarget,
    NormalSatForeground = MasterSatDispatcherConfig::kNormalSatForegroundTarget,
    CustomSatForeground = MasterSatDispatcherConfig::kCustomSatForegroundTarget,
    SpecialJumpTable = MasterSatDispatcherConfig::kSpecialPoolJumpTableTarget,
};

/// Pool kind identifier in dispatch sequence.
enum class SatPoolKind : std::uint8_t {
    Group1HighPriority = 0, // $FF2954, 4 slots
    SpecialHitSparks = 1,   // $FF19C8, 4 slots
    Group2Entities = 2,     // $FF19E8, 21 slots (Y-depth sorted)
    Group4GroundShadow = 3, // $FF2ABC, 8 slots
    Group3Floor = 4,        // $FF2D8C, 6 slots
    ForegroundPlayer = 5,   // $FF19E8[0] override
};

/// Entity slot descriptor participating in SAT composition.
struct SatEntitySlot {
    std::int16_t type{0};
    std::int16_t y{0};
    std::uint8_t flags_36{0U};
};

/// Emitted dispatch action representing one dispatched sprite entity.
struct SatDispatchEntry {
    SatPoolKind pool{SatPoolKind::Group1HighPriority};
    std::size_t slot_index{0U};
    std::int16_t type{0};
    std::int16_t y{0};
    std::uint8_t flags_36{0U};
    SatDispatchTarget target{SatDispatchTarget::NormalSat};
    std::uint32_t ram_addr{0U};
};

/// Result of running the master SAT sprite composition pass.
struct MasterSatDispatchResult {
    bool composite_flag_cleared{false};
    bool demo_branch_taken{false};
    std::uint32_t entity_done_mask{0U};
    std::vector<SatDispatchEntry> entries;
    bool sat_terminated{false};
    std::uint16_t terminated_sat_offset{0U};
    std::uint8_t vdp_sync_flags{0U};
};

/// Builds the 21-bit inactive entity mask in d7 as implemented in 0x00A1D6..0x00A1F6.
/// Bit i is set if group2[i].type == 0.
[[nodiscard]] std::uint32_t build_group2_inactive_mask(
    std::span<const SatEntitySlot> group2) noexcept;

/// Sorts active Group 2 entities in descending Y order (0x00A1F6..0x00A246).
/// Returns list of slot indices in render order.
[[nodiscard]] std::vector<std::size_t> sort_group2_y_depth_order(
    std::span<const SatEntitySlot> group2,
    std::uint32_t* out_done_mask = nullptr);

/// Runs high-level Master SAT Sprite Dispatcher pass (0x00A196..0x00A2EE).
[[nodiscard]] MasterSatDispatchResult dispatch_master_sat_pass(
    bool demo_active,
    bool foreground_player_override,
    std::span<const SatEntitySlot> group1,
    std::span<const SatEntitySlot> special,
    std::span<const SatEntitySlot> group2,
    std::span<const SatEntitySlot> group4,
    std::span<const SatEntitySlot> group3,
    std::uint16_t current_sat_offset = 0U,
    std::uint8_t initial_vdp_sync_flags = 0U);

/// Runs Master SAT Sprite Dispatcher against raw Mega Drive RAM byte buffer.
[[nodiscard]] MasterSatDispatchResult dispatch_master_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
