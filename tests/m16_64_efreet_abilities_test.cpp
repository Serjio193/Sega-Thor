#include "game/spirits/efreet_controller.hpp"

#include <cstdlib>
#include <iostream>

#define OASIS_CHECK(cond)                                                                      \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "CHECK FAILED: " #cond " at line " << __LINE__ << std::endl;          \
            std::exit(1);                                                                      \
        }                                                                                      \
    } while (0)

namespace {

void test_shared_sp_guard_0x011B28() {
    using oasis::game::spirits::try_consume_spirit_ability_sp;

    // Must leave >= 1 SP remaining
    std::uint16_t sp = 8U;
    OASIS_CHECK(!try_consume_spirit_ability_sp(sp, 8U));
    OASIS_CHECK(sp == 8U);

    sp = 9U;
    OASIS_CHECK(try_consume_spirit_ability_sp(sp, 8U));
    OASIS_CHECK(sp == 1U);

    sp = 1U;
    OASIS_CHECK(try_consume_spirit_ability_sp(sp, 0U)); // 0-cost dismiss allowed
    OASIS_CHECK(sp == 1U);

    std::cout << "[PASS] Phase 1: Shared Spirit SP-Cost Guard (0x011B28) Verified.\n";
}

void test_efreet_abilities_and_drain() {
    using namespace oasis::game::spirits;
    EfreetSpiritController efreet{};
    efreet.init_spawn(160, 120, 0);
    OASIS_CHECK(efreet.is_active() && efreet.type_id() == 0x001AU);
    OASIS_CHECK(efreet.state() == EfreetSpiritController::State::Follow);

    std::uint16_t sp = 50U;

    // Fire Dash: 8 SP -> State 0x08, Action 0x0C, speed 0x0100, SFX 0x800258
    OASIS_CHECK(efreet.trigger_fire_dash(sp));
    OASIS_CHECK(sp == 42U);
    OASIS_CHECK(efreet.state() == EfreetSpiritController::State::FireDash);
    OASIS_CHECK(efreet.action() == 0x000CU && efreet.speed() == 0x0100U);
    OASIS_CHECK(efreet.last_sound_cmd() == 0x00800258U);

    // Flame Pillar: 6 SP -> State 0x0A, Action 0x02, helper slot $FF1C1C
    OASIS_CHECK(efreet.trigger_flame_pillar(sp));
    OASIS_CHECK(sp == 36U);
    OASIS_CHECK(efreet.state() == EfreetSpiritController::State::FlamePillar);
    OASIS_CHECK(efreet.action() == 0x0002U && efreet.helper_slot_reserved());

    // Meltdown: 25 SP -> State 0x0C, Action 0x08, duration 96 + level (10 -> 106)
    OASIS_CHECK(efreet.trigger_meltdown(sp, 10U));
    OASIS_CHECK(sp == 11U);
    OASIS_CHECK(efreet.state() == EfreetSpiritController::State::Meltdown);
    OASIS_CHECK(efreet.action() == 0x0008U && efreet.ability_timer() == 106U);

    // Meltdown fails if SP <= 25
    OASIS_CHECK(!efreet.trigger_meltdown(sp, 0U));
    OASIS_CHECK(sp == 11U);

    // Dismiss: 0 SP -> State 0x10, Action 0x0A, inactive
    efreet.trigger_dismiss();
    OASIS_CHECK(!efreet.is_active() && efreet.state() == EfreetSpiritController::State::Dismiss);
    OASIS_CHECK(efreet.action() == 0x000AU);

    // 35-frame SP drain despawn
    efreet.init_spawn(160, 120, 0);
    sp = 1U;
    for (int f = 0; f < 34; ++f) {
        efreet.update_frame(sp);
        OASIS_CHECK(efreet.is_active() && sp == 1U);
    }
    efreet.update_frame(sp);
    OASIS_CHECK(sp == 0U && !efreet.is_active());

    std::cout << "[PASS] Phase 2: Efreet (0x001A) Abilities & 35-Frame SP Drain Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.64: SHARED SPIRIT SP GUARD & EFREET (0x001A) ABILITIES\n"
              << "=====================================================================\n";
    test_shared_sp_guard_0x011B28();
    test_efreet_abilities_and_drain();
    std::cout << "\n[PASS] All M16.64 Shared SP Guard & Efreet Tests PASSED!\n";
    return 0;
}
