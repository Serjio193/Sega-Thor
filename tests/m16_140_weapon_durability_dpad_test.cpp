#include "game/player/ali_weapon_durability.hpp"

#include <cassert>
#include <iostream>

using namespace oasis::game::player;

static void test_rom_constants() {
    static_assert(CombatWeaponDurabilityConfig::kWeaponDurabilityDecrementAddr == 0x008530U);
    static_assert(CombatWeaponDurabilityConfig::kThresholdResetGuardAddr == 0x0085C4U);
    static_assert(CombatWeaponDurabilityConfig::kDpadVelocityResolverAddr == 0x0085E2U);
    static_assert(CombatWeaponDurabilityConfig::kDpadJumpTableAddr == 0x0085FAU);

    static_assert(CombatWeaponDurabilityConfig::kEquippedSlotRam == 0x00FF0DB0U);
    static_assert(CombatWeaponDurabilityConfig::kEquippedWeaponIdRam == 0x00FF0DB1U);
    static_assert(CombatWeaponDurabilityConfig::kWeaponInventoryBaseRam == 0x00FF0D80U);
    static_assert(CombatWeaponDurabilityConfig::kBreakSfxId == 0x00C00269U);

    static_assert(CombatWeaponDurabilityConfig::kMinConsumableWeaponId == 2U);
    static_assert(CombatWeaponDurabilityConfig::kMaxConsumableWeaponId == 12U);
    static_assert(CombatWeaponDurabilityConfig::kInfiniteDurability == 0xFFU);
    static_assert(CombatWeaponDurabilityConfig::kDefaultDaggerId == 1U);
    static_assert(CombatWeaponDurabilityConfig::kRevertTimerDuration == 0x000AU);
    static_assert(CombatWeaponDurabilityConfig::kHudToastTimerDuration == 0x005AU);
}

static void test_durability_direct_vectors() {
    // Exact vectors from tests/rom_weapon_durability_dpad_test.py
    {
        const auto [broke, id, dur] = step_weapon_durability_direct(1U, 5U);
        assert(!broke && id == 1U && dur == 5U);
    }
    {
        const auto [broke, id, dur] = step_weapon_durability_direct(5U, 0xFFU);
        assert(!broke && id == 5U && dur == 0xFFU);
    }
    {
        const auto [broke, id, dur] = step_weapon_durability_direct(5U, 3U);
        assert(!broke && id == 5U && dur == 2U);
    }
    {
        const auto [broke, id, dur] = step_weapon_durability_direct(5U, 1U);
        assert(broke && id == 0U && dur == 0U);
    }
    {
        const auto [broke, id, dur] = step_weapon_durability_direct(12U, 0U);
        assert(broke && id == 0U && dur == 0U);
    }
    // Out of bounds weapon id
    {
        const auto [broke, id, dur] = step_weapon_durability_direct(13U, 10U);
        assert(!broke && id == 13U && dur == 10U);
    }
}

static void test_inventory_state_step_and_revert() {
    InventoryState inv{};
    // Setup slot 1 with Sword (ID 3, durability 2)
    inv.slots[1] = WeaponSlot{.id = 3U, .durability = 2U};
    inv.equipped_slot = 1U;
    inv.equipped_id = 3U;

    // First step: durability decrements from 2 to 1
    auto res1 = step_weapon_durability(inv);
    assert(res1.decremented);
    assert(!res1.broke);
    assert(res1.prev_durability == 2U);
    assert(res1.new_durability == 1U);
    assert(inv.slots[1].id == 3U);
    assert(inv.slots[1].durability == 1U);
    assert(inv.equipped_slot == 1U);
    assert(inv.equipped_id == 3U);

    // Second step: durability reaches 0 -> breaks and reverts to Dagger
    auto res2 = step_weapon_durability(inv);
    assert(!res2.decremented);
    assert(res2.broke);
    assert(res2.prev_durability == 1U);
    assert(res2.new_durability == 0U);
    assert(res2.sfx_id == CombatWeaponDurabilityConfig::kBreakSfxId);

    // Slot 1 is cleared
    assert(inv.slots[1].id == 0U);
    assert(inv.slots[1].durability == 0U);

    // State reverted to Dagger in slot 0
    assert(inv.equipped_slot == 0U);
    assert(inv.equipped_id == CombatWeaponDurabilityConfig::kDefaultDaggerId);
    assert(inv.revert_timer == CombatWeaponDurabilityConfig::kRevertTimerDuration);
    assert(inv.sub_state == 0U);
    assert(inv.hud_weapon_id == CombatWeaponDurabilityConfig::kDefaultDaggerId);
    assert(inv.hud_weapon_state == 0U);
    assert(inv.hud_toast_timer == CombatWeaponDurabilityConfig::kHudToastTimerDuration);
    assert(inv.break_flag == 0xFFU);
}

