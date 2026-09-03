#pragma once

#include "tools/re_atlas.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace oasis::tools {

struct AtlasRankingGroup {
    std::string dimension;
    std::string key;
    std::size_t frequency{};
    std::size_t potentially_resolvable_refs{};
    std::size_t unsupported_items{};
    std::size_t unique_instructions{};
    std::size_t unique_functions{};
    std::string evidence_basis;
};

struct AtlasRankingReport {
    std::size_t atlas_unresolved_reference_count{};
    std::size_t dynamic_resolvable_candidate_count{};
    std::size_t constant_propagation_candidate_count{};
    std::size_t unsupported_decoder_item_count{};
    std::vector<AtlasRankingGroup> groups;
};

[[nodiscard]] AtlasRankingReport rank_atlas_unresolved(const AtlasReport& atlas);
[[nodiscard]] std::string ranking_to_json(const AtlasRankingReport& report);
[[nodiscard]] std::string ranking_to_text(const AtlasRankingReport& report);

} // namespace oasis::tools
