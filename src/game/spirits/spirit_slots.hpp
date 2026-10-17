#pragma once

#include "game/entities/entity_pool.hpp"
#include "game/entities/screen_entity.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::player {
struct PlayerState;
}

namespace oasis::game::spirits {

inline constexpr std::uint32_t kSpiritSlotFlagsRamAddress = 0x00FF0DBA;
inline constexpr std::uint32_t kSpiritDispatchGuardRamAddress = 0x00FF0DC4;
inline constexpr std::uint32_t kSpiritEventBase = 0x16;
inline constexpr std::size_t kSpiritSlotCount = 4;

inline constexpr std::uint32_t kObservedDispatchRoutineAddress = 0x00031B80;
inline constexpr std::uint32_t kObservedResourceDispatchAddress = 0x0000C2EC;
inline constexpr std::uint32_t kObservedEffectQueueAddress = 0x0000CA24;
inline constexpr std::uint32_t kObservedOwnerCallbackAddress = 0x00017A96;
inline constexpr std::uint32_t kObservedTargetQueryAddress = 0x00017CA6;
inline constexpr std::uint32_t kObservedSummonEntryAddress = 0x00007A10;
inline constexpr std::uint32_t kObservedSummonInitializerAddress = 0x0000846C;
inline constexpr std::uint32_t kObservedSummonRecordAddress = 0x00FF1AA4;
inline constexpr std::uint32_t kObservedOwnerPoolAddress = 0x00FF2D8C;
inline constexpr std::uint32_t kObservedTargetPoolAddress = 0x00FF19E8;
inline constexpr std::uint16_t kObservedResourceSelector = 0x13;
inline constexpr std::uint16_t kObservedEffectSelector = 0x15;
inline constexpr std::uint16_t kObservedSummonType = 0x16;
inline constexpr std::uint16_t kObservedSummonDepth = 0x13;
inline constexpr std::uint16_t kObservedSummonResource = 0x04F8;
inline constexpr std::uint8_t kObservedDispatchSlot = 1;
inline constexpr std::uint8_t kObservedDispatchGuardBit = 0;

class SpiritSlots {
public:
    explicit constexpr SpiritSlots(std::uint8_t acquired_mask = 0) noexcept
        : acquired_mask_(acquired_mask & 0x0FU) {}

    [[nodiscard]] constexpr std::uint8_t acquired_mask() const noexcept {
        return acquired_mask_;
    }

    [[nodiscard]] constexpr bool acquired(std::uint8_t slot) const noexcept {
        return slot < kSpiritSlotCount &&
               (acquired_mask_ & static_cast<std::uint8_t>(1U << slot)) != 0;
    }

