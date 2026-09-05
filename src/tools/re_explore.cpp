#include "tools/re_explore.hpp"

#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <tuple>

namespace oasis::tools {
namespace {

constexpr std::uint32_t kDefaultControls[] = {
    0x3820U, 0x7A28U, 0x82AEU, 0x8E90U, 0x938EU, 0x9BF2U,
    0xA6A4U, 0xD3B2U, 0x60004U, 0x604BCU, 0x6121AU,
    0x60B8CU, 0x60D4AU, 0x611EEU, 0x60BCCU};
constexpr ControlExpectation kDefaultEdges[] = {
    {0x60004U, 0x6042AU}, {0x60B8CU, 0x6121AU}, {0x60D4AU, 0x6121AU},
    {0x611EEU, 0x6121AU}, {0x60BCCU, 0x604BCU}, {0xD3B2U, 0x3820U}};

struct Interval { std::uint32_t start{}; std::uint32_t end{}; RegionType type{}; std::string evidence; };
struct RawSpan { std::uint32_t start{}; std::uint32_t end{}; std::uint32_t owner{}; };
struct WorkItem { std::uint32_t priority{}; std::uint32_t address{}; };
struct WorkLess { bool operator()(const WorkItem& left, const WorkItem& right) const {
    return std::tie(left.priority, left.address) > std::tie(right.priority, right.address);
} };

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

std::string rom_identity(std::span<const std::uint8_t> rom) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto byte : rom) { hash ^= byte; hash *= 1099511628211ULL; }
    std::ostringstream out;
    out << "size=" << rom.size() << ";fnv1a64=" << std::uppercase << std::hex << hash;
    return out.str();
}

template <typename T> void unique_sorted(std::vector<T>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

bool in_rom(std::span<const std::uint8_t> rom, std::uint32_t address) {
    return address < rom.size() && (address & 1U) == 0U;
}

bool same_seed(const ExploreSeed& left, const ExploreSeed& right) {
    return std::tie(left.address, left.tier, left.priority, left.source, left.source_reference,
                     left.initial_confidence) ==
        std::tie(right.address, right.tier, right.priority, right.source, right.source_reference,
                 right.initial_confidence);
}

std::uint32_t seed_priority(SeedTier tier) { return static_cast<std::uint32_t>(tier) * 100U; }

void add_seed(std::map<std::uint32_t, std::vector<ExploreSeed>>& seeds, ExploreSeed seed) {
    seed.priority = seed_priority(seed.tier);
    auto& values = seeds[seed.address];
    if (std::find_if(values.begin(), values.end(), [&](const auto& item) { return same_seed(item, seed); }) == values.end())
        values.push_back(std::move(seed));
    std::sort(values.begin(), values.end(), [](const auto& left, const auto& right) {
        return std::tie(left.priority, left.address, left.source, left.source_reference) <
            std::tie(right.priority, right.address, right.source, right.source_reference);
    });
}

std::vector<Interval> data_intervals(const AtlasReport& atlas) {
    std::vector<Interval> result;
    for (const auto& entry : atlas.entries) {
        if (entry.type != AtlasEntryType::data && entry.type != AtlasEntryType::table) continue;
        const auto end = entry.end ? entry.end : entry.bounded_evidence_end;
        const auto type = entry.type == AtlasEntryType::table ? RegionType::pointer_data : RegionType::known_data;
        if (end && *end > entry.start) result.push_back({entry.start, *end, type, entry.id});
        else result.push_back({entry.start, entry.start + 2U, RegionType::probable_data, entry.id + ";start_only"});
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return std::tie(left.start, left.end, left.type, left.evidence) <
            std::tie(right.start, right.end, right.type, right.evidence);
    });
    return result;
}

const Interval* data_at(const std::vector<Interval>& intervals, std::uint32_t address) {
    for (const auto& item : intervals)
        if (address >= item.start && address < item.end) return &item;
    return nullptr;
}

ExploreEntry& entry_at(std::map<std::uint32_t, ExploreEntry>& entries, std::uint32_t address) {
    auto [found, inserted] = entries.emplace(address, ExploreEntry{.address = address});
    if (inserted) found->second.state = AnalysisState::discovered;
    return found->second;
}

