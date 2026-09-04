#include "tools/re_mass_verify.hpp"

#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

constexpr std::uint32_t kProbeExtra = 0x100U;
constexpr std::uint32_t kFallbackBudget = 0x100U;
constexpr std::uint32_t kConfirmed[] = {
    0x3820U, 0x7A28U, 0x82AEU, 0x8E90U, 0x938EU, 0x9BF2U,
    0xA6A4U, 0xD3B2U, 0x60004U, 0x604BCU, 0x6121AU};

bool has_flag(const CandidateRecord& record, const char* flag) {
    return std::find(record.source_flags.begin(), record.source_flags.end(), flag) !=
           record.source_flags.end();
}

bool is_data(const AtlasEntry& entry) {
    return entry.type == AtlasEntryType::data || entry.type == AtlasEntryType::table;
}

std::optional<std::pair<std::uint32_t, std::uint32_t>> interval(const AtlasEntry& entry) {
    const auto end = entry.end ? entry.end : entry.bounded_evidence_end;
    if (!end || *end <= entry.start) return std::nullopt;
    return std::pair{entry.start, *end};
}

bool overlaps(std::uint32_t start, std::uint32_t end, const AtlasEntry& entry) {
    const auto range = interval(entry);
    return range && range->first < end && range->second > start;
}

void add_reason(MassCandidate& item, const char* reason) {
    if (std::find(item.failure_reasons.begin(), item.failure_reasons.end(), reason) ==
        item.failure_reasons.end()) item.failure_reasons.emplace_back(reason);
}