    // The event handler at 0x7BE8 maps 0x16..0x19 to bits 0..3.
    constexpr void apply_event(std::uint8_t event_code) noexcept {
        if (event_code < kSpiritEventBase ||
            event_code >= kSpiritEventBase + kSpiritSlotCount) {
            return;
        }
        acquired_mask_ |= static_cast<std::uint8_t>(
            1U << (event_code - kSpiritEventBase));
    }

private:
    std::uint8_t acquired_mask_{};
};

struct SpiritDispatchInput {
    // These are the two raw input bytes read from entity +0x40/+0x41 by
    // 0x31B80. Their controller-button meanings remain unassigned.
    std::uint8_t input_40{};
    std::uint8_t input_41{};
    std::uint8_t slot_flags{};
    std::uint8_t guard_flags{};
};

struct SpiritDispatchTrace {
    bool entered_observed_path{};
    bool slot_available{};
    std::uint8_t guard_flags{};
    std::array<std::uint16_t, 2> selectors{};
    std::size_t selector_count{};
};

struct ObservedSummonCaller {
    // Raw fields read by the 0x7A10 gate and 0x846C initializer. Their game
    // meanings remain unassigned.
    std::uint16_t state_30{};
    std::uint8_t flags_37{};
    std::uint16_t position_x{};
    std::uint16_t position_y{};
    std::uint8_t raw_17{};
    std::uint16_t raw_14{};
};

struct ObservedSummonSeed {
    std::uint16_t raw_type{};
    std::uint16_t position_x{};
    std::uint16_t position_y{};
    std::uint16_t raw_depth{};
    std::uint8_t raw_66{};
    std::uint16_t raw_14{};
    std::uint16_t raw_resource_18{};
    std::uint16_t raw_resource_5a{};
    std::uint32_t raw_a6{};
    std::uint32_t raw_aa{};
};

struct ObservedTargetQuery {
    std::size_t owner_index{};
    std::int16_t relative_x_min{-10};
    std::int16_t relative_y_min{-6};
    std::int16_t relative_x_max{10};
    std::int16_t relative_y_max{6};
    std::int16_t relative_z_min{};
    std::int16_t relative_z_max{4};
    std::uint16_t required_type{0x16};
};

// Mirrors the first-match behavior of 0xB922 followed by the type check at
// 0x17CE4. A later record is not considered after the first spatial match.
[[nodiscard]] std::optional<std::size_t> find_observed_target(
    const entities::EntityPoolView& owner_pool,
    const entities::EntityPoolView& target_pool,
    const ObservedTargetQuery& query) noexcept;

[[nodiscard]] SpiritDispatchTrace trace_observed_dispatch(
    const SpiritDispatchInput& input) noexcept;

// Mirrors the caller gate at 0x7A10 and the field writes in 0x846C. The
// later table-derived velocity writes are intentionally not represented.
[[nodiscard]] std::optional<ObservedSummonSeed> initialize_observed_summon(
    const ObservedSummonCaller& caller) noexcept;

inline constexpr std::uint16_t kSpiritTypeDytto = 0x0018;
inline constexpr std::uint16_t kSpiritTypeEfreet = 0x001A;
inline constexpr std::uint16_t kSpiritTypeBow = 0x001C;
inline constexpr std::uint16_t kSpiritTypeShade = 0x001E;

inline constexpr std::uint16_t kSpiritStateAppear = 26; // 0x1A in ROM
inline constexpr std::uint16_t kSpiritStateFollow = 2;  // 0x02 in ROM
inline constexpr std::uint16_t kSpiritStateHover = 0;   // 0x00 in ROM
inline constexpr std::uint16_t kSpiritStateDespawn = 28; // 0x1C in ROM

inline constexpr std::uint16_t kSpiritStateBubbleCast = 14; // 0x0E in ROM (0x011DA8)
inline constexpr std::uint16_t kSpiritStateBubbleFly = 16;  // 0x10 in ROM (0x011E54)
inline constexpr std::uint16_t kSpiritStateBubblePop = 22;  // 0x16 in ROM (0x011F8A)

inline constexpr std::uint16_t kSpiritAnimAppear = 34; // 0x22 in ROM
inline constexpr std::uint16_t kSpiritAnimHover = 4;   // 0x04 in ROM
inline constexpr std::uint16_t kSpiritAnimBubbleCast = 6;   // 0x06 in ROM (0x011DAE)
inline constexpr std::uint16_t kSpiritAnimBubble = 26;      // 0x1A in ROM (0x011E5A)
inline constexpr std::uint16_t kSpiritAnimBubblePop = 36;   // 0x24 in ROM (0x011F90)

inline constexpr std::uint16_t kSpiritBubbleSpCost = 5;     // 5 SP (ROM 0x011D8A)
inline constexpr std::uint16_t kSpiritBubbleLifetime = 24;  // 24 frames (ROM 0x011F6A)
inline constexpr std::int32_t kSpiritBubbleSpeed = 4;       // 4.0 px/frame cardinal (ROM 0x011E9C)
inline constexpr std::uint16_t kSpiritHoldThreshold = 20;   // 20 frames (ROM 0x011AE8)
inline constexpr std::uint8_t kSpiritDoubleTapWindow = 6;   // 6 frames (ROM 0x011AFA)

inline constexpr std::uint16_t kSpiritSpCostInterval = 45; // 0x2D frames per SP (ROM 0x011B72)

inline constexpr std::uint32_t kSpiritDyttoGfxDescriptor = 0x0011F360;
inline constexpr std::uint32_t kSpiritDyttoAnimDescriptor = 0x000405E2;
inline constexpr std::uint32_t kSpiritBubbleGfxDescriptor = 0x00141374;
inline constexpr std::uint32_t kSpiritBubbleFrame0 = 0x00141412;
inline constexpr std::uint32_t kSpiritSpawnRoutineDytto = 0x00014668;
inline constexpr std::uint32_t kSpiritSummonRoutineDytto = 0x00018CDE;
inline constexpr std::uint32_t kSpiritUpdateRoutineDytto = 0x00011B4E;
inline constexpr std::uint32_t kSpiritCommandInputRoutine = 0x00011A4C;
inline constexpr std::uint32_t kSpiritCommandDispatchRoutine = 0x000121B4;
inline constexpr std::uint32_t kSpiritBubbleCastRoutine = 0x00011D8A;
inline constexpr std::uint32_t kSpiritBubbleFlyRoutine = 0x00011ED2;
inline constexpr std::uint32_t kSpiritRepositionRoutine = 0x0003A464;

struct ActiveSpirit {
    bool active{false};
    std::uint16_t type_id{0};          // 0x0018 for Dytto
    std::uint8_t slot{0};              // 0 for Dytto (0..3)
    std::uint16_t state{0};            // 26: appear, 2: follow, 14: bubble cast, 28: despawn
    std::uint16_t anim_selector{0};    // 34: appear, 4: hover, 6: bubble cast
    std::int32_t world_x{0};
    std::int32_t world_y{0};
    std::int32_t world_z{0};
    std::int32_t target_x{0};
    std::int32_t target_y{0};
    std::uint8_t direction{2};         // 0: Up, 1: Right, 2: Down, 3: Left
    std::uint16_t state_timer{0};      // timer in current state
    std::uint16_t sp_timer{kSpiritSpCostInterval}; // 45 frames
    std::vector<entities::SpritePiece> pieces{};
    std::uint16_t tile_base{0};
    bool halted{false};

