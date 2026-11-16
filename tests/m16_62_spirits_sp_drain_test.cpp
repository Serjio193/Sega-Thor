#include "game/spirits/all_spirits_registry.hpp"

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

void test_sp_drain_intervals_and_tables() {
    using namespace oasis::game::spirits;

    const std::uint16_t expected_intervals[4] = {45U, 35U, 40U, 30U};
    const std::uint32_t expected_despawns[4] = {0x012314U, 0x012BA8U, 0x0136CCU, 0x013F22U};
    const std::uint32_t expected_tables[4] = {0x011BACU, 0x0125AEU, 0x013008U, 0x0138E6U};

    for (std::uint8_t idx = 0U; idx < 4U; ++idx) {
        const auto spec = get_spirit_spec(idx);
        OASIS_CHECK(spec.has_value());
        OASIS_CHECK(spec->sp_drain_interval_frames == expected_intervals[idx]);
        OASIS_CHECK(spec->despawn_handler_pc == expected_despawns[idx]);
        OASIS_CHECK(spec->state_table_pc == expected_tables[idx]);
    }
    std::cout << "[PASS] Phase 1: All 4 Spirits SP Drain Intervals & State Tables Verified.\n";
}

void test_live_sp_drain_and_despawn() {
    using namespace oasis::game::spirits;

    const std::uint16_t expected_intervals[4] = {45U, 35U, 40U, 30U};
    const std::uint32_t expected_despawns[4] = {0x012314U, 0x012BA8U, 0x0136CCU, 0x013F22U};

    for (std::uint8_t idx = 0U; idx < 4U; ++idx) {
        ActiveSpiritController ctrl{};
        OASIS_CHECK(ctrl.summon_spirit_by_index(idx, 160, 120));
        OASIS_CHECK(ctrl.is_active());
        OASIS_CHECK(ctrl.sp_drain_timer() == expected_intervals[idx]);
        if (idx == 0U) {
            OASIS_CHECK(ctrl.dytto().is_active());
        }

        std::uint16_t player_sp = 2U;
        // Tick interval - 1 frames -> SP still 2
        for (std::uint16_t f = 0U; f + 1U < expected_intervals[idx]; ++f) {
            OASIS_CHECK(!ctrl.step_sp_drain(player_sp));
            OASIS_CHECK(player_sp == 2U && ctrl.is_active());
        }
        // Frame `interval`: SP decrements 2 -> 1, timer resets, still active
        OASIS_CHECK(!ctrl.step_sp_drain(player_sp));
        OASIS_CHECK(player_sp == 1U && ctrl.is_active());
        OASIS_CHECK(ctrl.sp_drain_timer() == expected_intervals[idx]);

        // Tick another `interval` frames: SP decrements 1 -> 0, despawns cleanly!
        bool despawned = false;
        for (std::uint16_t f = 0U; f < expected_intervals[idx]; ++f) {
            despawned = ctrl.step_sp_drain(player_sp);
        }
        OASIS_CHECK(despawned);
        OASIS_CHECK(player_sp == 0U && !ctrl.is_active() && ctrl.despawned());
        OASIS_CHECK(ctrl.last_despawn_handler_pc() == expected_despawns[idx]);
        OASIS_CHECK(!ctrl.dytto().is_active());
    }

    std::cout << "[PASS] Phase 2: Live SP Drain Countdown (45/35/40/30f) & Clean Despawn Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.62: ALL-SPIRITS LIVE SP DRAIN & DESPAWN CONTRACTS\n"
              << "=====================================================================\n";
    test_sp_drain_intervals_and_tables();
    test_live_sp_drain_and_despawn();
    std::cout << "\n[PASS] All M16.62 Spirits SP Drain & Despawn Tests PASSED!\n";
    return 0;
}