void add_stop(ExploreReport& report, std::uint32_t entry, std::uint32_t pc, StopReason reason,
              std::string detail) {
    report.stops.push_back({entry, pc, reason, std::move(detail)});
}

std::string context_for(const DecodedInstruction& instruction) {
    std::string result;
    for (std::size_t i = 0; i < instruction.addressing_modes.size(); ++i) {
        if (i) result += ',';
        result += instruction.addressing_modes[i];
    }
    return result;
}

std::string frontier_id(const std::string& rom_id, std::uint32_t entry, std::uint32_t pc,
                        FrontierType type, const std::string& context) {
    return rom_id + ":" + hex32(entry) + ":" + hex32(pc) + ":" + frontier_type_name(type) + ":" + context;
}

void add_frontier(ExploreReport& report, const std::string& rom_id, FrontierRecord record) {
    record.id = frontier_id(rom_id, record.source_entry, record.source_pc, record.blocker_type,
                            record.known_context + ":" + record.reason);
    if (std::find_if(report.frontier.begin(), report.frontier.end(), [&](const auto& item) { return item.id == record.id; }) == report.frontier.end())
        report.frontier.push_back(std::move(record));
}

ExploreEdgeKind edge_kind(const DecodedInstruction& instruction) {
    if (instruction.flow == FlowKind::direct_call) return ExploreEdgeKind::direct_call;
    if (instruction.flow == FlowKind::direct_jump || instruction.mnemonic == "bra") return ExploreEdgeKind::direct_jump;
    if (instruction.mnemonic == "dbcc") return ExploreEdgeKind::dbcc;
    return ExploreEdgeKind::conditional_branch;
}

bool has_fallthrough(const DecodedInstruction& instruction) {
    return instruction.flow == FlowKind::direct_call || instruction.flow == FlowKind::none ||
        (instruction.flow == FlowKind::direct_branch && instruction.mnemonic != "bra");
}

void add_edge(ExploreReport& report, ExploreEdge edge) {
    report.edges.push_back(std::move(edge));
}

