#include "game/enemy/dungeon_enemy_dispatch.hpp"

#include <cassert>

int main() {
    using oasis::game::enemy::DungeonEnemySubstateController;
    using oasis::game::enemy::lookup_006c_variant_param;
    using oasis::game::enemy::lookup_dungeon_enemy_family;
    using oasis::game::enemy::resolve_0072_substate_target;

    // 1. Family lookup contracts (0x006C, 0x006E, 0x0070, 0x0072).
    const auto f6c = lookup_dungeon_enemy_family(0x006CU);
    assert(f6c.has_value());
    assert(f6c->handler_pc == 0x0001F5A4U);
    assert(f6c->substate_count == 3U);
    assert(f6c->variant_table_pc == 0x0001F5C8U);

    const auto f6e = lookup_dungeon_enemy_family(0x006EU);
    assert(f6e.has_value());
    assert(f6e->handler_pc == 0x0001EB54U);
    assert(f6e->substate_count == 10U);

    const auto f70 = lookup_dungeon_enemy_family(0x0070U);
    assert(f70.has_value());
    assert(f70->handler_pc == 0x0001DF1CU);
    assert(f70->substate_count == 6U);
    assert(f70->shared_helper_pc == 0x0001DFA4U);

    const auto f72 = lookup_dungeon_enemy_family(0x0072U);
    assert(f72.has_value());
    assert(f72->handler_pc == 0x0001D140U);
    assert(f72->substate_count == 10U);
    assert(f72->jump_table_pc == 0x0001D152U);

    assert(!lookup_dungeon_enemy_family(0x0048U).has_value());

    // 2. 0x006C variant table [3, 6, 9, 12] ($96 -> $90).
    assert(lookup_006c_variant_param(0U) == 3U);
    assert(lookup_006c_variant_param(1U) == 6U);
    assert(lookup_006c_variant_param(2U) == 9U);
    assert(lookup_006c_variant_param(3U) == 12U);
    assert(lookup_006c_variant_param(4U) == 0U);

    DungeonEnemySubstateController ctrl;
    assert(!ctrl.init(0x0099U));
    assert(!ctrl.is_valid());

    assert(ctrl.init(0x006CU, 2U));
    assert(ctrl.is_valid());
    assert(ctrl.variant_index_006c() == 2U);
    assert(ctrl.param_90() == 9U);
    assert(ctrl.active_target_pc() == 0x0001F5A4U);
    ctrl.set_substate(2U);
    assert(ctrl.substate() == 2U);
    ctrl.set_substate(3U); // Out-of-range resets to 0
    assert(ctrl.substate() == 0U);

    // 3. 0x0072 jump table resolution (0..9).
    assert(ctrl.init(0x0072U));
    assert(resolve_0072_substate_target(0U) == 0x0001D412U);
    assert(resolve_0072_substate_target(1U) == 0x0001DA20U);
    assert(resolve_0072_substate_target(2U) == 0x0001DC14U);
    assert(resolve_0072_substate_target(3U) == 0x0001D166U);
    assert(resolve_0072_substate_target(9U) == 0x0001D23EU);
    assert(resolve_0072_substate_target(10U) == 0U);

    ctrl.set_substate(9U);
    assert(ctrl.active_target_pc() == 0x0001D23EU);
    ctrl.advance_substate();
    assert(ctrl.substate() == 0U);
    assert(ctrl.active_target_pc() == 0x0001D412U);

    return 0;
}
