#include "tools/re_cfg_audit.hpp"

#include "tools/re_program.hpp"
#include "tools/re_resolution.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

constexpr std::uint32_t kTarget = 0x60004U;

struct DecodedContext {
    const DecodedSlice* slice{};
    std::map<std::uint32_t, std::size_t> block_by_address;
    std::vector<bool> reachable;
};

const DecodedInstruction* instruction_at(const DecodedSlice& slice, std::uint32_t address) {
    const auto found = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                    [=](const auto& item) { return item.address == address; });
    return found == slice.instructions.end() ? nullptr : &*found;
}

bool has_block(const DecodedSlice& slice, std::uint32_t start) {
    return std::any_of(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                       [=](const auto& block) { return block.start == start; });
}

std::vector<std::uint32_t> successors(const DecodedSlice& slice, const BasicBlock& block) {
    std::vector<std::uint32_t> result;
    if (block.instruction_addresses.empty()) return result;
    const auto* last = instruction_at(slice, block.instruction_addresses.back());
    if (!last) return result;
    for (const auto& edge : slice.control_flow)
        if (edge.source == last->address && has_block(slice, edge.target)) result.push_back(edge.target);
    const bool has_fallthrough = last->flow == FlowKind::direct_call ||
        (last->flow == FlowKind::direct_branch && last->mnemonic != "bra") ||
        last->flow == FlowKind::none;
    if (has_fallthrough && has_block(slice, block.end)) result.push_back(block.end);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

DecodedContext make_context(const DecodedSlice& slice) {
    DecodedContext context{.slice = &slice, .block_by_address = {}, .reachable = std::vector<bool>(slice.basic_blocks.size(), false)};
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i)
        for (const auto address : slice.basic_blocks[i].instruction_addresses) context.block_by_address[address] = i;
    const auto entry = context.block_by_address.find(slice.entry);
    if (entry == context.block_by_address.end()) return context;
    std::queue<std::size_t> pending;
    context.reachable[entry->second] = true;
    pending.push(entry->second);
    while (!pending.empty()) {
        const auto index = pending.front();
        pending.pop();
        for (const auto target : successors(slice, slice.basic_blocks[index])) {
            const auto found = std::find_if(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                                            [=](const auto& block) { return block.start == target; });
            if (found == slice.basic_blocks.end()) continue;
            const auto target_index = static_cast<std::size_t>(std::distance(slice.basic_blocks.begin(), found));
            if (!context.reachable[target_index]) {
                context.reachable[target_index] = true;
                pending.push(target_index);
            }
        }
    }
    return context;
}

std::vector<FunctionTarget> atlas_targets(const AtlasReport& atlas) {
    std::vector<FunctionTarget> result;
    for (const auto& entry : atlas.entries) {
        if (entry.type != AtlasEntryType::function && entry.type != AtlasEntryType::bounded_code) continue;
        if (entry.end) result.push_back({entry.start, 0, entry.end});
        else if (entry.bounded_evidence_end)
            result.push_back({entry.start, *entry.bounded_evidence_end - entry.start, std::nullopt});
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.entry < right.entry; });
    return result;
}

struct RawEdge {
    std::uint32_t source{};
    std::uint32_t target{};
    std::uint32_t source_function{};
    std::string kind;
};

bool same_edge(const RawEdge& left, const RawEdge& right) {
    return std::tie(left.source, left.target, left.source_function, left.kind) ==
        std::tie(right.source, right.target, right.source_function, right.kind);
}

std::vector<RawEdge> known_edges(const AtlasReport& atlas, const MultiSliceReport& program) {
    std::vector<RawEdge> result;
    for (const auto& function : program.functions) {
        for (const auto& edge : function.slice.control_flow)
            result.push_back({edge.source, edge.target, function.entry, edge.kind == FlowKind::direct_call ? "direct_call" : "direct_branch"});
    }
    for (const auto& call : program.direct_call_sites)
        result.push_back({call.instruction_address, call.target, call.caller_entry, "direct_call"});
    for (const auto& entry : atlas.entries)
        for (const auto address : entry.direct_rom_refs)
            result.push_back({entry.start, address, entry.start, "absolute_rom_ref"});
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return std::tie(left.target, left.source, left.source_function, left.kind) <
            std::tie(right.target, right.source, right.source_function, right.kind);
    });
    result.erase(std::unique(result.begin(), result.end(), same_edge), result.end());
    return result;
}

bool all_padding(const std::vector<std::uint8_t>& bytes) {
    if (bytes.empty()) return false;
    return std::all_of(bytes.begin(), bytes.end(), [](const auto byte) { return byte == 0x00U || byte == 0xFFU; });
}