void sort_and_unique(ExploreReport& report) {
    std::sort(report.edges.begin(), report.edges.end(), [](const auto& left, const auto& right) {
        return std::tie(left.source_entry, left.source_pc, left.target, left.kind, left.evidence_class) <
            std::tie(right.source_entry, right.source_pc, right.target, right.kind, right.evidence_class);
    });
    report.edges.erase(std::unique(report.edges.begin(), report.edges.end(), [](const auto& left, const auto& right) {
        return std::tie(left.source_entry, left.source_pc, left.target, left.kind, left.evidence_class) ==
            std::tie(right.source_entry, right.source_pc, right.target, right.kind, right.evidence_class);
    }), report.edges.end());
    std::sort(report.stops.begin(), report.stops.end(), [](const auto& left, const auto& right) {
        return std::tie(left.source_entry, left.source_pc, left.reason, left.detail) <
            std::tie(right.source_entry, right.source_pc, right.reason, right.detail);
    });
    std::sort(report.frontier.begin(), report.frontier.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
}

void add_seed_sources(std::span<const std::uint8_t> rom, const CandidateMapReport& candidates,
                      const AtlasReport& atlas, const std::vector<std::uint32_t>& controls,
                      std::map<std::uint32_t, std::vector<ExploreSeed>>& seeds) {
    for (std::size_t index = 0; index < 16U && index * 4U + 3U < rom.size(); ++index) {
        const auto address = (static_cast<std::uint32_t>(rom[index * 4U]) << 24U) |
            (static_cast<std::uint32_t>(rom[index * 4U + 1U]) << 16U) |
            (static_cast<std::uint32_t>(rom[index * 4U + 2U]) << 8U) | rom[index * 4U + 3U];
        if (in_rom(rom, address)) add_seed(seeds, {address, SeedTier::tier0, 0, "vector", "vector[" + std::to_string(index) + "]", "CONFIRMED_VECTOR"});
    }
    for (const auto& entry : atlas.entries) {
        if (entry.type != AtlasEntryType::function && entry.type != AtlasEntryType::bounded_code) continue;
        const auto found = std::find_if(candidates.candidates.begin(), candidates.candidates.end(), [&](const auto& item) { return item.entry == entry.start; });
        if (found != candidates.candidates.end() && found->classification == CandidateClassification::confirmed)
            add_seed(seeds, {entry.start, SeedTier::tier0, 0, "confirmed_project_entry", entry.id, "CONFIRMED"});
    }
    for (const auto& edge : atlas.call_edges)
        if (in_rom(rom, edge.callee)) add_seed(seeds, {edge.callee, SeedTier::tier1, 0, "static_direct_call", hex32(edge.caller) + "->" + hex32(edge.callee), "STATIC_SUPPORTED"});
    for (const auto& candidate : candidates.candidates) {
        if (candidate.classification == CandidateClassification::conflict) continue;
        if (candidate.ghidra_function && (!candidate.ghidra_calls.empty() || !candidate.ghidra_called_by.empty()))
            add_seed(seeds, {candidate.entry, SeedTier::tier2, 0, "ghidra_function_with_xrefs", "candidate_map", "LIKELY"});
        else if (candidate.ghidra_function)
            add_seed(seeds, {candidate.entry, SeedTier::tier3, 0, "ghidra_function_candidate", "candidate_map", "LIKELY"});
    }
    for (const auto address : controls)
        add_seed(seeds, {address, SeedTier::tier0, 0, "control_corpus", "bounded_control_corpus", "PROJECT_ENTRY"});
}

void build_map(ExploreReport& report, const std::vector<RawSpan>& code, const std::vector<Interval>& data,
               std::map<std::uint32_t, ExploreEntry>& entries) {
    std::vector<std::uint32_t> points;
    for (const auto& span : code) { points.push_back(span.start); points.push_back(span.end); }
    for (const auto& span : data) { points.push_back(span.start); points.push_back(span.end); }
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    for (std::size_t index = 1; index < points.size(); ++index) {
        const auto start = points[index - 1U];
        const auto end = points[index];
        if (start >= end) continue;
        std::vector<std::uint32_t> owners;
        for (const auto& span : code) if (span.start < end && span.end > start) owners.push_back(span.owner);
        unique_sorted(owners);
        std::vector<std::string> sources;
        for (const auto& span : data) if (span.start < end && span.end > start) sources.push_back(span.evidence);
        const bool has_data = !sources.empty();
        const bool has_code = !owners.empty();
        if (!has_code && !has_data) continue;
        const auto data_span = std::find_if(data.begin(), data.end(), [&](const auto& item) { return item.start < end && item.end > start; });
        ExploreMapRange range{start, end, has_code && has_data ? RegionType::conflict : has_code ? RegionType::instruction : data_span->type,
                              owners, sources, has_code ? "STRUCTURAL" : "KNOWN", has_data ? sources.front() : "decoded_instruction", AnalysisState::analyzed};
        if (has_code) range.sources.push_back("decoded_instruction");
        for (const auto owner : owners) {
            const auto found = entries.find(owner);
            if (found != entries.end() && found->second.state == AnalysisState::conflict) range.status = AnalysisState::conflict;
        }
        if (!report.address_map.empty() && report.address_map.back().end == range.start &&
            report.address_map.back().type == range.type && report.address_map.back().owners == range.owners &&
            report.address_map.back().sources == range.sources && report.address_map.back().status == range.status)
            report.address_map.back().end = range.end;
        else report.address_map.push_back(std::move(range));
    }
    std::sort(report.address_map.begin(), report.address_map.end(), [](const auto& left, const auto& right) { return std::tie(left.start, left.end, left.type) < std::tie(right.start, right.end, right.type); });
}

void cluster_frontier(ExploreReport& report) {
    std::map<FrontierType, std::vector<std::uint32_t>> grouped;
    for (const auto& item : report.frontier) grouped[item.blocker_type].push_back(item.source_pc);
    for (auto& [type, addresses] : grouped) {
        const auto count = addresses.size();
        unique_sorted(addresses);
        if (addresses.size() > 5U) addresses.resize(5U);
        report.blocker_clusters.push_back({frontier_type_name(type), count, addresses});
    }
    std::sort(report.blocker_clusters.begin(), report.blocker_clusters.end(), [](const auto& left, const auto& right) {
        return std::tie(right.count, left.blocker_type) < std::tie(left.count, right.blocker_type);
    });
}

} // namespace