bool in_set(std::span<const std::uint32_t> values, std::uint32_t value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

std::size_t caller_count(const CandidateRecord& record) {
    std::set<std::uint32_t> callers(record.ghidra_called_by.begin(), record.ghidra_called_by.end());
    if (record.known_direct_caller && callers.empty()) callers.insert(0xFFFFFFFFU);
    return callers.size();
}

bool known_transfer(const DecodedInstruction& instruction,
                    const std::set<std::uint32_t>& entries) {
    return instruction.direct_target &&
           (instruction.flow == FlowKind::direct_branch || instruction.flow == FlowKind::direct_jump) &&
           entries.contains(*instruction.direct_target);
}

MassBoundaryStatus boundary_status(const CandidateRecord& record, const DecodedSlice& slice,
                                   bool valid_range) {
    if (!record.ghidra_range_start || !record.ghidra_range_end) return MassBoundaryStatus::unknown;
    if (!valid_range || *record.ghidra_range_start != record.entry ||
        *record.ghidra_range_end <= record.entry) return MassBoundaryStatus::conflict;
    const auto start = *record.ghidra_range_start;
    const auto end = *record.ghidra_range_end;
    bool crosses = false;
    bool transfers_past = false;
    std::uint32_t max_end = start;
    for (const auto& instruction : slice.instructions) {
        max_end = std::max(max_end, instruction.address + static_cast<std::uint32_t>(instruction.bytes.size()));
        crosses = crosses || instruction.address < start || instruction.address >= end;
        transfers_past = transfers_past || (instruction.direct_target && *instruction.direct_target >= end);
    }
    if (crosses || transfers_past) return MassBoundaryStatus::longer;
    if (max_end < end) {
        const bool terminal = std::any_of(slice.instructions.begin(), slice.instructions.end(),
            [](const auto& item) { return item.flow == FlowKind::return_instruction; });
        if (terminal || !slice.unresolved_control_flow.empty() || !slice.unsupported_instruction_addresses.empty())
            return MassBoundaryStatus::shorter;
    }
    return max_end == end ? MassBoundaryStatus::agrees : MassBoundaryStatus::unknown;
}

StructuralClassification classify(const MassCandidate& item, const CandidateRecord& record) {
    if (item.known_data_overlap) return StructuralClassification::data_conflict;
    if (item.boundary_status == MassBoundaryStatus::conflict ||
        item.boundary_status == MassBoundaryStatus::longer)
        return StructuralClassification::boundary_conflict;
    if (item.unsupported_opcode_count || item.unsupported_addressing_count)
        return StructuralClassification::unsupported;
    if (!item.decode_ok || item.decode_conflict) return StructuralClassification::decode_failure;
    if (item.unresolved_indirect_flow) return StructuralClassification::indirect_flow;
    const bool terminal = item.reaches_rts || item.reaches_rte || item.ends_known_direct_transfer;
    const bool direct_support = item.direct_bsr_target || item.direct_jsr_target || item.known_static_target ||
                                item.vector_target || record.known_direct_caller;
    if (direct_support && terminal && item.boundary_status != MassBoundaryStatus::longer)
        return StructuralClassification::strong_static;
    if (terminal && (item.existing_beta_support || item.existing_dynamic_support ||
                     !record.ghidra_calls.empty() || !record.ghidra_called_by.empty()))
        return StructuralClassification::moderate_static;
    return StructuralClassification::weak_static;
}

void collect_reasons(MassCandidate& item, const CandidateRecord& record) {
    if (item.known_data_overlap) add_reason(item, "known_data_overlap");
    if (item.confirmed_code_overlap) add_reason(item, "confirmed_code_overlap");
    if (item.other_ghidra_overlap) add_reason(item, "multiple_entry_overlap");
    if (!item.decode_ok || item.decode_conflict) add_reason(item, "decode_failure");
    if (item.unsupported_opcode_count) add_reason(item, "unsupported_opcode");
    if (item.unsupported_addressing_count) add_reason(item, "unsupported_addressing");
    if (item.unresolved_indirect_flow) add_reason(item, "indirect_flow");
    if (item.boundary_status == MassBoundaryStatus::shorter) add_reason(item, "boundary_shorter_than_ghidra");
    if (item.boundary_status == MassBoundaryStatus::longer) add_reason(item, "boundary_longer_than_ghidra");
    if (item.boundary_status == MassBoundaryStatus::conflict) add_reason(item, "boundary_conflict");
    if (!(item.reaches_rts || item.reaches_rte || item.ends_known_direct_transfer))
        add_reason(item, "unknown_terminal");
    if (!record.known_direct_call_target && !record.known_direct_caller &&
        record.ghidra_calls.empty() && record.ghidra_called_by.empty())
        add_reason(item, "no_direct_xref");
    if (record.known_direct_caller && !record.ghidra_function) add_reason(item, "call_target_only");
}

std::string fix_class(const std::string& reason) {
    if (reason == "unsupported_opcode" || reason == "unsupported_addressing") return "decoder coverage";
    if (reason == "indirect_flow") return "bounded indirect-flow resolver";
    if (reason == "boundary_longer_than_ghidra" || reason == "boundary_shorter_than_ghidra" ||
        reason == "boundary_conflict" || reason == "multiple_entry_overlap") return "boundary continuation heuristic";
    if (reason == "known_data_overlap") return "data-region exclusion";
    if (reason == "no_direct_xref" || reason == "call_target_only") return "static edge recovery";
    if (reason == "unknown_terminal") return "terminal/return recognition";
    return reason;
}

std::string effort_for(const std::string& fix) {
    if (fix == "data-region exclusion" || fix == "terminal/return recognition") return "low";
    if (fix == "decoder coverage" || fix == "static edge recovery") return "medium";
    return "medium/high";
}

std::string risk_for(const std::string& fix) {
    if (fix == "data-region exclusion" || fix == "decoder coverage") return "low";
    if (fix == "terminal/return recognition") return "low/medium";
    return "medium";
}

std::string gain_for(std::size_t count, std::size_t total) {
    if (total && count * 10U >= total) return "high";
    if (total && count * 30U >= total) return "medium";
    return "low";
}

void aggregate(MassVerificationReport& report) {
    std::map<std::string, FailureCluster> clusters;
    for (const auto& item : report.candidates) {
        for (const auto& reason : item.failure_reasons) {
            auto& cluster = clusters[reason];
            cluster.reason = reason;
            ++cluster.count;
            if (cluster.sample_addresses.size() < 5U) cluster.sample_addresses.push_back(item.entry);
        }
        if (item.complexity != CandidateComplexity::leaf) continue;
        ++report.leaf.total;
        if (!item.unsupported_opcode_count && !item.unsupported_addressing_count &&
            !item.unresolved_indirect_flow && (item.reaches_rts || item.reaches_rte) &&
            (item.boundary_status == MassBoundaryStatus::agrees || item.boundary_status == MassBoundaryStatus::shorter) &&
            !item.known_data_overlap) ++report.leaf.clean;
        else if (item.unsupported_opcode_count || item.unsupported_addressing_count) ++report.leaf.unsupported;
        else if (item.unresolved_indirect_flow) ++report.leaf.indirect;
        else if (item.boundary_status == MassBoundaryStatus::conflict || item.boundary_status == MassBoundaryStatus::longer ||
                 item.boundary_status == MassBoundaryStatus::shorter) ++report.leaf.boundary_conflict;
        else if (!item.reaches_rts && !item.reaches_rte) ++report.leaf.terminal_failure;
        else ++report.leaf.other;
    }
    for (auto& [reason, cluster] : clusters) {
        cluster.systemic_fix_candidate = cluster.count > 1U;
        report.failure_clusters.push_back(std::move(cluster));
    }
    std::sort(report.failure_clusters.begin(), report.failure_clusters.end(), [](const auto& left, const auto& right) {
        return std::tie(right.count, left.reason) < std::tie(left.count, right.reason);
    });
    std::map<std::string, std::size_t> fix_counts;
    for (const auto& cluster : report.failure_clusters) fix_counts[fix_class(cluster.reason)] += cluster.count;
    for (const auto& [fix, count] : fix_counts)
        report.top_systemic_fixes.push_back({fix, count, effort_for(fix), risk_for(fix), gain_for(count, report.total_candidates)});
    std::sort(report.top_systemic_fixes.begin(), report.top_systemic_fixes.end(), [](const auto& left, const auto& right) {
        return std::tie(right.affected_count, left.blocker_class) < std::tie(left.affected_count, right.blocker_class);
    });
    if (report.top_systemic_fixes.size() > 3U) report.top_systemic_fixes.resize(3U);
}

} // namespace