const char* confidence_for(CfgAuditClassification classification) {
    if (classification == CfgAuditClassification::secondary_entry_candidate) return "LIKELY";
    if (classification == CfgAuditClassification::unknown) return "UNKNOWN";
    return "HYPOTHESIS";
}

CfgAuditClassification classify(const AuditRecord& record) {
    if (record.byte_end > 0x61204U || record.bytes.size() < 2U) return CfgAuditClassification::boundary_window_tail;
    if (record.embedded_data_pattern) return CfgAuditClassification::embedded_data_candidate;
    if (!record.decoder_supported) return CfgAuditClassification::decoder_artifact_candidate;
    if (!record.incoming_edges.empty() && std::any_of(record.incoming_edges.begin(), record.incoming_edges.end(),
                                                       [](const auto& edge) { return edge.source_function != kTarget; }))
        return CfgAuditClassification::secondary_entry_candidate;
    if (record.fallthrough_possible || !record.outgoing_targets.empty())
        return CfgAuditClassification::unreachable_code_candidate;
    return CfgAuditClassification::unknown;
}

std::string class_name(CfgAuditClassification classification) {
    switch (classification) {
    case CfgAuditClassification::unreachable_code_candidate: return "unreachable_code_candidate";
    case CfgAuditClassification::embedded_data_candidate: return "embedded_data_candidate";
    case CfgAuditClassification::secondary_entry_candidate: return "secondary_entry_candidate";
    case CfgAuditClassification::decoder_artifact_candidate: return "decoder_artifact_candidate";
    case CfgAuditClassification::boundary_window_tail: return "boundary_window_tail";
    case CfgAuditClassification::unknown: return "unknown";
    }
    return "unknown";
}

void sort_record_edges(AuditRecord& record) {
    std::sort(record.incoming_edges.begin(), record.incoming_edges.end(), [](const auto& left, const auto& right) {
        return std::tie(left.target, left.source, left.source_function, left.kind) <
            std::tie(right.target, right.source, right.source_function, right.kind);
    });
    record.incoming_edges.erase(std::unique(record.incoming_edges.begin(), record.incoming_edges.end(),
                                            [](const auto& left, const auto& right) {
        return left.source == right.source && left.target == right.target &&
            left.source_function == right.source_function && left.kind == right.kind;
    }), record.incoming_edges.end());
    std::sort(record.outgoing_targets.begin(), record.outgoing_targets.end());
    record.outgoing_targets.erase(std::unique(record.outgoing_targets.begin(), record.outgoing_targets.end()), record.outgoing_targets.end());
}

void add_classification(CfgAuditReport& report, const AuditRecord& record) {
    const auto key = class_name(record.classification);
    const auto found = std::find_if(report.classification_counts.begin(), report.classification_counts.end(),
                                    [&](const auto& item) { return item.key == key; });
    if (found == report.classification_counts.end()) report.classification_counts.push_back({key, 1, record.byte_end - record.instruction_address});
    else {
        ++found->records;
        found->bytes += record.byte_end - record.instruction_address;
    }
}

void add_factor(std::vector<CfgAuditFactorCount>& factors, const std::string& key) {
    const auto found = std::find_if(factors.begin(), factors.end(), [&](const auto& item) { return item.key == key; });
    if (found == factors.end()) factors.push_back({key, 1}); else ++found->records;
}

void build_islands(CfgAuditReport& report) {
    std::size_t island_index = 0;
    for (std::size_t i = 0; i < report.records.size();) {
        AuditIsland island;
        island.id = "island_" + std::to_string(island_index++);
        island.start = report.records[i].instruction_address;
        island.end = report.records[i].byte_end;
        while (i < report.records.size() &&
               (island.record_addresses.empty() || report.records[i].instruction_address == island.end)) {
            const auto& record = report.records[i++];
            island.end = record.byte_end;
            island.record_addresses.push_back(record.instruction_address);
            island.instruction_count = island.record_addresses.size();
            island.incoming_edges.insert(island.incoming_edges.end(), record.incoming_edges.begin(), record.incoming_edges.end());
            island.outgoing_targets.insert(island.outgoing_targets.end(), record.outgoing_targets.begin(), record.outgoing_targets.end());
        }
        island.byte_count = island.end - island.start;
        std::sort(island.incoming_edges.begin(), island.incoming_edges.end(), [](const auto& left, const auto& right) {
            return std::tie(left.target, left.source, left.source_function, left.kind) <
                std::tie(right.target, right.source, right.source_function, right.kind);
        });
        island.incoming_edges.erase(std::unique(island.incoming_edges.begin(), island.incoming_edges.end(),
                                                [](const auto& left, const auto& right) {
            return left.source == right.source && left.target == right.target && left.source_function == right.source_function && left.kind == right.kind;
        }), island.incoming_edges.end());
        std::sort(island.outgoing_targets.begin(), island.outgoing_targets.end());
        island.outgoing_targets.erase(std::unique(island.outgoing_targets.begin(), island.outgoing_targets.end()), island.outgoing_targets.end());
        const auto first_class = report.records[std::find_if(report.records.begin(), report.records.end(),
            [&](const auto& record) { return record.instruction_address == island.record_addresses.front(); }) - report.records.begin()].classification;
        island.classification = first_class;
        for (const auto address : island.record_addresses) {
            const auto found = std::find_if(report.records.begin(), report.records.end(),
                                            [=](const auto& record) { return record.instruction_address == address; });
            if (found != report.records.end() && found->classification != first_class) island.classification = CfgAuditClassification::unknown;
        }
        island.confidence = confidence_for(island.classification);
        if (!island.outgoing_targets.empty()) island.terminating_instruction = island.record_addresses.back();
        report.islands.push_back(std::move(island));
    }
}

} // namespace

