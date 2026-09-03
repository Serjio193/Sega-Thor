#pragma once

#include "game/entities/entity_pool.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

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

} // namespace oasis::game::spirits