MassVerificationReport verify_mass_structure(std::span<const std::uint8_t> rom,
                                             const CandidateMapReport& candidate_map,
                                             const AtlasReport& atlas) {
    if (rom.empty()) throw std::invalid_argument("mass verification requires a ROM span");
    MassVerificationReport report;
    report.candidate_map_schema = candidate_map.schema;
    report.ghidra_schema = candidate_map.ghidra_schema;
    report.ghidra_program = candidate_map.ghidra_program;
    report.total_candidates = candidate_map.candidates.size();
    std::set<std::uint32_t> entries;
    for (const auto& record : candidate_map.candidates) {
        if (!entries.insert(record.entry).second) throw std::invalid_argument("duplicate candidate entry");
    }
    for (const auto& record : candidate_map.candidates) {
        MassCandidate item{.entry = record.entry, .previous_classification = record.classification,
                           .complexity = record.complexity,
                           .direct_bsr_target = has_flag(record, "GHIDRA_DIRECT_BSR_TARGET"),
                           .direct_jsr_target = has_flag(record, "GHIDRA_DIRECT_JSR_TARGET"),
                           .known_static_target = record.known_direct_call_target,
                           .vector_target = has_flag(record, "GHIDRA_VECTOR_TARGET"),
                           .direct_caller_count = caller_count(record),
                           .unresolved_indirect_flow = record.indirect_flow,
                           .existing_beta_support = record.beta_match_kind != "unknown",
                           .existing_dynamic_support = record.dynamic_observed};
        std::uint32_t decode_end = record.entry + kFallbackBudget;
        bool valid_range = true;
        if (record.ghidra_range_start || record.ghidra_range_end) {
            valid_range = record.ghidra_range_start && record.ghidra_range_end &&
                          *record.ghidra_range_start == record.entry && *record.ghidra_range_end > record.entry;
            if (valid_range) decode_end = *record.ghidra_range_end + kProbeExtra;
        }
        for (const auto& atlas_entry : atlas.entries) {
            if (is_data(atlas_entry) && (atlas_entry.start == record.entry ||
                (record.ghidra_range_start && record.ghidra_range_end && overlaps(*record.ghidra_range_start,
                *record.ghidra_range_end, atlas_entry)))) item.known_data_overlap = true;
            if (atlas_entry.type == AtlasEntryType::function || atlas_entry.type == AtlasEntryType::bounded_code) {
                if (atlas_entry.start != record.entry && record.ghidra_range_start && record.ghidra_range_end &&
                    atlas_entry.boundary_confidence == AtlasConfidence::confirmed &&
                    overlaps(*record.ghidra_range_start, *record.ghidra_range_end, atlas_entry)) item.confirmed_code_overlap = true;
            }
        }
        for (const auto& other : candidate_map.candidates) {
            if (other.entry == record.entry || !record.ghidra_range_start || !record.ghidra_range_end ||
                !other.ghidra_range_start || !other.ghidra_range_end || other.entry == record.entry) continue;
            if (*record.ghidra_range_start < *other.ghidra_range_end && *other.ghidra_range_start < *record.ghidra_range_end)
                item.other_ghidra_overlap = true;
        }
        const bool known_data_entry = std::any_of(atlas.entries.begin(), atlas.entries.end(), [&](const auto& entry) {
            return is_data(entry) && entry.start == record.entry;
        });
        if (!known_data_entry && record.entry < rom.size() && (record.entry & 1U) == 0U && valid_range) {
            decode_end = std::min<std::uint32_t>(decode_end, static_cast<std::uint32_t>(rom.size()));
            if (decode_end > record.entry + 1U) {
                const auto slice = decode_m68k_slice(rom, {.entry = record.entry,
                    .byte_budget = decode_end - record.entry, .instruction_budget = 4096U});
                item.decode_ok = !slice.instructions.empty() && slice.instructions.front().address == record.entry &&
                                 slice.instructions.front().supported;
                item.first_instruction_supported = item.decode_ok;
                item.reachable_instruction_count = slice.instructions.size();
                item.reachable_block_count = slice.basic_blocks.size();
                item.unsupported_opcode_count = slice.unsupported_instruction_addresses.size();
                item.unsupported_addressing_count = 0;
                for (const auto& instruction : slice.instructions)
                    item.unsupported_addressing_count += instruction.unsupported_addressing.size();
                item.unresolved_indirect_flow = !slice.unresolved_control_flow.empty();
                std::set<std::uint32_t> decoded_addresses;
                for (const auto& instruction : slice.instructions) decoded_addresses.insert(instruction.address);
                for (const auto& instruction : slice.instructions) {
                    item.reaches_rts = item.reaches_rts || instruction.mnemonic == "rts";
                    item.reaches_rte = item.reaches_rte || instruction.mnemonic == "rte";
                    const auto next = instruction.address + static_cast<std::uint32_t>(instruction.bytes.size());
                    const bool terminal_branch = instruction.flow == FlowKind::direct_jump ||
                        (instruction.flow == FlowKind::direct_branch && !decoded_addresses.contains(next));
                    item.ends_known_direct_transfer = item.ends_known_direct_transfer ||
                        (terminal_branch && known_transfer(instruction, entries));
                }
                item.boundary_status = boundary_status(record, slice, valid_range);
            }
        } else {
            item.decode_conflict = !valid_range || (!known_data_entry &&
                (record.entry >= rom.size() || (record.entry & 1U) != 0U));
            item.boundary_status = record.ghidra_range_start ? MassBoundaryStatus::conflict : MassBoundaryStatus::unknown;
        }
        item.structural_classification = classify(item, record);
        collect_reasons(item, record);
        report.candidates.push_back(std::move(item));
    }
    std::sort(report.candidates.begin(), report.candidates.end(), [](const auto& left, const auto& right) {
        return left.entry < right.entry;
    });
    for (const auto entry : kConfirmed) {
        const auto found = std::find_if(report.candidates.begin(), report.candidates.end(),
            [=](const auto& item) { return item.entry == entry; });
        if (found == report.candidates.end()) report.control_set.push_back({entry, false, false, true, StructuralClassification::unknown});
        else {
            const bool pass = found->decode_ok && !found->unsupported_opcode_count &&
                !found->unsupported_addressing_count && !found->unresolved_indirect_flow &&
                !found->known_data_overlap && found->boundary_status != MassBoundaryStatus::conflict &&
                found->boundary_status != MassBoundaryStatus::longer;
            report.control_set.push_back({entry, true, pass, !pass, found->structural_classification});
        }
    }
    aggregate(report);
    return report;
}