CfgAuditReport audit_bounded_unreachable_cfg(const AtlasReport& atlas, std::span<const std::uint8_t> retail_rom) {
    const auto target_entry = std::find_if(atlas.entries.begin(), atlas.entries.end(),
                                            [](const auto& entry) { return entry.start == kTarget; });
    if (target_entry == atlas.entries.end()) throw std::invalid_argument("Atlas does not contain 0x60004");
    const auto slice = decode_m68k_slice(retail_rom, {.entry = kTarget, .byte_budget = 0x1200U});
    const auto context = make_context(slice);
    const auto program = analyze_m68k_functions(retail_rom, atlas_targets(atlas));
    const auto edges = known_edges(atlas, program);
    const auto resolution = resolve_bounded_displacements(atlas, retail_rom);
    CfgAuditReport report{.target_entry = kTarget, .window_start = kTarget, .window_end = 0x61204U,
                          .raw_static_evidence_records = 0, .outside_reachable_records = 0,
                          .reachable_unresolved_after_resolution = resolution.still_unresolved,
                          .nonreachable_unresolved = 0, .raw_unresolved_after_resolution = resolution.still_unresolved,
                          .records_with_known_incoming_edges = 0, .records_without_known_incoming_edges = 0,
                          .secondary_entry_candidates = 0, .suspected_data_or_artifact_records = 0,
                          .unknown_remainder = 0, .atlas_unresolved_before = resolution.atlas_unresolved_before,
                          .atlas_unresolved_after_audit = resolution.atlas_unresolved_before,
                          .ranking_displacement_before = resolution.ranking_delta.empty() ? 0 : resolution.ranking_delta.front().before,
                          .ranking_displacement_after_audit = resolution.ranking_delta.empty() ? 0 : resolution.ranking_delta.front().before,
                          .beta_evidence = "no new beta scan; existing 0x60004 correspondence is exact"};
    for (const auto& reference : target_entry->unresolved_references) {
        if (reference.addressing_mode != "address_displacement") continue;
        const auto block = context.block_by_address.find(reference.instruction_address);
        if (block == context.block_by_address.end() || context.reachable[block->second]) continue;
        const auto* instruction = instruction_at(slice, reference.instruction_address);
        if (!instruction) continue;
        AuditRecord record{.instruction_address = reference.instruction_address,
                           .byte_end = reference.instruction_address + static_cast<std::uint32_t>(instruction->bytes.size()),
                           .block_start = reference.block_start, .opcode = instruction->opcode, .bytes = instruction->bytes,
                           .direct_memory_references = instruction->memory_references,
                           .unresolved_memory_references = instruction->unresolved_memory_references};
        record.decoder_supported = instruction->supported;
        record.mnemonic = instruction->mnemonic;
        record.decoder_status = instruction->supported ? "supported_bounded_decode" : "unsupported_opcode";
        record.alignment_padding_pattern = all_padding(record.bytes);
        record.embedded_data_pattern = record.alignment_padding_pattern;
        const auto& block_data = slice.basic_blocks[block->second];
        const auto position = std::find(block_data.instruction_addresses.begin(), block_data.instruction_addresses.end(), record.instruction_address);
        if (position != block_data.instruction_addresses.begin() && position != block_data.instruction_addresses.end()) {
            const auto* previous = instruction_at(slice, *(position - 1));
            record.fallthrough_possible = previous && previous->address + previous->bytes.size() == record.instruction_address;
        }
        for (const auto& edge : edges) {
            if (edge.target == record.instruction_address)
                record.incoming_edges.push_back({edge.source, edge.target, edge.source_function, edge.kind});
        }
        for (const auto& edge : slice.control_flow)
            if (edge.source == record.instruction_address) record.outgoing_targets.push_back(edge.target);
        sort_record_edges(record);
        std::vector<std::uint32_t> reachable_addresses;
        for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i)
            if (context.reachable[i]) reachable_addresses.insert(reachable_addresses.end(), slice.basic_blocks[i].instruction_addresses.begin(), slice.basic_blocks[i].instruction_addresses.end());
        std::sort(reachable_addresses.begin(), reachable_addresses.end());
        const auto following = std::lower_bound(reachable_addresses.begin(), reachable_addresses.end(), record.instruction_address);
        if (following != reachable_addresses.begin()) {
            record.nearest_preceding_reachable = *(following - 1);
            record.preceding_distance = record.instruction_address - *record.nearest_preceding_reachable;
            record.nearest_preceding_reachable_block = slice.basic_blocks[context.block_by_address.at(*record.nearest_preceding_reachable)].start;
        }
        if (following != reachable_addresses.end()) {
            record.nearest_following_reachable = *following;
            record.following_distance = *record.nearest_following_reachable - record.instruction_address;
            record.nearest_following_reachable_block = slice.basic_blocks[context.block_by_address.at(*record.nearest_following_reachable)].start;
        }
        if (record.incoming_edges.empty()) add_factor(report.reachability_factors, "no_incoming_evidence");
        else add_factor(report.reachability_factors, "known_direct_or_pointer_xref");
        if (record.fallthrough_possible) add_factor(report.reachability_factors, "lexical_fallthrough_possible");
        if (record.alignment_padding_pattern) add_factor(report.reachability_factors, "alignment_or_padding_pattern");
        if (!record.decoder_supported) add_factor(report.reachability_factors, "decoder_unsupported");
        if (record.nearest_preceding_reachable) {
            const auto* preceding = instruction_at(slice, *record.nearest_preceding_reachable);
            if (preceding && preceding->flow == FlowKind::direct_branch && preceding->mnemonic == "bra")
                add_factor(report.reachability_factors, "unconditional_branch_skip");
            if (preceding && preceding->flow == FlowKind::return_instruction)
                add_factor(report.reachability_factors, "return_before_region");
        }
        if (!slice.unresolved_control_flow.empty()) add_factor(report.reachability_factors, "indirect_entry_possible_unresolved");
        record.classification = classify(record);
        record.confidence = confidence_for(record.classification);
        record.reason = record.classification == CfgAuditClassification::unknown ? "no bounded incoming or structural evidence" : class_name(record.classification);
        report.records.push_back(std::move(record));
    }
    std::sort(report.records.begin(), report.records.end(), [](const auto& left, const auto& right) { return left.instruction_address < right.instruction_address; });
    report.raw_static_evidence_records = report.records.size();
    report.outside_reachable_records = report.records.size();
    report.nonreachable_unresolved = report.records.size();
    report.reachable_unresolved_after_resolution = resolution.still_unresolved - report.nonreachable_unresolved;
    for (const auto& record : report.records) {
        add_classification(report, record);
        if (record.incoming_edges.empty()) ++report.records_without_known_incoming_edges;
        else ++report.records_with_known_incoming_edges;
        if (record.classification == CfgAuditClassification::secondary_entry_candidate) ++report.secondary_entry_candidates;
        if (record.classification == CfgAuditClassification::embedded_data_candidate ||
            record.classification == CfgAuditClassification::decoder_artifact_candidate) ++report.suspected_data_or_artifact_records;
        if (record.classification == CfgAuditClassification::unknown) ++report.unknown_remainder;
    }
    std::sort(report.reachability_factors.begin(), report.reachability_factors.end(), [](const auto& left, const auto& right) {
        return left.key < right.key;
    });
    build_islands(report);
    return report;
}

std::string cfg_audit_classification_name(CfgAuditClassification classification) { return class_name(classification); }

CfgAuditClassification classify_cfg_audit_record(const AuditRecord& record) { return classify(record); }

std::vector<AuditIsland> group_cfg_audit_islands(std::span<const AuditRecord> records) {
    CfgAuditReport report;
    report.records.assign(records.begin(), records.end());
    std::sort(report.records.begin(), report.records.end(), [](const auto& left, const auto& right) {
        return left.instruction_address < right.instruction_address;
    });
    build_islands(report);
    return report.islands;
}

} // namespace oasis::tools
