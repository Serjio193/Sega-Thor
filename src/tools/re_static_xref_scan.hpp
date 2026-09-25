#pragma once

#include "tools/re_slice_decoder.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

struct StaticXrefSpan {
    std::uint32_t start{};
    std::uint32_t end{};
    bool operator==(const StaticXrefSpan&) const = default;
};

struct StaticXrefCandidate {
    std::uint32_t caller_pc{};
    std::uint32_t caller_end{};
    std::uint16_t opcode{};
    std::string mnemonic;
    FlowKind flow{FlowKind::none};
    std::uint8_t condition_code{0xFF};
    std::uint32_t target_pc{};
    StaticXrefSpan source_span;
    StaticXrefSpan target_component;
    bool target_is_component_entry{};
    bool fallthrough{};
};

struct StaticXrefRangeResult {
    StaticXrefSpan span;
    std::uint32_t decoded_end{};
    std::size_t instruction_count{};
    std::string stop_reason;
};

struct StaticXrefScanResult {
    std::vector<StaticXrefCandidate> candidates;
    std::vector<StaticXrefRangeResult> ranges;
    std::size_t decoded_instruction_count{};
    std::size_t decoded_byte_count{};
};

[[nodiscard]] StaticXrefScanResult scan_static_xrefs(
    std::span<const std::uint8_t> rom,
    std::span<const StaticXrefSpan> verified_asm,
    std::span<const StaticXrefSpan> target_components);

[[nodiscard]] std::string static_xref_kind(const StaticXrefCandidate& candidate);

} // namespace oasis::tools