ExploreReport explore_m68k(std::span<const std::uint8_t> rom, const CandidateMapReport& candidates,
                           const AtlasReport& atlas, const ExploreOptions& requested) {
    ExploreOptions options = requested;
    if (options.control_entries.empty()) options.control_entries.assign(std::begin(kDefaultControls), std::end(kDefaultControls));
    if (options.control_edges.empty()) options.control_edges.assign(std::begin(kDefaultEdges), std::end(kDefaultEdges));
    ExploreReport report{.rom_identity = rom_identity(rom), .rom_wide_requested = options.rom_wide};
    const auto intervals = data_intervals(atlas);
    std::map<std::uint32_t, std::vector<ExploreSeed>> seed_map;
    add_seed_sources(rom, candidates, atlas, options.control_entries, seed_map);
    for (const auto& [address, seeds] : seed_map) report.seeds.insert(report.seeds.end(), seeds.begin(), seeds.end());
    std::map<std::uint32_t, ExploreEntry> entries;
    for (const auto& [address, seeds] : seed_map) entry_at(entries, address).seeds = seeds;
    std::vector<RawSpan> code_spans;
    std::priority_queue<WorkItem, std::vector<WorkItem>, WorkLess> queue;
    std::set<std::uint32_t> queued;
    auto queue_entry = [&](std::uint32_t address) {
        auto& item = entry_at(entries, address);
        if (item.state == AnalysisState::analyzed || item.state == AnalysisState::analyzing || queued.contains(address)) return;
        std::uint32_t priority = 400U;
        if (!item.seeds.empty()) priority = item.seeds.front().priority;
        item.state = AnalysisState::queued;
        queued.insert(address);
        queue.push({priority, address});
    };
    for (const auto address : options.control_entries) queue_entry(address);
    auto run_queue = [&]() {
        while (!queue.empty() && report.metrics.entries_processed < options.max_entries) {
            const auto work = queue.top(); queue.pop(); queued.erase(work.address);
            auto& result = entry_at(entries, work.address);
            if (result.state == AnalysisState::analyzed) continue;
            ++report.metrics.entries_processed;
            report.processing_order.push_back(work.address);
            result.state = AnalysisState::analyzing;
            const auto candidate = std::find_if(candidates.candidates.begin(), candidates.candidates.end(), [&](const auto& item) { return item.entry == work.address; });
            const auto* guard = data_at(intervals, work.address);
            if (!in_rom(rom, work.address)) {
                result.state = AnalysisState::conflict;
                add_stop(report, work.address, work.address, StopReason::out_of_rom, "entry_out_of_rom_or_odd");
                add_frontier(report, report.rom_identity, {"", work.address, work.address, FrontierType::out_of_rom, {}, 0, "", "", {work.address}, {}, StopReason::out_of_rom, "entry_out_of_rom"});
                continue;
            }
            if (guard) {
                const bool independent_code_evidence = candidate != candidates.candidates.end() && candidate->ghidra_range_end &&
                    *candidate->ghidra_range_end > work.address;
                result.state = independent_code_evidence ? AnalysisState::conflict : AnalysisState::blocked_data;
                if (independent_code_evidence) code_spans.push_back({work.address, static_cast<std::uint32_t>(std::min<std::size_t>(*candidate->ghidra_range_end, rom.size())), work.address});
                add_stop(report, work.address, work.address, independent_code_evidence ? StopReason::conflict : StopReason::known_data, guard->evidence);
                add_frontier(report, report.rom_identity, {"", work.address, work.address, FrontierType::code_data_conflict, {}, 0, "", "", {work.address}, {guard->evidence}, independent_code_evidence ? StopReason::conflict : StopReason::known_data, independent_code_evidence ? "independent code evidence overlaps Atlas data" : "Atlas data guard"});
                continue;
            }
            std::size_t budget = options.entry_byte_budget;
            bool explicit_atlas_budget = false;
            const auto atlas_entry = std::find_if(atlas.entries.begin(), atlas.entries.end(), [&](const auto& item) { return item.start == work.address; });
            if (atlas_entry != atlas.entries.end()) {
                const auto end = atlas_entry->end ? atlas_entry->end : atlas_entry->bounded_evidence_end;
                if (end && *end > work.address) { budget = *end - work.address; explicit_atlas_budget = true; }
            }
            if (!explicit_atlas_budget && candidate != candidates.candidates.end() && candidate->ghidra_range_end && *candidate->ghidra_range_end > work.address)
                budget = std::min(options.entry_byte_budget, static_cast<std::size_t>(*candidate->ghidra_range_end - work.address + 0x100U));
            budget = std::max<std::size_t>(budget, 2U);
            DecodedSlice slice;
            try { slice = decode_m68k_slice(rom, {.entry = work.address, .byte_budget = budget, .instruction_budget = options.instruction_budget}); }
            catch (...) {
                result.state = AnalysisState::conflict;
                add_stop(report, work.address, work.address, StopReason::decode_failure, "decoder_rejected_entry");
                add_frontier(report, report.rom_identity, {"", work.address, work.address, FrontierType::decode_failure, {}, 0, "", "", {work.address}, {}, StopReason::decode_failure, "decoder_rejected_entry"});
                continue;
            }
            std::map<std::uint32_t, const DecodedInstruction*> decoded;
            for (const auto& instruction : slice.instructions) decoded[instruction.address] = &instruction;
            std::set<std::uint32_t> pending{work.address};
            std::set<std::uint32_t> visited;
            bool saw_indirect = false, saw_unsupported = false, saw_data = false, saw_conflict = false;
            while (!pending.empty()) {
                const auto pc = *pending.begin(); pending.erase(pending.begin());
                if (!visited.insert(pc).second) { add_stop(report, work.address, pc, StopReason::already_analyzed, "same_entry_path_merge"); continue; }
                const auto data_guard = data_at(intervals, pc);
                if (data_guard) {
                    saw_data = true;
                    add_stop(report, work.address, pc, StopReason::known_data, data_guard->evidence);
                    add_frontier(report, report.rom_identity, {"", work.address, pc, FrontierType::code_data_conflict, {}, 0, "", "", {work.address}, {data_guard->evidence}, StopReason::known_data, "Atlas data guard"});
                    continue;
                }
                const auto found = decoded.find(pc);
                if (found == decoded.end()) {
                    const auto reason = pc >= rom.size() ? StopReason::out_of_rom : StopReason::decode_failure;
                    add_stop(report, work.address, pc, reason, "successor_not_in_bounded_decode");
                    add_frontier(report, report.rom_identity, {"", work.address, pc, reason == StopReason::out_of_rom ? FrontierType::out_of_rom : FrontierType::decode_failure, {}, 0, "", "", {work.address}, {}, reason, "successor_not_in_bounded_decode"});
                    continue;
                }
                const auto& instruction = *found->second;
                ++result.decoded_instruction_count;
                if (instruction.supported) result.supported_instruction_bytes += instruction.bytes.size();
                code_spans.push_back({instruction.address, instruction.address + static_cast<std::uint32_t>(instruction.bytes.size()), work.address});
                if (!instruction.supported) {
                    saw_unsupported = true;
                    add_stop(report, work.address, pc, StopReason::unsupported_instruction, instruction.mnemonic);
                    add_frontier(report, report.rom_identity, {"", work.address, pc, FrontierType::unsupported, instruction.bytes, instruction.opcode, instruction.mnemonic, context_for(instruction), {work.address}, {}, StopReason::unsupported_instruction, "decoder_unsupported"});
                    continue;
                }
                const auto next = instruction.address + static_cast<std::uint32_t>(instruction.bytes.size());
                if (instruction.flow == FlowKind::return_instruction) {
                    add_stop(report, work.address, pc, StopReason::return_instruction, instruction.mnemonic);
                    continue;
                }
                if (instruction.flow == FlowKind::indirect_call || instruction.flow == FlowKind::indirect_jump) {
                    const auto* dynamic = find_dynamic_edge(options.dynamic_edges, work.address, pc);
                    const bool dynamic_valid = dynamic && in_rom(rom, dynamic->target) &&
                        !data_at(intervals, dynamic->target);
                    if (dynamic_valid) {
                        const bool is_call = instruction.flow == FlowKind::indirect_call;
                        add_edge(report, {work.address, pc, dynamic->target, ExploreEdgeKind::dynamic_indirect,
                                          is_call, dynamic->evidence_class, dynamic->frontier_id,
                                          dynamic->job_id, dynamic->result_hash, dynamic->backend,
                                          dynamic->scenario});
                        if (is_call) queue_entry(dynamic->target);
                        else pending.insert(dynamic->target);
                    } else {
                        saw_indirect = true;
                        add_edge(report, {work.address, pc, 0U, ExploreEdgeKind::unresolved_indirect, false});
                        add_stop(report, work.address, pc, StopReason::indirect_transfer, instruction.mnemonic);
                        add_frontier(report, report.rom_identity, {"", work.address, pc, FrontierType::indirect_flow, instruction.bytes, instruction.opcode, instruction.mnemonic, context_for(instruction), {work.address}, {}, StopReason::indirect_transfer, "computed target is unresolved"});
                    }
                    if (instruction.flow == FlowKind::indirect_jump && !dynamic_valid) continue;
                }
                if (instruction.direct_target) {
                    const auto target = *instruction.direct_target;
                    const auto kind = edge_kind(instruction);
                    add_edge(report, {work.address, pc, target, kind, in_rom(rom, target) && !data_at(intervals, target)});
                    if (!in_rom(rom, target)) {
                        add_stop(report, work.address, pc, StopReason::out_of_rom, "direct_target=" + hex32(target));
                        add_frontier(report, report.rom_identity, {"", work.address, pc, FrontierType::out_of_rom, instruction.bytes, instruction.opcode, instruction.mnemonic, "target=" + hex32(target), {work.address}, {}, StopReason::out_of_rom, "direct_target_out_of_rom"});
                    } else if (data_at(intervals, target)) {
                        saw_data = true;
                        add_stop(report, work.address, pc, StopReason::known_data, "direct_target=" + hex32(target));
                        add_frontier(report, report.rom_identity, {"", work.address, pc, FrontierType::code_data_conflict, instruction.bytes, instruction.opcode, instruction.mnemonic, "target=" + hex32(target), {work.address}, {}, StopReason::known_data, "direct_target_hits_Atlas_data"});
                    } else {
                        if (instruction.flow == FlowKind::direct_call) queue_entry(target);
                        if (instruction.flow == FlowKind::direct_jump || instruction.mnemonic == "bra") add_stop(report, work.address, pc, StopReason::direct_terminal_transfer, instruction.mnemonic);
                        if (instruction.flow != FlowKind::direct_call) pending.insert(target);
                    }
                }
                if (has_fallthrough(instruction)) {
                    add_edge(report, {work.address, pc, next, ExploreEdgeKind::fallthrough, decoded.contains(next)});
                    if (decoded.contains(next)) pending.insert(next);
                    else if (in_rom(rom, next)) { add_stop(report, work.address, pc, StopReason::decode_failure, "fallthrough_not_in_bounded_decode"); }
                    else add_stop(report, work.address, pc, StopReason::out_of_rom, "fallthrough_out_of_rom");
                }
            }
            result.frontier_count = static_cast<std::size_t>(std::count_if(report.frontier.begin(), report.frontier.end(), [&](const auto& item) { return item.source_entry == work.address; }));
            result.stop_count = static_cast<std::size_t>(std::count_if(report.stops.begin(), report.stops.end(), [&](const auto& item) { return item.source_entry == work.address; }));
            result.block_count = slice.basic_blocks.size();
            result.state = saw_conflict ? AnalysisState::conflict : saw_data ? AnalysisState::blocked_data : saw_unsupported ? AnalysisState::blocked_unsupported : saw_indirect ? AnalysisState::blocked_indirect : AnalysisState::analyzed;
        }
    };
    run_queue();
    for (const auto& expectation : options.control_edges) {
        const auto found = std::find_if(report.edges.begin(), report.edges.end(), [&](const auto& edge) { return edge.source_entry == expectation.source && edge.target == expectation.target && (edge.kind == ExploreEdgeKind::direct_call || edge.kind == ExploreEdgeKind::direct_jump); });
        if (found != report.edges.end()) {
            auto control = std::find_if(report.control_set.begin(), report.control_set.end(), [&](const auto& item) { return item.entry == expectation.source; });
            if (control == report.control_set.end()) report.control_set.push_back({expectation.source, true, true, true, false, {expectation}});
            else control->recovered_edges.push_back(expectation);
        }
    }
    for (const auto address : options.control_entries) {
        const auto found = entries.find(address);
        const bool present = found != entries.end();
        const bool analyzed = present && found->second.state != AnalysisState::discovered && found->second.state != AnalysisState::queued;
        const bool pass = present && analyzed && found->second.state != AnalysisState::blocked_data && found->second.state != AnalysisState::conflict;
        auto control = std::find_if(report.control_set.begin(), report.control_set.end(), [&](const auto& item) { return item.entry == address; });
        if (control == report.control_set.end()) report.control_set.push_back({address, present, analyzed, pass, !pass, {}});
        else { control->present = present; control->analyzed = analyzed; control->heuristic_pass = pass; control->heuristic_miss = !pass; }
    }
    std::sort(report.control_set.begin(), report.control_set.end(), [](const auto& left, const auto& right) { return left.entry < right.entry; });
    report.bounded_control_pass = std::all_of(report.control_set.begin(), report.control_set.end(), [](const auto& item) { return item.heuristic_pass; });
    if (options.rom_wide && report.bounded_control_pass) {
        report.rom_wide_performed = true;
        for (const auto& [address, ignored] : seed_map) queue_entry(address);
        run_queue();
    } else if (options.rom_wide) report.rom_wide_skip_reason = "bounded_control_corpus_failed";
    sort_and_unique(report);
    report.entries.reserve(entries.size());
    for (auto& [address, item] : entries) report.entries.push_back(std::move(item));
    std::sort(report.entries.begin(), report.entries.end(), [](const auto& left, const auto& right) { return left.address < right.address; });
    build_map(report, code_spans, intervals, entries);
    for (const auto& range : report.address_map) {
        const auto bytes = static_cast<std::size_t>(range.end - range.start);
        if (range.type == RegionType::instruction) report.metrics.instruction_bytes += bytes;
        if (range.type == RegionType::known_data) report.metrics.known_data_bytes += bytes;
        if (range.type == RegionType::probable_data) report.metrics.probable_data_bytes += bytes;
        if (range.type == RegionType::pointer_data) report.metrics.pointer_data_bytes += bytes;
        if (range.type == RegionType::conflict) report.metrics.conflict_bytes += bytes;
    }
    report.metrics.total_rom_bytes = rom.size();
    const auto classified = report.metrics.instruction_bytes + report.metrics.known_data_bytes + report.metrics.probable_data_bytes + report.metrics.pointer_data_bytes + report.metrics.conflict_bytes;
    report.metrics.unclassified_bytes = classified < rom.size() ? rom.size() - classified : 0;
    report.metrics.seeds = report.seeds.size();
    report.metrics.discovered_entries = report.entries.size();
    for (const auto& item : report.entries) {
        if (item.state != AnalysisState::discovered) ++report.metrics.queued_entries;
        if (item.state == AnalysisState::analyzed) ++report.metrics.analyzed_entries;
        if (item.state == AnalysisState::blocked_indirect) ++report.metrics.blocked_indirect_entries;
        if (item.state == AnalysisState::blocked_unsupported) ++report.metrics.blocked_unsupported_entries;
        if (item.state == AnalysisState::blocked_data) ++report.metrics.blocked_data_entries;
        if (item.state == AnalysisState::conflict) ++report.metrics.conflict_entries;
        report.metrics.decoded_instructions += item.decoded_instruction_count;
    }
    report.metrics.direct_calls = std::count_if(report.edges.begin(), report.edges.end(), [](const auto& item) { return item.kind == ExploreEdgeKind::direct_call; });
    report.metrics.direct_jumps = std::count_if(report.edges.begin(), report.edges.end(), [](const auto& item) { return item.kind == ExploreEdgeKind::direct_jump; });
    report.metrics.conditional_branches = std::count_if(report.edges.begin(), report.edges.end(), [](const auto& item) { return item.kind == ExploreEdgeKind::conditional_branch || item.kind == ExploreEdgeKind::dbcc; });
    report.metrics.fallthroughs = std::count_if(report.edges.begin(), report.edges.end(), [](const auto& item) { return item.kind == ExploreEdgeKind::fallthrough; });
    report.metrics.unresolved_indirect = std::count_if(report.edges.begin(), report.edges.end(), [](const auto& item) { return item.kind == ExploreEdgeKind::unresolved_indirect; });
    report.metrics.dynamic_indirect_edges = std::count_if(report.edges.begin(), report.edges.end(), [](const auto& item) { return item.kind == ExploreEdgeKind::dynamic_indirect; });
    report.metrics.unsupported_instructions = std::count_if(report.stops.begin(), report.stops.end(), [](const auto& item) { return item.reason == StopReason::unsupported_instruction; });
    report.metrics.decode_failures = std::count_if(report.stops.begin(), report.stops.end(), [](const auto& item) { return item.reason == StopReason::decode_failure; });
    report.metrics.frontier_count = report.frontier.size();
    cluster_frontier(report);
    return report;
}

