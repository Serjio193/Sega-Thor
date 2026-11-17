#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::player {

struct EntitySimConfig {
    static constexpr std::uint32_t kFreezeSlowmoAddr = 0x008F22U;
    static constexpr std::uint32_t kFreezeSlowmoEndAddr = 0x008F84U;
    static constexpr std::uint32_t kAnimFrameStepperAddr = 0x008F84U;
    static constexpr std::uint32_t kAnimFrameStepperEndAddr = 0x009040U;
    static constexpr std::uint32_t kTerrainPreflightAddr = 0x009040U;
    static constexpr std::uint32_t kTerrainPreflightEndAddr = 0x009084U;

    static constexpr std::uint32_t kGlobalFlagsRam = 0x00FF17B8U;
    static constexpr std::uint8_t kGlobalSlowmoBit = 0U;
    static constexpr std::uint8_t kGlobalFreezeBit = 1U;

    static constexpr std::uint32_t kLayerSelectRam = 0x00FF16F0U;
    static constexpr std::uint8_t kLayerSelectBit = 7U;
    static constexpr std::uint32_t kLayer1TilemapRam = 0x00FF1766U;
    static constexpr std::uint32_t kLayer2TilemapRam = 0x00FF1770U;

    // Slot Offsets
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kPosXOffset = 0x08U;
    static constexpr std::size_t kAnimTablePtrOffset = 0x26U;
    static constexpr std::size_t kStateOffset = 0x2AU;
    static constexpr std::size_t kPrevStateOffset = 0x2CU;
    static constexpr std::size_t kKeyframeTimerOffset = 0x2EU;
    static constexpr std::size_t kFrameOffsetOffset = 0x30U;
    static constexpr std::size_t kSpriteAssetOffset = 0x32U;
    static constexpr std::size_t kFlags37Offset = 0x37U;
    static constexpr std::size_t kFlags38Offset = 0x38U;
    static constexpr std::size_t kCollision40Offset = 0x40U;
    static constexpr std::size_t kCollision41Offset = 0x41U;
    static constexpr std::size_t kVelXOffset = 0x72U;
    static constexpr std::size_t kVelYOffset = 0x76U;
    static constexpr std::size_t kSubtimer9cOffset = 0x9CU;
    static constexpr std::size_t kSubtimer9dOffset = 0x9DU;

    // Flags 37 bits
    static constexpr std::uint8_t kFlagAnimLockBit = 0U;
    static constexpr std::uint8_t kFlagLoopEndTriggerBit = 1U;
    static constexpr std::uint8_t kFlagLoopMarkBit = 2U;
    static constexpr std::uint8_t kFlagIntangibleBit = 4U;

    // Flags 38 bits
    static constexpr std::uint8_t kFlagFlyingBit = 1U;

    // Move code for +X
    static constexpr std::uint8_t kMoveCodePosX = 0x11U;

    // Branch targets
    static constexpr std::uint32_t kBypassTarget = 0x009382U;
    static constexpr std::uint32_t kBranchFlying = 0x009708U;
    static constexpr std::uint32_t kBranchSweepY = 0x0091F6U;
    static constexpr std::uint32_t kBranchSweepNegX = 0x009128U;
};

// 1. Simulation Freeze & Slow-Motion Handler (0x008F22..0x008F84)
struct FreezeSlowmoResult {
    bool frozen{false};
    bool skip_anim{false};
    std::int32_t vel_x{0};
    std::int32_t vel_y{0};
};

FreezeSlowmoResult step_entity_freeze_slowmo(
    std::span<std::uint8_t> slot,
    bool global_freeze,
    bool global_slowmo) noexcept;

// 2. Animation Frame Stepper (0x008F84..0x009040)
struct AnimKeyframe {
    std::uint16_t sprite_id{0U};
    std::uint16_t raw_duration{0U}; // bit 15: loop marker
};

void step_entity_animation(
    std::span<std::uint8_t> slot,
    std::span<const std::uint8_t> anim_data) noexcept;

std::vector<std::uint8_t> pack_anim_sequence(
    std::span<const AnimKeyframe> keyframes);

// 3. Terrain Collision Pre-flight (0x009040..0x009084)
enum class TerrainPreflightAction : std::uint8_t {
    BypassIntangible,
    Flying,
    SweepY,
    SweepNegX,
    SweepPosX,
};

struct TerrainPreflightResult {
    TerrainPreflightAction action{TerrainPreflightAction::SweepY};
    std::uint32_t layer_tilemap_ram{0U};
    std::uint8_t move_code{0U};
};

TerrainPreflightResult preflight_terrain_collision(
    std::span<std::uint8_t> slot,
    bool layer_select_bit7) noexcept;

} // namespace oasis::game::player
