#include "game/spirits/shade_bow_controllers.hpp"

#include <cassert>
#include <cstdint>

int main() {
    using oasis::game::spirits::BowConfig;
    using oasis::game::spirits::BowSpiritController;
    using oasis::game::spirits::ShadeConfig;
    using oasis::game::spirits::ShadeSpiritController;

    // Verify Shade ROM metadata contracts.
    assert(ShadeConfig::kEntityType == 0x001CU);
    assert(ShadeConfig::kStateTableRom == 0x00013008U);
    assert(ShadeConfig::kDespawnHandlerRom == 0x000136CCU);
    assert(ShadeConfig::kSpDrainIntervalFrames == 40U);
    assert(ShadeConfig::kShadowTetherRom == 0x00013036U);
    assert(ShadeConfig::kShadowSurgeRom == 0x00013180U);

    // Shade: inactive controller rejects abilities.
    ShadeSpiritController shade;
    std::uint16_t sp = 50U;
    assert(!shade.cast_shadow_tether(sp));
    assert(!shade.trigger_shadow_surge());
    assert(sp == 50U);

    // Shade: spawn & Shadow Tether SP guard (7 SP -> requires >= 8 SP).
    shade.init_spawn(120, 240, 0);
    assert(shade.is_active());
    assert(shade.type_id() == 0x001CU);
    assert(shade.sp_drain_timer() == 40U);

    sp = 7U;
    assert(!shade.cast_shadow_tether(sp));
    assert(sp == 7U);

    sp = 8U;
    assert(shade.cast_shadow_tether(sp));
    assert(sp == 1U);
    assert(shade.state() == ShadeSpiritController::State::ShadowTether);
    assert(static_cast<std::uint16_t>(shade.state()) == 0x0006U);
    assert(shade.action() == 0x0002U);
    assert(shade.player_lock_flag() == 0xFFU);

    // Shade: Shadow Surge transition (0x013180).
    assert(shade.trigger_shadow_surge());
    assert(shade.state() == ShadeSpiritController::State::ShadowSurge);
    assert(static_cast<std::uint16_t>(shade.state()) == 0x000CU);
    assert(shade.action() == 0x000EU);
    assert(shade.player_lock_flag() == 0U);

    // Shade: 40-frame SP drain reaches 0 and despawns cleanly.
    sp = 1U;
    shade.init_spawn(120, 240, 0);
    for (int f = 0; f < 39; ++f) {
        shade.update_frame(sp);
        assert(shade.is_active());
        assert(sp == 1U);
    }
    shade.update_frame(sp);
    assert(sp == 0U);
    assert(!shade.is_active());
    assert(shade.state() == ShadeSpiritController::State::Dismiss);

    // Verify Bow ROM metadata contracts.
    assert(BowConfig::kEntityType == 0x001EU);
    assert(BowConfig::kStateTableRom == 0x000138E6U);
    assert(BowConfig::kDespawnHandlerRom == 0x00013F22U);
    assert(BowConfig::kSpDrainIntervalFrames == 30U);
    assert(BowConfig::kBiteRom == 0x00013934U);
    assert(BowConfig::kPollenVineRom == 0x000139BAU);
    assert(BowConfig::kPollenVineHelperRom == 0x00013FDCU);
    assert(BowConfig::kPlantStormRom == 0x00013A18U);

    // Bow: spawn & Bite (4 SP).
    BowSpiritController bow;
    bow.init_spawn(300, 400, 16);
    assert(bow.is_active());
    assert(bow.type_id() == 0x001EU);
    assert(bow.sp_drain_timer() == 30U);

    sp = 4U;
    assert(!bow.cast_bite(sp));
    assert(sp == 4U);

    sp = 5U;
    assert(bow.cast_bite(sp));
    assert(sp == 1U);
    assert(bow.state() == BowSpiritController::State::Bite);

    // Bow: Pollen / Vine (9 SP, helper 0x013FDC, State 0x04, Action 0x00).
    sp = 9U;
    assert(!bow.cast_pollen_vine(sp));
    assert(sp == 9U);

    sp = 10U;
    assert(bow.cast_pollen_vine(sp));
    assert(sp == 1U);
    assert(bow.state() == BowSpiritController::State::PollenVine);
    assert(static_cast<std::uint16_t>(bow.state()) == 0x0004U);
    assert(bow.action() == 0x0000U);
    assert(bow.last_helper_pc() == 0x00013FDCU);

    // Bow: Plant Storm (28 SP, State 0x0A, Action 0x10).
    sp = 28U;
    assert(!bow.cast_plant_storm(sp));
    assert(sp == 28U);

    sp = 29U;
    assert(bow.cast_plant_storm(sp));
    assert(sp == 1U);
    assert(bow.state() == BowSpiritController::State::PlantStorm);
    assert(static_cast<std::uint16_t>(bow.state()) == 0x000AU);
    assert(bow.action() == 0x0010U);

    // Bow: 30-frame SP drain reaches 0 and despawns cleanly.
    sp = 1U;
    bow.init_spawn(300, 400, 16);
    for (int f = 0; f < 29; ++f) {
        bow.update_frame(sp);
        assert(bow.is_active());
        assert(sp == 1U);
    }
    bow.update_frame(sp);
    assert(sp == 0U);
    assert(!bow.is_active());
    assert(bow.state() == BowSpiritController::State::Dismiss);

    return 0;
}
