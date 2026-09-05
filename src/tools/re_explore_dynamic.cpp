#include "tools/re_explore.hpp"

namespace oasis::tools {

const DynamicEdgeEvidence* find_dynamic_edge(
    std::span<const DynamicEdgeEvidence> edges, std::uint32_t source_entry,
    std::uint32_t source_pc) {
    for (const auto& edge : edges) {
        if (edge.source_entry == source_entry && edge.source_pc == source_pc &&
            edge.evidence_class != "FORCED_HYPOTHESIS" &&
            edge.evidence_class != "DYNAMIC_STATE_GUIDED")
            return &edge;
    }
    return nullptr;
}

} // namespace oasis::tools
