#pragma once

#include <cstdint>
#include <optional>

namespace oasis::game::scripts {

inline constexpr std::uint32_t kObservedEventRouterAddress = 0x00007A28;
inline constexpr std::uint32_t kObservedEventProducerAddress = 0x000082AE;
inline constexpr std::uint32_t kObservedEventCodeRamAddress = 0x00FF1976;
inline constexpr std::uint32_t kObservedEventWordRamAddress = 0x00FF1978;
inline constexpr std::uint32_t kObservedEventLongRamAddress = 0x00FF197A;
inline constexpr std::uint32_t kObservedEventSourcePoolAddress = 0x00FF19E8;
inline constexpr std::uint16_t kObservedEventSourceType = 0x0008;
inline constexpr std::uint32_t kObservedExternalDriverAddress = 0x00060004;
inline constexpr std::uint16_t kObservedDriverFirstSelector = 0x0006;
inline constexpr std::uint16_t kObservedDriverSecondSelector = 0x0008;
inline constexpr std::uint16_t kObservedDriverSentinel = 0x01FF;
inline constexpr std::uint32_t kObservedDriverStateRamAddress = 0x00FF17B8;
inline constexpr std::uint16_t kObservedDriverStateAndMask = 0xFFF9;
inline constexpr std::uint32_t kObservedDriverSourceRamAddress = 0x00FF0D7E;
inline constexpr std::uint32_t kObservedRouteFlagClearAddress = 0x00007B2A;
inline constexpr std::uint32_t kObservedRouteFallbackAddress = 0x00007A6C;

struct ObservedEventSource {
    // Raw fields read from the selected FF19E8 record by 0x82AE.
    std::uint16_t raw_type{};
    std::uint16_t raw_32{};
    std::uint8_t raw_52{};
    std::uint16_t raw_04{};
    std::int32_t raw_4e{};
};

struct ObservedEventTransfer {
    std::uint16_t raw_event_code{};
    std::uint16_t raw_event_word{};
    std::int32_t raw_event_long{};
    bool source_type_cleared{};
};

struct ObservedEventRoute {
    bool eligible{};
    std::uint32_t handler_address{};
};

struct ObservedFlagClearTrace {
    std::uint16_t first_selector{};
    std::uint16_t first_result_masked{};
    bool first_result_was_sentinel{};
    bool second_call_issued{};
    std::uint16_t second_selector{};
    std::uint16_t state_and_mask{};
    std::uint32_t record_field_source{};
    std::uint16_t timeout_field_value{};
};

// Mirrors the type-8 producer at 0x82AE after its bounded pool query has
// selected a source record. The source type is cleared as part of the write.
[[nodiscard]] std::optional<ObservedEventTransfer> produce_observed_event(
    const ObservedEventSource& source) noexcept;

// Mirrors the raw 0x7B2A branch after the first 0x60004 call. The external
// helper's meaning and returned value are intentionally not modeled.
[[nodiscard]] constexpr ObservedFlagClearTrace trace_flag_clear_handler(
    std::uint16_t first_driver_result) noexcept {
    const auto masked_result = static_cast<std::uint16_t>(
        first_driver_result & kObservedDriverSentinel);
    if (masked_result != kObservedDriverSentinel) {
        return {.first_selector = kObservedDriverFirstSelector,
                .first_result_masked = masked_result,
                .first_result_was_sentinel = false};
    }
    return {.first_selector = kObservedDriverFirstSelector,
            .first_result_masked = masked_result,
            .first_result_was_sentinel = true,
            .second_call_issued = true,
            .second_selector = kObservedDriverSecondSelector,
            .state_and_mask = kObservedDriverStateAndMask,
            .record_field_source = kObservedDriverSourceRamAddress,
            .timeout_field_value = 0xFFFF};
}

// Mirrors the bounded dispatch split at 0x7A28. Handler addresses remain raw
// because their event or dialogue meanings are not yet proven.
[[nodiscard]] constexpr ObservedEventRoute route_observed_event(
    std::uint16_t raw_event_code,
    std::uint8_t flags_37) noexcept {
    if ((flags_37 & 0x02U) == 0) {
        return {true, kObservedRouteFlagClearAddress};
    }
    if (raw_event_code == 0 || raw_event_code > 0x3FU) {
        return {true, kObservedRouteFallbackAddress};
    }
    if (raw_event_code <= 0x0CU) {
        return {true, 0x00007B64};
    }
    if (raw_event_code <= 0x10U) {
        return {true, 0x00007BD4};
    }
    if (raw_event_code == 0x11U) {
        return {true, 0x00007BF6};
    }
    if (raw_event_code <= 0x15U) {
        return {true, 0x00007BA4};
    }
    if (raw_event_code <= 0x19U) {
        return {true, 0x00007BE8};
    }
    return {true, 0x00007B84};
}

} // namespace oasis::game::scripts
