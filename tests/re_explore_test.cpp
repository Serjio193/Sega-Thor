#include "tools/re_explore.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using namespace oasis::tools;

CandidateMapReport candidates(std::initializer_list<std::pair<std::uint32_t, std::uint32_t>> ranges) {
    CandidateMapReport result;
    for (const auto [entry, end] : ranges) {
        CandidateRecord item;
        item.entry = entry;
        item.ghidra_function = true;
        item.ghidra_range_start = entry;
        item.ghidra_range_end = end;
        item.classification = CandidateClassification::ghidra_only;
        result.candidates.push_back(item);
    }
    return result;
}

AtlasReport data_at(std::uint32_t start, std::uint32_t end) {
    AtlasReport result;
    result.entries.push_back({"data", AtlasEntryType::table, start, end, end, AtlasConfidence::unknown,
                              AtlasConfidence::confirmed});
    return result;
}

const ExploreEdge* edge(const ExploreReport& report, std::uint32_t pc, ExploreEdgeKind kind) {
    for (const auto& item : report.edges)
        if (item.source_pc == pc && item.kind == kind) return &item;
    return nullptr;
}

bool has_frontier(const ExploreReport& report, FrontierType type) {
    for (const auto& item : report.frontier) if (item.blocker_type == type) return true;
    return false;
}

int main() {
    {
        std::vector<std::uint8_t> rom = {
            0x61, 0x04, 0x66, 0x00, 0x00, 0x04, 0x4E, 0x71,
            0x60, 0xF8, 0x4E, 0x75};
        ExploreOptions options;
        options.control_entries = {0};
        options.control_edges.clear();
        const auto report = explore_m68k(rom, candidates({{0, 4}}), AtlasReport{}, options);
        assert(report.bounded_control_pass);
        assert(edge(report, 0, ExploreEdgeKind::direct_call));
        assert(edge(report, 0, ExploreEdgeKind::fallthrough));
        assert(edge(report, 2, ExploreEdgeKind::conditional_branch));
        assert(edge(report, 2, ExploreEdgeKind::fallthrough));
        assert(edge(report, 8, ExploreEdgeKind::direct_jump));
        assert(!edge(report, 8, ExploreEdgeKind::fallthrough));
        assert(!report.processing_order.empty() && report.processing_order.front() == 0U);
        assert(std::find(report.processing_order.begin(), report.processing_order.end(), 6U) != report.processing_order.end());
        assert(!report.stops.empty());
    }
    {
        std::vector<std::uint8_t> rom = {0x51, 0xC8, 0x00, 0x04, 0x4E, 0x75, 0x4E, 0x75};
        ExploreOptions options;
        options.control_entries = {0};
        options.control_edges.clear();
        const auto report = explore_m68k(rom, candidates({{0, 4}}), AtlasReport{}, options);
        assert(edge(report, 0, ExploreEdgeKind::dbcc));
        assert(edge(report, 0, ExploreEdgeKind::fallthrough));
    }
    {
        std::vector<std::uint8_t> rom = {0x4E, 0x75, 0x4E, 0x73, 0x4E, 0x75, 0x4E, 0xD0};
        ExploreOptions options;
        options.control_entries = {0, 2, 6};
        options.control_edges.clear();
        const auto report = explore_m68k(rom, candidates({{0, 2}, {2, 4}, {6, 8}}), AtlasReport{}, options);
        assert(report.metrics.analyzed_entries >= 2);
        assert(has_frontier(report, FrontierType::indirect_flow));
        assert(report.metrics.unresolved_indirect == 1);
        assert(report.address_map.size() >= 2);
    }
    {
        std::vector<std::uint8_t> rom = {0x4E, 0x71, 0xFF, 0xFF, 0x4E, 0x75, 0x00, 0x00};
        ExploreOptions options;
        options.control_entries = {0};
        options.control_edges.clear();
        const auto report = explore_m68k(rom, candidates({{0, 6}}), AtlasReport{}, options);
        assert(has_frontier(report, FrontierType::unsupported));
        assert(report.metrics.blocked_unsupported_entries == 1);
    }
    {
        std::vector<std::uint8_t> rom = {0x4E, 0x71, 0x4E, 0x75, 0x00, 0x00, 0x00, 0x00};
        ExploreOptions options;
        options.control_entries = {0, 2};
        options.control_edges.clear();
        const auto report = explore_m68k(rom, candidates({{0, 4}, {2, 4}}), data_at(2, 4), options);
        assert(has_frontier(report, FrontierType::code_data_conflict));
        assert(report.metrics.conflict_entries == 1);
        assert(report.metrics.conflict_bytes > 0);
        const auto json_a = explore_to_json(report);
        const auto json_b = explore_to_json(report);
        assert(json_a == json_b);
        assert(json_a.find("address_map") != std::string::npos);
    }
    {
        std::vector<std::uint8_t> rom = {0x4E, 0x71, 0x4E, 0x75};
        ExploreOptions options;
        options.control_entries = {0, 2};
        options.control_edges.clear();
        const auto report = explore_m68k(rom, candidates({{0, 2}, {2, 4}}), AtlasReport{}, options);
        bool shared_owner = false;
        for (const auto& range : report.address_map)
            if (range.owners.size() > 1U) shared_owner = true;
        assert(shared_owner);
        assert(report.metrics.conflict_bytes == 0);
    }
    return 0;
}
