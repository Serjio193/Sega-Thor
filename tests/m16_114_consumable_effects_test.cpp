#include "game/player/consumable_effects.hpp"

#include <cassert>

namespace {

using namespace oasis::game::player;

void test_area_block_and_spirit_gem_guards() {
    ConsumableContext ctx{};
    ctx.area_id = 0x0010U;
    ctx.hp = 50;
    ctx.max_hp = 200;
    assert(!can_use_consumable(0x1EU, ctx)); // Area 0x10 blocks all consumables

    ctx.area_id = 0x0001U;
    ctx.sp = 40;
    ctx.max_sp = 100;
    ctx.unlocked_spirit_mask = 0x01U; // Dytto unlocked (bit 0)

    assert(can_use_consumable(0x1AU, ctx));
    assert(!can_use_consumable(0x1BU, ctx)); // Efreet not unlocked

    ctx.spirit_active = true;
    assert(!can_use_consumable(0x1AU, ctx)); // already active
    ctx.spirit_active = false;

    ctx.summon_locked = true;
    assert(!can_use_consumable(0x1AU, ctx)); // summon locked ($FF16F1 & 1)
    ctx.summon_locked = false;

    ctx.sp = 0;
    assert(!can_use_consumable(0x1AU, ctx)); // SP <= 0

    ctx.sp = 25;
    const auto gem_res = apply_consumable_effect(0x1AU, ctx);
    assert(gem_res.applied);
    assert(gem_res.summon_requested);
    assert(gem_res.summon_spirit_index == 0U);
    assert(ctx.sp == 100);
    assert(ctx.spirit_active);
}

void test_hp_sp_and_elixir_restore_tiers() {
    ConsumableContext ctx{};
    ctx.area_id = 0x0001U;
    ctx.max_hp = 200;
    ctx.max_sp = 100;

    // HP Food 25% (0x1E -> +50), 50% (0x1F -> +100), 75% (0x20 -> +150), 100% (0x21 -> +200)
    ctx.hp = 10;
    assert(can_use_consumable(0x1EU, ctx));
    const auto r25 = apply_consumable_effect(0x1EU, ctx);
    assert(r25.applied && r25.hp_delta == 50 && ctx.hp == 60);
    assert(r25.sound_cmd == kConsumableHealSfxCmd);

    const auto r50 = apply_consumable_effect(0x1FU, ctx);
    assert(r50.applied && r50.hp_delta == 100 && ctx.hp == 160);

    // Clamped to max_hp (160 + 150 -> 200)
    const auto r75 = apply_consumable_effect(0x20U, ctx);
    assert(r75.applied && r75.hp_delta == 40 && ctx.hp == 200);
    assert(!can_use_consumable(0x21U, ctx)); // already at max_hp

    // SP Food 50% (0x23 -> +50)
    ctx.sp = 10;
    assert(can_use_consumable(0x23U, ctx));
    const auto sp50 = apply_consumable_effect(0x23U, ctx);
    assert(sp50.applied && sp50.sp_delta == 50 && ctx.sp == 60);

    // Elixir 100% (0x29) restores both HP and SP
    ctx.hp = 50;
    ctx.sp = 20;
    assert(can_use_consumable(0x29U, ctx));
    const auto elx = apply_consumable_effect(0x29U, ctx);
    assert(elx.applied && ctx.hp == 200 && ctx.sp == 100);
    assert(elx.sound_cmd == kConsumableHealSfxCmd);
}

} // namespace

int main() {
    test_area_block_and_spirit_gem_guards();
    test_hp_sp_and_elixir_restore_tiers();
    return 0;
}