std::string seed_tier_name(SeedTier value) { return "TIER_" + std::to_string(static_cast<int>(value)); }
std::string region_type_name(RegionType value) {
    switch (value) { case RegionType::unknown: return "UNKNOWN"; case RegionType::instruction: return "INSTRUCTION"; case RegionType::known_data: return "KNOWN_DATA"; case RegionType::probable_data: return "PROBABLE_DATA"; case RegionType::pointer_data: return "POINTER_DATA"; case RegionType::conflict: return "CONFLICT"; } return "UNKNOWN";
}
std::string analysis_state_name(AnalysisState value) {
    switch (value) { case AnalysisState::unseen: return "UNSEEN"; case AnalysisState::discovered: return "DISCOVERED"; case AnalysisState::queued: return "QUEUED"; case AnalysisState::analyzing: return "ANALYZING"; case AnalysisState::analyzed: return "ANALYZED"; case AnalysisState::blocked_indirect: return "BLOCKED_INDIRECT"; case AnalysisState::blocked_unsupported: return "BLOCKED_UNSUPPORTED"; case AnalysisState::blocked_data: return "BLOCKED_DATA"; case AnalysisState::conflict: return "CONFLICT"; } return "UNSEEN";
}
std::string explore_edge_name(ExploreEdgeKind value) {
    switch (value) { case ExploreEdgeKind::direct_call: return "DIRECT_CALL"; case ExploreEdgeKind::direct_jump: return "DIRECT_JUMP"; case ExploreEdgeKind::conditional_branch: return "CONDITIONAL_BRANCH"; case ExploreEdgeKind::dbcc: return "DBCC"; case ExploreEdgeKind::fallthrough: return "FALLTHROUGH"; case ExploreEdgeKind::unresolved_indirect: return "UNRESOLVED_INDIRECT"; case ExploreEdgeKind::dynamic_indirect: return "DYNAMIC_INDIRECT"; } return "FALLTHROUGH";
}
std::string stop_reason_name(StopReason value) {
    switch (value) { case StopReason::return_instruction: return "RETURN"; case StopReason::direct_terminal_transfer: return "DIRECT_TERMINAL_TRANSFER"; case StopReason::indirect_transfer: return "INDIRECT_TRANSFER"; case StopReason::unsupported_instruction: return "UNSUPPORTED_INSTRUCTION"; case StopReason::known_data: return "KNOWN_DATA"; case StopReason::out_of_rom: return "OUT_OF_ROM"; case StopReason::conflict: return "CONFLICT"; case StopReason::already_analyzed: return "ALREADY_ANALYZED"; case StopReason::decode_failure: return "DECODE_FAILURE"; } return "DECODE_FAILURE";
}
std::string frontier_type_name(FrontierType value) {
    switch (value) { case FrontierType::indirect_flow: return "INDIRECT_FLOW"; case FrontierType::unsupported: return "UNSUPPORTED"; case FrontierType::decode_failure: return "DECODE_FAILURE"; case FrontierType::boundary_conflict: return "BOUNDARY_CONFLICT"; case FrontierType::code_data_conflict: return "CODE_DATA_CONFLICT"; case FrontierType::out_of_rom: return "OUT_OF_ROM"; case FrontierType::other_unknown: return "OTHER_UNKNOWN"; } return "OTHER_UNKNOWN";
}

} // namespace oasis::tools
