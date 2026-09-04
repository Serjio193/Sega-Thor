#pragma once

#include "tools/re_reachable_closure.hpp"

namespace oasis::tools {

struct BoundedStackAnalysis {
    ClosureReason reason{ClosureReason::other};
    std::optional<std::uint32_t> value;
    std::vector<ClosureDefinition> definitions;
    std::vector<ClosureDefinition> provenance;
    std::string status{"not_applicable"};
    std::optional<std::uint32_t> a7_before;
    std::uint32_t a7_increment_bytes{};
};

[[nodiscard]] BoundedStackAnalysis analyze_bounded_movea_postincrement(
    const DecodedSlice& slice, std::uint32_t instruction_address, std::uint8_t destination_register);

} // namespace oasis::tools
