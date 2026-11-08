#pragma once

#include "game/render/sat_pipeline_contract.hpp"

#include <cstdint>

namespace oasis::game::render {

// Authentic Beyond Oasis HUD Overlay Generator based on ROM routine 0x003FA0.
// Renders health and spirit/magic gauge bars and base frames into SAT slots 0..5.
class HudOverlay {
public:
    static constexpr std::size_t kHudSpriteCount = 6;

    // Base pattern indices for HUD sprites
    static constexpr std::uint16_t kHealthFrameTile = 0x0781;
    static constexpr std::uint16_t kHealthBarPart1Tile = 0x0787;
    static constexpr std::uint16_t kHealthBarPart2Tile = 0x078B;
    static constexpr std::uint16_t kMagicFrameTile = 0x078E;
    static constexpr std::uint16_t kMagicBarPart1Tile = 0x0794;
    static constexpr std::uint16_t kMagicBarPart2Tile = 0x0798;

    HudOverlay() noexcept = default;

    // Emits the 6 authentic HUD sprites into the SAT pipeline.
    // Slots 0..2: Health gauge base and bars.
    // Slots 3..5: Magic/spirit gauge base and bars.
    bool emit_hud(SatPipelineContract& sat,
                  int hp = 100, int max_hp = 100,
                  int sp = 100, int max_sp = 100) noexcept;

    [[nodiscard]] static constexpr bool is_hud_slot(std::size_t slot) noexcept {
        return slot < kHudSpriteCount;
    }
};

} // namespace oasis::game::render
