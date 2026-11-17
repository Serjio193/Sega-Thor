#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace oasis::game::player {

struct AnimPaletteEntityStepperConfig {
    static constexpr std::uint32_t kAnimPaletteSequencerAddr = 0x008E22U;
    static constexpr std::uint32_t kAnimPaletteSequencerEndAddr = 0x008E8EU;
    static constexpr std::uint32_t kEntityMasterStepperAddr = 0x008E90U;
    static constexpr std::uint32_t kEntityMasterStepperEndAddr = 0x008EF6U;
    static constexpr std::uint32_t kEntityStateResetAddr = 0x008EF8U;
    static constexpr std::uint32_t kEntityStateResetEndAddr = 0x008F10U;
    static constexpr std::uint32_t kSecondaryEntityDispatchAddr = 0x008F12U;
    static constexpr std::uint32_t kSecondaryEntityDispatchEndAddr = 0x008F22U;

    static constexpr std::uint32_t kAnimPaletteActiveRam = 0x00FF164FU;
    static constexpr std::uint32_t kAnimPaletteTablePtrRam = 0x00FF17A2U;
    static constexpr std::uint32_t kCramBufferRam = 0x00FF134CU;
    static constexpr std::uint32_t kCramSyncFlagRam = 0x00FF164DU;
    static constexpr std::uint8_t kCramSyncBit = 0U;

    static constexpr std::uint32_t kRecoveryVectorRam = 0x00FF193CU;

    // Group 1: 4 slots x 90B at $FF2954
    static constexpr std::uint32_t kGroup1BaseRam = 0x00FF2954U;
    static constexpr std::size_t kGroup1Slots = 4U;
    static constexpr std::size_t kGroup1Stride = 90U;
    static constexpr std::uint32_t kGroup1RecoveryVector = 0x00008EAAU;
    static constexpr std::uint32_t kGroup1Handler = 0x00008F12U;

    // Group 2: 21 slots x 188B at $FF19E8
    static constexpr std::uint32_t kGroup2BaseRam = 0x00FF19E8U;
    static constexpr std::size_t kGroup2Slots = 21U;
    static constexpr std::size_t kGroup2Stride = 188U;
    static constexpr std::uint32_t kGroup2RecoveryVector = 0x00008ECCU;
    static constexpr std::uint32_t kGroup2Handler = 0x00008F22U;

    // Group 3: 6 slots x 90B at $FF2D8C
    static constexpr std::uint32_t kGroup3BaseRam = 0x00FF2D8CU;
    static constexpr std::size_t kGroup3Slots = 6U;
    static constexpr std::size_t kGroup3Stride = 90U;
    static constexpr std::uint32_t kGroup3RecoveryVector = 0x00008EEEU;
    static constexpr std::uint32_t kGroup3Handler = 0x00008F12U;

    // Entity State Reset
    static constexpr std::size_t kStateOffset = 0x2AU;
    static constexpr std::size_t kTimerOffset = 0x2CU;
    static constexpr std::uint16_t kTimerInitVal = 0xFFFFU;
    static constexpr std::size_t kFlags37Offset = 0x37U;
    static constexpr std::uint8_t kFlags37Mask = 0xF9U;
    static constexpr std::size_t kSubtimer30Offset = 0x30U;
    static constexpr std::size_t kSubtimer2eOffset = 0x2EU;

    // Secondary Entity Dispatch
    static constexpr std::uint8_t kSecondaryActiveBit = 4U;
    static constexpr std::uint32_t kSecondaryScriptEngineAddr = 0x0039CAU;
    static constexpr std::uint32_t kSecondaryBranchTarget = 0x008FA4U;
};

// 1. Animated Palette CRAM Sequencer (0x008E22..0x008E8E)
[[nodiscard]] bool step_anim_palette_raw(
    bool active,
    std::span<std::uint8_t> table,
    std::span<std::uint8_t> cram_buffer) noexcept;

struct AnimPaletteFrame {
    std::uint16_t duration{0U};
    std::uint16_t color{0U};
};

std::vector<std::uint8_t> create_anim_palette_entry(
    std::uint16_t target_cram_offset,
    std::uint16_t initial_frame_offset,
    std::uint16_t initial_timer,
    std::span<const AnimPaletteFrame> frames,
    std::uint16_t stride_to_next = 0U);

// 2. Entity State Reset Helper (0x008EF8..0x008F10)
void reset_entity_state(
    std::span<std::uint8_t> slot,
    std::uint16_t new_state) noexcept;

// 3. Secondary Entity Dispatch (0x008F12..0x008F22)
void dispatch_secondary_entity(
    std::span<std::uint8_t> slot) noexcept;

// 4. 3-Group Entity Master Stepper (0x008E90..0x008EF6)
struct EntityMasterStepperStats {
    std::uint32_t last_recovery_addr{0U};
    std::size_t group1_active_count{0U};
    std::size_t group2_active_count{0U};
    std::size_t group3_active_count{0U};
};

using SecondaryEntityHandler = std::function<void(std::size_t group_idx, std::size_t slot_idx, std::span<std::uint8_t> slot)>;
using PrimaryEntityHandler = std::function<void(std::size_t slot_idx, std::span<std::uint8_t> slot)>;

EntityMasterStepperStats step_entity_groups_raw(
    std::span<std::uint8_t> group1_mem,
    std::span<std::uint8_t> group2_mem,
    std::span<std::uint8_t> group3_mem,
    std::uint32_t& recovery_vector_ram,
    const SecondaryEntityHandler& on_secondary = {},
    const PrimaryEntityHandler& on_primary = {}) noexcept;

} // namespace oasis::game::player