    // Dytto command & ability state (ROM 0x011A4C..0x011B26, 0x011D8A..0x011F98)
    std::uint8_t last_command_code{0};       // 0: none, 1: tap (water bubble), 2: double-tap, 3: hold
    std::uint16_t last_command_sp_cost{0};   // 5 SP for water bubble (ROM 0x011D8A)
    std::uint16_t command_state{0};          // 14: bubble cast, 16: fly, 22: pop
    std::uint16_t command_timer{0};          // duration timer in command state
    std::uint8_t command_direction{2};       // direction of cast / projectile
    bool command_active{false};              // true while casting or projectile active
    std::uint16_t button_a_hold_timer{0};    // hold duration counter ($ff199e in ROM)
    std::uint8_t double_tap_timer{0};        // double-tap window timer ($ff19a1 in ROM, 6 frames)
    bool prev_button_a{false};               // previous Button::A state ($ff19a0 in ROM)

    // Water Bubble effect entity (Type 0x0018, $A2 = 0xFF in ROM 0x011E04..0x011F98)
    bool projectile_active{false};
    std::uint16_t projectile_state{0};       // 16: fly, 22: pop
    std::int32_t projectile_x{0};
    std::int32_t projectile_y{0};
    std::int32_t projectile_z{0};
    std::int32_t projectile_vx{0};
    std::int32_t projectile_vy{0};
    std::uint16_t projectile_lifetime{0};    // 24 frames (0x18 in ROM 0x011F6A)
    std::vector<entities::SpritePiece> projectile_pieces{};
    std::uint16_t projectile_tile_base{0};
    std::uint16_t target_entity_hit{0};      // type_id of entity struck (e.g. 0x0048)
};

bool try_summon_spirit(
    ActiveSpirit& spirit,
    const SpiritSlots& slots,
    std::uint8_t slot_id,
    std::int32_t px,
    std::int32_t py,
    std::int32_t pz,
    player::PlayerState& player,
    std::span<const std::uint8_t> rom) noexcept;

void update_active_spirit(
    ActiveSpirit& spirit,
    const player::PlayerState& player,
    player::PlayerState& mutable_player) noexcept;

std::uint8_t decode_dytto_command_input(
    ActiveSpirit& spirit,
    bool button_a_pressed) noexcept;

bool try_execute_dytto_command(
    ActiveSpirit& spirit,
    std::uint8_t command_code,
    player::PlayerState& player,
    std::span<const std::uint8_t> rom) noexcept;

void update_dytto_ability(
    ActiveSpirit& spirit,
    std::span<const std::uint8_t> rom,
    std::vector<entities::ScreenEntity>* entities = nullptr) noexcept;

void cleanup_spirit_ability(ActiveSpirit& spirit) noexcept;

void reposition_spirit_on_screen_transition(
    ActiveSpirit& spirit,
    std::int32_t px,
    std::int32_t py,
    std::int32_t pz = 0) noexcept;

void despawn_spirit(ActiveSpirit& spirit) noexcept;

} // namespace oasis::game::spirits