std::string mass_boundary_name(MassBoundaryStatus status) {
    switch (status) {
    case MassBoundaryStatus::agrees: return "BOUNDARY_AGREES";
    case MassBoundaryStatus::shorter: return "BOUNDARY_SHORTER";
    case MassBoundaryStatus::longer: return "BOUNDARY_LONGER";
    case MassBoundaryStatus::unknown: return "BOUNDARY_UNKNOWN";
    case MassBoundaryStatus::conflict: return "BOUNDARY_CONFLICT";
    }
    return "BOUNDARY_UNKNOWN";
}

std::string structural_classification_name(StructuralClassification value) {
    switch (value) {
    case StructuralClassification::strong_static: return "STRONG_STATIC";
    case StructuralClassification::moderate_static: return "MODERATE_STATIC";
    case StructuralClassification::weak_static: return "WEAK_STATIC";
    case StructuralClassification::indirect_flow: return "INDIRECT_FLOW";
    case StructuralClassification::unsupported: return "UNSUPPORTED";
    case StructuralClassification::boundary_conflict: return "BOUNDARY_CONFLICT";
    case StructuralClassification::data_conflict: return "DATA_CONFLICT";
    case StructuralClassification::decode_failure: return "DECODE_FAILURE";
    case StructuralClassification::unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // namespace oasis::tools
