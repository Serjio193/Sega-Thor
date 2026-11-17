#include "game/entities/entity_pool_allocators.hpp"

#include <cassert>

namespace {

using namespace oasis::game::entities;

void test_pool_capacities_and_addresses() {
    EntityPoolAllocators pools;

    // Effect8: 8 slots, $FF2ABC, stride 0x5A, VRAM tile 0x0694 + idx * 9
    for (std::size_t i = 0; i < 8U; ++i) {
        const auto slot = pools.allocate(ObjectPoolKind::Effect8, 0x10U);
        assert(slot.has_value());
        assert(slot->index == i);
        assert(slot->ram_addr == 0xFF2ABCU + i * 0x5AU);
        assert(slot->vram_tile == 0x0694U + i * 9U);
    }
    assert(!pools.allocate(ObjectPoolKind::Effect8, 0x10U).has_value());
    pools.free_slot(ObjectPoolKind::Effect8, 3U);
    const auto reused_eff = pools.allocate(ObjectPoolKind::Effect8, 0x12U);
    assert(reused_eff.has_value() && reused_eff->index == 3U && reused_eff->vram_tile == 0x0694U + 27U);

    // Special6: 6 slots, $FF2D8C, stride 0x5A
    for (std::size_t i = 0; i < 6U; ++i) {
        const auto slot = pools.allocate(ObjectPoolKind::Special6, 0x20U);
        assert(slot.has_value());
        assert(slot->index == i);
        assert(slot->ram_addr == 0xFF2D8CU + i * 0x5AU);
    }
    assert(!pools.allocate(ObjectPoolKind::Special6, 0x20U).has_value());

    // FullEntity18: 18 slots, $FF1C1C, stride 0xBC
    for (std::size_t i = 0; i < 18U; ++i) {
        const auto slot = pools.allocate(ObjectPoolKind::FullEntity18, 0x30U);
        assert(slot.has_value());
        assert(slot->index == i);
        assert(slot->ram_addr == 0xFF1C1CU + i * 0xBCU);
    }
    assert(!pools.allocate(ObjectPoolKind::FullEntity18, 0x30U).has_value());

    // EnemyEntity17: 17 slots, $FF1CD8, stride 0xBC
    for (std::size_t i = 0; i < 17U; ++i) {
        const auto slot = pools.allocate(ObjectPoolKind::EnemyEntity17, 0x40U);
        assert(slot.has_value());
        assert(slot->index == i);
        assert(slot->ram_addr == 0xFF1CD8U + i * 0xBCU);
    }
    assert(!pools.allocate(ObjectPoolKind::EnemyEntity17, 0x40U).has_value());
}

void test_enemy_hud_invalidation_on_reuse() {
    EntityPoolAllocators pools;
    const auto s0 = pools.allocate(ObjectPoolKind::EnemyEntity17, 0x42U);
    const auto s1 = pools.allocate(ObjectPoolKind::EnemyEntity17, 0x44U);
    assert(s0.has_value() && s1.has_value());

    // Set HUD target to s1, timer = 90
    pools.enemy_hud_target = s1->ram_addr;
    pools.enemy_hud_timer = 90U;

    // Reusing s0 must NOT clear enemy_hud_timer
    pools.free_slot(ObjectPoolKind::EnemyEntity17, 0U);
    const auto r0 = pools.allocate(ObjectPoolKind::EnemyEntity17, 0x46U);
    assert(r0.has_value() && r0->index == 0U);
    assert(pools.enemy_hud_timer == 90U);

    // Reusing s1 MUST clear enemy_hud_timer ($FF184F)
    pools.free_slot(ObjectPoolKind::EnemyEntity17, 1U);
    const auto r1 = pools.allocate(ObjectPoolKind::EnemyEntity17, 0x48U);
    assert(r1.has_value() && r1->index == 1U);
    assert(pools.enemy_hud_timer == 0U);
}

} // namespace

int main() {
    test_pool_capacities_and_addresses();
    test_enemy_hud_invalidation_on_reuse();
    return 0;
}
