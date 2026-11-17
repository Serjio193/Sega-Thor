#include "game/video/palette_fade.hpp"

#include <array>
#include <cassert>

int main() {
    using oasis::game::video::PaletteFadeConfig;
    using oasis::game::video::PaletteFadeDirection;
    using oasis::game::video::PaletteFadeEngine;
    using oasis::game::video::scale_bgr333_color;

    // 1. Verify exact 0x0EEE ramp at 0x002FDA.
    const std::array<std::uint16_t, 8> expected_ramp = {
        0x0000U, 0x0222U, 0x0444U, 0x0666U, 0x0888U, 0x0AAAU, 0x0CCCU, 0x0EEEU,
    };
    for (std::uint8_t s = 0U; s < 8U; ++s) {
        assert(scale_bgr333_color(0x0EEEU, s) == expected_ramp[s]);
    }
    assert(scale_bgr333_color(0x0ACEU, 7U) == 0x0ACEU);
    assert(scale_bgr333_color(0x0ACEU, 0U) == 0x0000U);

    // 2. Fade In (steps 0..7, delay = 1 -> 2 frames per step, unblanks on step 0).
    PaletteFadeEngine engine;
    std::array<std::uint16_t, PaletteFadeConfig::kCramColorCount> pal{};
    pal[0] = 0x0EEEU;
    pal[1] = 0x0842U;
    engine.set_palette(pal);

    engine.start_fade_in(1U);
    assert(engine.is_active());
    assert(engine.direction() == PaletteFadeDirection::FadeIn);
    assert(engine.current_step() == 0U);
    assert(engine.cram_shadow()[0] == 0x0000U);
    assert((engine.vdp_flags_ff164d() & PaletteFadeConfig::kFlagDisplayUnblank) != 0U);
    assert((engine.vdp_flags_ff164d() & PaletteFadeConfig::kFlagCramDmaDirty) != 0U);

    engine.clear_cram_dma_dirty();
    assert(engine.step_frame()); // delay frame 1/2
    assert(engine.current_step() == 0U);
    assert(engine.step_frame()); // advances to step 1
    assert(engine.current_step() == 1U);
    assert(engine.cram_shadow()[0] == 0x0222U);

    while (engine.step_frame()) {}
    assert(!engine.is_active());
    assert(engine.current_step() == 7U);
    assert(engine.cram_shadow()[0] == 0x0EEEU);
    assert(engine.cram_shadow()[1] == 0x0842U);

    // 3. Fade Out (steps 7..0, restores $FF18A6 -> $FF134C on completion).
    engine.start_fade_out(0U);
    assert(engine.is_active());
    assert(engine.current_step() == 7U);
    assert(engine.cram_shadow()[0] == 0x0EEEU);

    for (int s = 6; s >= 0; --s) {
        assert(engine.step_frame());
        assert(engine.current_step() == static_cast<std::uint8_t>(s));
        assert(engine.cram_shadow()[0] == expected_ramp[static_cast<std::size_t>(s)]);
    }
    assert(!engine.step_frame()); // Completes and restores backup -> shadow
    assert(!engine.is_active());
    assert(engine.cram_shadow()[0] == 0x0EEEU);
    assert(engine.cram_shadow()[1] == 0x0842U);

    return 0;
}
