#include "tools/re_candidate_map.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string_view>

namespace oasis::tools {
namespace {

struct WorkingRecord {
    CandidateRecord record;
    const GhidraFunction* function{};
};

struct StaticEdge {
    std::uint32_t caller{};
    std::uint32_t target{};
    std::uint32_t site{};
};

constexpr std::uint32_t kA7E2 = 0xA7E2U;
constexpr std::uint32_t kConfirmed[] = {
    0x3820U, 0x7A28U, 0x82AEU, 0x8E90U, 0x938EU, 0x9BF2U,
    0xA6A4U, 0xD3B2U, 0x60004U, 0x604BCU, 0x6121AU};

struct DynamicFact {
    std::uint32_t address;
    const char* source;
};

constexpr DynamicFact kDynamicFacts[] = {
    {0xA6A4U, "existing bounded A6A4 scenario"},
    {0x60B8CU, "existing BizHawk start_pulse_120 scenario"},
    {0x611EEU, "existing BizHawk natural_idle_to_6121a scenario"},
    {0x6121AU, "existing BizHawk natural caller/target scenario"}};

bool contains(std::span<const std::uint32_t> values, std::uint32_t value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

void add_unique(std::vector<std::uint32_t>& values, std::uint32_t value) {
    if (!contains(values, value)) values.push_back(value);
}

void add_flag(std::vector<std::string>& values, const char* value) {
    if (std::find(values.begin(), values.end(), value) == values.end()) values.emplace_back(value);
}

void sort_unique(std::vector<std::uint32_t>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

bool is_confirmed(std::uint32_t address) {
    return std::find(std::begin(kConfirmed), std::end(kConfirmed), address) != std::end(kConfirmed);
}

std::vector<StaticEdge> existing_static_edges(const AtlasReport& atlas) {
    std::vector<StaticEdge> result;
    for (const auto& edge : atlas.call_edges)
        for (const auto site : edge.call_sites) result.push_back({edge.caller, edge.callee, site});
    // These three call sites and the reset-slice branch are durable ledger evidence.
    result.push_back({0x60B8CU, 0x6121AU, 0x60B8CU});
    result.push_back({0x60D4AU, 0x6121AU, 0x60D4AU});
    result.push_back({0x611EEU, 0x6121AU, 0x611EEU});
    result.push_back({0x60004U, 0x6042AU, 0x60004U});
    return result;
}

bool range_contains(const CandidateRecord& record, std::uint32_t address) {
    return record.ghidra_range_start && record.ghidra_range_end &&
           address >= *record.ghidra_range_start && address < *record.ghidra_range_end;
}

bool interval_overlaps(const CandidateRecord& record, std::uint32_t start, std::uint32_t end) {
    return record.ghidra_range_start && record.ghidra_range_end &&
           *record.ghidra_range_start < end && *record.ghidra_range_end > start;
}

CandidateComplexity complexity(const GhidraFunction* function, bool indirect) {
    if (!function) return CandidateComplexity::unknown;
    if (function->calls.empty() && function->has_return) return CandidateComplexity::leaf;
    if (!indirect && function->calls.size() <= 3U && function->basic_block_count <= 8U)
        return CandidateComplexity::shallow;
    return CandidateComplexity::complex;
}

std::string atlas_relation(const AtlasReport& atlas, std::uint32_t address, bool& present,
                           bool& exact) {
    const auto entries = atlas_entries_at(atlas, address);
    present = !entries.empty();
    for (const auto* entry : entries) {
        if (entry->start == address) {
            exact = true;
            return "entry:" + entry->verification_status;
        }
    }
    if (!entries.empty()) return "covered_by:" + entries.front()->id;
    return "absent";
}

std::string beta_kind(const std::optional<AtlasBetaCorrespondence>& beta,
                      std::optional<std::uint32_t>& address) {
    if (!beta) return "unknown";
    address = beta->address;
    switch (beta->match) {
    case AtlasCorrespondence::exact: return "exact";
    case AtlasCorrespondence::structural: return "structural";
    case AtlasCorrespondence::changed: return "changed";
    default: return "unknown";
    }
}

void merge_ghidra_function(WorkingRecord& working, const GhidraFunction& function) {
    if (working.function) return;
    working.function = &function;
    auto& record = working.record;
    record.ghidra_function = true;
    record.ghidra_range_start = function.range_start;
    record.ghidra_range_end = function.range_end;
    record.ghidra_calls = function.calls;
    record.ghidra_called_by = function.called_by;
    add_flag(record.source_flags, "GHIDRA_FUNCTION");
}

void merge_ghidra_candidate(WorkingRecord& working, const GhidraCandidate& candidate) {
    auto& record = working.record;
    if (!record.ghidra_function) {
        record.ghidra_range_start = candidate.range_start;
        record.ghidra_range_end = candidate.range_end;
    }
    if (candidate.source.find("DIRECT_CALL") != std::string::npos ||
        candidate.source == "VECTOR_OR_DIRECT_CALL_TARGET")
        add_flag(record.source_flags, "GHIDRA_DIRECT_CALL_TARGET");
}

void add_static_signals(CandidateRecord& record, const std::vector<StaticEdge>& edges) {
    for (const auto& edge : edges) {
        if (edge.target == record.entry) {
            record.known_direct_call_target = true;
            add_flag(record.source_flags, "KNOWN_STATIC_TARGET");
        }
        if (edge.caller == record.entry || edge.site == record.entry) {
            record.known_direct_caller = true;
            add_unique(record.known_direct_call_sites, edge.site);
            add_flag(record.source_flags, "KNOWN_STATIC_TARGET");
        }
    }
    sort_unique(record.known_direct_call_sites);
}

void add_dynamic_signals(CandidateRecord& record, const AtlasReport& atlas) {
    for (const auto& fact : kDynamicFacts) {
        if (fact.address == record.entry) {
            record.dynamic_observed = true;
            record.dynamic_sources.emplace_back(fact.source);
            add_flag(record.source_flags, "DYNAMIC_PC");
        }
    }
    for (const auto* entry : atlas_entries_at(atlas, record.entry)) {
        if (entry->start != record.entry || !entry->dynamic) continue;
        record.dynamic_observed = true;
        record.dynamic_sources.emplace_back("existing Atlas dynamic summary");
        add_flag(record.source_flags, "DYNAMIC_PC");
        if (record.entry == 0xA6A4U) record.indirect_flow = true;
    }
}

void add_conflicts(CandidateRecord& record, const AtlasReport& atlas) {
    for (const auto& table : atlas.entries) {
        if (table.type != AtlasEntryType::table) continue;
        const auto end = table.end ? table.end : table.bounded_evidence_end;
        if (range_contains(record, table.start) || (end && interval_overlaps(record, table.start, *end))) {
            record.code_data_conflict = true;
            break;
        }
    }
    const auto exact_entries = atlas_entries_at(atlas, record.entry);
    for (const auto* entry : exact_entries) {
        if (entry->start != record.entry || !entry->end || entry->boundary_confidence != AtlasConfidence::confirmed)
            continue;
        if (record.ghidra_range_end && *record.ghidra_range_end != *entry->end)
            record.boundary_conflict = true;
    }
    record.indirect_flow = record.indirect_flow || range_contains(record, kA7E2);
}

void classify(CandidateRecord& record, bool exact_atlas) {
    if (is_confirmed(record.entry)) record.classification = CandidateClassification::confirmed;
    else if (record.code_data_conflict) record.classification = CandidateClassification::conflict;
    else if (exact_atlas || record.known_direct_call_target || record.known_direct_caller)
        record.classification = CandidateClassification::static_supported;
    else if (record.dynamic_observed) record.classification = CandidateClassification::dynamic_observed;
    else record.classification = CandidateClassification::ghidra_only;
}

void score(CandidateRecord& record, bool exact_atlas) {
    int result = 0;
    const auto add = [&](int points, const char* reason) {
        result += points;
        if (points > 0) record.ranking_reasons.emplace_back(std::string("+") + std::to_string(points) + " " + reason);
        else if (points < 0) record.ranking_reasons.emplace_back(std::to_string(points) + " " + reason);
    };
    if (record.dynamic_observed) add(40, "dynamic observed");
    if (record.known_direct_call_target) add(20, "known direct call target");
    if (record.known_direct_caller) add(10, "known direct caller/call site");
    if (record.beta_match_kind == "exact") add(10, "beta exact correspondence");
    else if (record.beta_match_kind == "structural") add(8, "beta structural correspondence");
    else if (record.beta_match_kind == "changed") add(4, "beta changed correspondence");
    if (record.complexity == CandidateComplexity::leaf) add(10, "leaf-like structure");
    else if (record.complexity == CandidateComplexity::shallow) add(6, "shallow structure");
    else if (record.complexity == CandidateComplexity::complex) add(-8, "complex structure");
    if (record.ghidra_function && !record.indirect_flow) add(4, "no observed indirect flow");
    if (exact_atlas) add(5, "Atlas entry exists");
    if (record.code_data_conflict || record.boundary_conflict) add(-50, "evidence conflict");
    if (record.classification == CandidateClassification::ghidra_only &&
        record.ghidra_calls.empty() && record.ghidra_called_by.empty()) add(-5, "Ghidra-only without xrefs");
    record.ranking_score = result;
}

void audit(CandidateRecord& record) {
    if (record.classification == CandidateClassification::conflict || record.code_data_conflict) {
        record.quality_audit = "CONFLICT";
    } else if (record.known_direct_call_target || record.known_direct_caller || record.dynamic_observed) {
        record.quality_audit = "GOOD_NEXT_TARGET";
    } else if (record.classification == CandidateClassification::ghidra_only &&
               record.ghidra_calls.empty() && record.ghidra_called_by.empty()) {
        record.quality_audit = "LOW_VALUE";
    } else {
        record.quality_audit = "PLAUSIBLE";
    }
}

} // namespace

CandidateMapReport build_candidate_map(const GhidraMap& ghidra, const AtlasReport& atlas) {
    std::map<std::uint32_t, WorkingRecord> records;
    for (const auto& function : ghidra.functions) {
        auto& working = records[function.entry];
        working.record.entry = function.entry;
        merge_ghidra_function(working, function);
    }
    for (const auto& candidate : ghidra.candidates) {
        auto& working = records[candidate.entry];
        working.record.entry = candidate.entry;
        merge_ghidra_candidate(working, candidate);
    }
    for (const auto& entry : atlas.entries) {
        auto& working = records[entry.start];
        working.record.entry = entry.start;
        working.record.atlas_present = true;
        add_flag(working.record.source_flags, "ATLAS_ENTRY");
    }
    for (const auto& fact : kDynamicFacts) records[fact.address].record.entry = fact.address;
    const auto edges = existing_static_edges(atlas);
    for (const auto& edge : edges) {
        records[edge.target].record.entry = edge.target;
        records[edge.site].record.entry = edge.site;
    }

    CandidateMapReport report;
    report.ghidra_schema = ghidra.schema;
    report.ghidra_program = ghidra.program_name;
    report.ghidra_language = ghidra.language_id;
    report.ghidra_analysis_mode = ghidra.analysis_mode;
    report.ghidra_semantic_status = ghidra.semantic_status;
    for (auto& [address, working] : records) {
        auto& record = working.record;
        bool exact_atlas = false;
        record.atlas_status = atlas_relation(atlas, address, record.atlas_present, exact_atlas);
        add_static_signals(record, edges);
        add_dynamic_signals(record, atlas);
        if (exact_atlas) {
            record.beta_match_kind = beta_kind(atlas_beta_lookup(atlas, address), record.beta_address);
            if (record.beta_match_kind != "unknown") add_flag(record.source_flags, "BETA_CORRESPONDENCE");
        }
        add_conflicts(record, atlas);
        sort_unique(record.ghidra_calls);
        sort_unique(record.ghidra_called_by);
        if (record.ghidra_function) record.complexity = complexity(working.function, record.indirect_flow);
        classify(record, exact_atlas);
        score(record, exact_atlas);
        report.candidates.push_back(std::move(record));
    }
    std::sort(report.candidates.begin(), report.candidates.end(), [](const auto& left, const auto& right) {
        if (left.ranking_score != right.ranking_score) return left.ranking_score > right.ranking_score;
        return left.entry < right.entry;
    });
    const auto count = [&](CandidateClassification value) {
        return static_cast<std::size_t>(std::count_if(report.candidates.begin(), report.candidates.end(),
            [=](const auto& item) { return item.classification == value; }));
    };
    report.confirmed_count = count(CandidateClassification::confirmed);
    report.static_supported_count = count(CandidateClassification::static_supported);
    report.dynamic_observed_count = count(CandidateClassification::dynamic_observed);
    report.ghidra_only_count = count(CandidateClassification::ghidra_only);
    report.conflict_count = count(CandidateClassification::conflict);
    report.leaf_count = static_cast<std::size_t>(std::count_if(report.candidates.begin(), report.candidates.end(),
        [](const auto& item) { return item.complexity == CandidateComplexity::leaf; }));
    report.shallow_count = static_cast<std::size_t>(std::count_if(report.candidates.begin(), report.candidates.end(),
        [](const auto& item) { return item.complexity == CandidateComplexity::shallow; }));
    report.complex_count = static_cast<std::size_t>(std::count_if(report.candidates.begin(), report.candidates.end(),
        [](const auto& item) { return item.complexity == CandidateComplexity::complex; }));
    report.unknown_complexity_count = report.candidates.size() - report.leaf_count - report.shallow_count - report.complex_count;
    std::size_t audited{};
    for (auto& item : report.candidates) if (item.classification != CandidateClassification::confirmed && audited++ < 10U) audit(item);
    for (const auto& item : report.candidates) {
        if (item.classification == CandidateClassification::confirmed || !item.ghidra_function ||
            item.code_data_conflict || item.boundary_conflict) continue;
        report.recommended_next_target = item.entry;
        report.recommended_reason = "highest-ranked non-confirmed Ghidra function with existing static support and no recorded conflict";
        break;
    }
    return report;
}

std::string candidate_classification_name(CandidateClassification value) {
    switch (value) {
    case CandidateClassification::confirmed: return "CONFIRMED";
    case CandidateClassification::static_supported: return "STATIC_SUPPORTED";
    case CandidateClassification::dynamic_observed: return "DYNAMIC_OBSERVED";
    case CandidateClassification::ghidra_only: return "GHIDRA_ONLY";
    case CandidateClassification::conflict: return "CONFLICT";
    }
    return "GHIDRA_ONLY";
}

std::string candidate_complexity_name(CandidateComplexity value) {
    switch (value) {
    case CandidateComplexity::leaf: return "LEAF";
    case CandidateComplexity::shallow: return "SHALLOW";
    case CandidateComplexity::complex: return "COMPLEX";
    case CandidateComplexity::unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // namespace oasis::tools
