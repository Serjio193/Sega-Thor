#include "game/enemy/heavy_enemy_dispatch.hpp"

#include <cassert>

int main() {
    using oasis::game::enemy::HeavyEnemySubstateController;
    using oasis::game::enemy::lookup_heavy_enemy_family;

    // 1. Verify ROM family specs (0x0068, 0x0074, 0x0076, 0x0080).
    const auto f68 = lookup_heavy_enemy_family(0x0068U);
    assert(f68.has_value());
    assert(f68->handler_pc == 0x00025B32U);
    assert(f68->substate_count == 5U);
    assert(f68->substate_0_target_pc == 0x00025D30U);
    assert(f68->substate_1_target_pc == 0x000263FAU);

    const auto f74 = lookup_heavy_enemy_family(0x0074U);
    assert(f74.has_value());
    assert(f74->handler_pc == 0x00027C8AU);
    assert(f74->substate_count == 5U);
    assert(f74->substate_0_target_pc == 0x00027DDAU);
    assert(f74->substate_1_target_pc == 0x00027CAAU);

    const auto f76 = lookup_heavy_enemy_family(0x0076U);
    assert(f76.has_value());
    assert(f76->handler_pc == 0x000283E6U);
    assert(f76->substate_mask == 0x7FU);
    assert(f76->substate_count == 4U);
    assert(f76->substate_0_target_pc == 0x000284E8U);
    assert(f76->init_timer_frames == 20U);

    const auto f80 = lookup_heavy_enemy_family(0x0080U);
    assert(f80.has_value());
    assert(f80->handler_pc == 0x00028810U);
    assert(f80->substate_count == 3U);
    assert(f80->substate_0_target_pc == 0x000288C2U);
    assert(f80->init_timer_frames == 40U);
    assert(f80->init_state == 6U);
    assert(f80->init_action == 4U);

    assert(!lookup_heavy_enemy_family(0x0048U).has_value());

    // 2. Controller: 0x0068 substate 0/1/out-of-range.
    HeavyEnemySubstateController ctrl;
    assert(!ctrl.init(0x0099U));
    assert(!ctrl.is_valid());

    assert(ctrl.init(0x0068U));
    assert(ctrl.active_target_pc() == 0x00025D30U);
    ctrl.set_substate(1U);
    assert(ctrl.active_target_pc() == 0x000263FAU);
    ctrl.set_substate(5U); // Out-of-range resets to 0
    assert(ctrl.substate() == 0U);

    // 3. Controller: 0x0076 & 0x7F mask, timer 20, Z 0x20, $90 = $96.
    assert(ctrl.init(0x0076U, 0x00ABU));
    assert(ctrl.timer_frames() == 20U);
    assert(ctrl.z() == 0x0020U);
    assert(ctrl.param_90() == 0x00ABU);
    assert(ctrl.active_target_pc() == 0x000284E8U);
    ctrl.set_substate(0x82U); // Masked with 0x7F -> 0x02 (< 4)
    assert(ctrl.substate() == 2U);
    ctrl.set_substate(0x84U); // Masked with 0x7F -> 0x04 (>= 4) -> resets to 0
    assert(ctrl.substate() == 0U);

    // 4. Controller: 0x0080 timer 40, State 6, Action 4, $90 = $96.
    assert(ctrl.init(0x0080U, 0x0015U));
    assert(ctrl.timer_frames() == 40U);
    assert(ctrl.state() == 6U);
    assert(ctrl.action() == 4U);
    assert(ctrl.param_90() == 0x0015U);
    assert(ctrl.active_target_pc() == 0x000288C2U);
    ctrl.advance_substate();
    ctrl.advance_substate();
    assert(ctrl.substate() == 2U);
    ctrl.advance_substate();
    assert(ctrl.substate() == 0U);

    return 0;
}
