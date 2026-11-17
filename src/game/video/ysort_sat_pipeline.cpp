#include "game/video/ysort_sat_pipeline.hpp"

#include <algorithm>

namespace oasis::game::video {

namespace {

void append_active_fx_pass(
    std::vector<SatRenderOrderEntry>& out,
    SatRenderPassKind pass,
    std::span<const bool> active_flags,
    std::size_t max_slots) {
    const std::size_t count = std::min(active_flags.size(), max_slots);
    for (std::size_t i = 0U; i < count; ++i) {
        if (active_flags[i]) {
            out.push_back(SatRenderOrderEntry{pass, static_cast<std::uint8_t>(i)});
        }
    }
}

} // namespace

std::vector<SatRenderOrderEntry> build_ysort_sat_render_order(
    std::span<const bool> high_priority_fx_active,
    std::span<const bool> hit_sparks_active,
    std::span<const SatRenderSlotInput> entities,
    std::span<const bool> ground_shadow_fx_active,
    std::span<const bool> floor_fx_active) {
    std::vector<SatRenderOrderEntry> out;
    out.reserve(kHighPriorityFxCount + kHitSparksCount + kYSortEntityCount +
                kGroundShadowFxCount + kFloorFxCount);

    append_active_fx_pass(out, SatRenderPassKind::HighPriorityFx,
                          high_priority_fx_active, kHighPriorityFxCount);
    append_active_fx_pass(out, SatRenderPassKind::HitSparks,
                          hit_sparks_active, kHitSparksCount);

    std::vector<std::size_t> active_entities;
    const std::size_t ent_count = std::min(entities.size(), kYSortEntityCount);
    active_entities.reserve(ent_count);
    for (std::size_t i = 0U; i < ent_count; ++i) {
        if (entities[i].active && entities[i].y > kInitialMaxYSentinel) {
            active_entities.push_back(i);
        }
    }
    std::stable_sort(active_entities.begin(), active_entities.end(),
                     [&entities](std::size_t a, std::size_t b) {
                         return entities[a].y > entities[b].y;
                     });
    for (const std::size_t idx : active_entities) {
        out.push_back(SatRenderOrderEntry{
            SatRenderPassKind::YSortedEntity,
            static_cast<std::uint8_t>(idx),
        });
    }

    append_active_fx_pass(out, SatRenderPassKind::GroundShadowFx,
                          ground_shadow_fx_active, kGroundShadowFxCount);
    append_active_fx_pass(out, SatRenderPassKind::FloorFx,
                          floor_fx_active, kFloorFxCount);

    return out;
}

} // namespace oasis::game::video
