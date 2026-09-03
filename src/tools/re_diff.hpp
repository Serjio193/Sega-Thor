#pragma once

#include "core/rom_identity.hpp"
#include "tools/re_program.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class MatchKind { exact_match, structural_match, changed_blocks, unmatched };

struct DifferentialTarget {
    std::uint32_t entry{};
    std::size_t byte_budget{};
    std::optional<std::uint32_t> confirmed_end;
};

struct ChangedBlock {
    std::size_t ordinal{};
    std::optional<std::uint32_t> retail_start;
    std::optional<std::uint32_t> beta_start;
};

struct AnalogCandidate {
    std::uint32_t beta_entry{};
    MatchKind match{MatchKind::unmatched};
    std::size_t matching_instructions{};
    std::size_t matching_blocks{};
    std::vector<ChangedBlock> changed_blocks;
    std::vector<std::string> beta_normalized_opcode_signature;
};

struct TargetComparison {
    DifferentialTarget target;
    std::uint32_t retail_range_end{};
    std::uint32_t beta_same_address_range_end{};
    std::size_t retail_instructions{};
    std::size_t retail_basic_blocks{};
    std::vector<std::string> retail_normalized_opcode_signature;
    std::vector<std::string> beta_same_address_normalized_opcode_signature;
    MatchKind same_address_match{MatchKind::unmatched};
    std::vector<ChangedBlock> same_address_changed_blocks;
    std::vector<AnalogCandidate> analogs;
    std::size_t unmatched_retail_instructions{};
    std::size_t unmatched_beta_instructions{};
};

struct DifferentialReport {
    RomIdentity retail;
    RomIdentity beta;
    std::vector<TargetComparison> targets;
};

[[nodiscard]] std::string match_kind_name(MatchKind kind);

[[nodiscard]] DifferentialReport compare_m68k_revisions(
    std::span<const std::uint8_t> retail_rom,
    std::span<const std::uint8_t> beta_rom,
    std::span<const DifferentialTarget> targets);

[[nodiscard]] std::string diff_to_json(const DifferentialReport& report);
[[nodiscard]] std::string diff_to_text(const DifferentialReport& report);

} // namespace oasis::tools