static void test_threshold_guard() {
    std::uint16_t value = 50U;
    const std::uint16_t limit = 100U;
    std::uint8_t flag = 0U;

    // Below limit
    assert(!check_threshold_guard(value, limit, flag));
    assert(value == 50U);
    assert(flag == 0U);

    // Equal to limit (unsigned bls branch: no action)
    value = 100U;
    assert(!check_threshold_guard(value, limit, flag));
    assert(value == 100U);
    assert(flag == 0U);

    // Exceeds limit -> reset value to 0, trip flag to 0xFF
    value = 101U;
    assert(check_threshold_guard(value, limit, flag));
    assert(value == 0U);
    assert(flag == 0xFFU);
}

static void test_dpad_16_way_resolver() {
    constexpr auto kCardX = CombatWeaponDurabilityConfig::kSpeedCardinalX;
    constexpr auto kCardY = CombatWeaponDurabilityConfig::kSpeedCardinalY;
    constexpr auto kDiagX = CombatWeaponDurabilityConfig::kSpeedDiagonalX;
    constexpr auto kDiagY = CombatWeaponDurabilityConfig::kSpeedDiagonalY;

    // 0x0: Neutral (facing preserved)
    {
        auto r = resolve_dpad_motion(0x0U, 1U);
        assert(!r.moving && r.facing == 1U && r.vx == 0 && r.vy == 0);
    }
    // 0x1: Up (0x008654)
    {
        auto r = resolve_dpad_motion(0x1U, 1U);
        assert(r.moving && r.facing == 0U && r.vx == 0 && r.vy == -kCardY);
    }
    // 0x2: Down (0x008644)
    {
        auto r = resolve_dpad_motion(0x2U, 1U);
        assert(r.moving && r.facing == 2U && r.vx == 0 && r.vy == kCardY);
    }
    // 0x3: Up + Down (cancel -> neutral)
    {
        auto r = resolve_dpad_motion(0x3U, 3U);
        assert(!r.moving && r.facing == 3U && r.vx == 0 && r.vy == 0);
    }
    // 0x4: Left (0x008634)
    {
        auto r = resolve_dpad_motion(0x4U, 0U);
        assert(r.moving && r.facing == 3U && r.vx == -kCardX && r.vy == 0);
    }
    // 0x5: Up-Left (0x0086A0)
    {
        auto r = resolve_dpad_motion(0x5U, 1U);
        assert(r.moving && r.facing == 0U && r.vx == -kDiagX && r.vy == -kDiagY);
    }
    // 0x6: Down-Left (0x00868C)
    {
        auto r = resolve_dpad_motion(0x6U, 1U);
        assert(r.moving && r.facing == 2U && r.vx == -kDiagX && r.vy == kDiagY);
    }
    // 0x7: Left + Up + Down (Up/Down cancel -> Left)
    {
        auto r = resolve_dpad_motion(0x7U, 0U);
        assert(r.moving && r.facing == 3U && r.vx == -kCardX && r.vy == 0);
    }
    // 0x8: Right (0x008624)
    {
        auto r = resolve_dpad_motion(0x8U, 0U);
        assert(r.moving && r.facing == 1U && r.vx == kCardX && r.vy == 0);
    }
    // 0x9: Up-Right (0x008678)
    {
        auto r = resolve_dpad_motion(0x9U, 2U);
        assert(r.moving && r.facing == 0U && r.vx == kDiagX && r.vy == -kDiagY);
    }
    // 0xA: Down-Right (0x008664)
    {
        auto r = resolve_dpad_motion(0xAU, 0U);
        assert(r.moving && r.facing == 2U && r.vx == kDiagX && r.vy == kDiagY);
    }
    // 0xB: Right + Up + Down (Up/Down cancel -> Right)
    {
        auto r = resolve_dpad_motion(0xBU, 0U);
        assert(r.moving && r.facing == 1U && r.vx == kCardX && r.vy == 0);
    }
    // 0xC: Left + Right (cancel -> neutral)
    {
        auto r = resolve_dpad_motion(0xCU, 2U);
        assert(!r.moving && r.facing == 2U && r.vx == 0 && r.vy == 0);
    }
    // 0xD: Up + Left + Right (Left/Right cancel -> Up)
    {
        auto r = resolve_dpad_motion(0xDU, 2U);
        assert(r.moving && r.facing == 0U && r.vx == 0 && r.vy == -kCardY);
    }
    // 0xE: Down + Left + Right (Left/Right cancel -> Down)
    {
        auto r = resolve_dpad_motion(0xEU, 0U);
        assert(r.moving && r.facing == 2U && r.vx == 0 && r.vy == kCardY);
    }
    // 0xF: Up + Down + Left + Right (all cancel -> neutral)
    {
        auto r = resolve_dpad_motion(0xFU, 1U);
        assert(!r.moving && r.facing == 1U && r.vx == 0 && r.vy == 0);
    }
}

int main() {
    std::cout << "[oasis_m16_140_weapon_durability_dpad_test] Starting..." << std::endl;
    test_rom_constants();
    test_durability_direct_vectors();
    test_inventory_state_step_and_revert();
    test_threshold_guard();
    test_dpad_16_way_resolver();
    std::cout << "[oasis_m16_140_weapon_durability_dpad_test] All tests passed!" << std::endl;
    return 0;
}
