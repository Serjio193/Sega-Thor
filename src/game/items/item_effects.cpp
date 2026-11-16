#include "game/items/item_effects.hpp"
#include "game/items/item_attribute_tables.hpp"

#include <algorithm>

namespace oasis::game::items {

namespace {

[[nodiscard]] std::int16_t calc_pct(std::int16_t max_val, std::uint8_t pct) noexcept {
    if (max_val <= 0) return 0;
    if (pct == 25U) return static_cast<std::int16_t>(max_val >> 2U);
    if (pct == 50U) return static_cast<std::int16_t>(max_val >> 1U);
    if (pct == 75U) return static_cast<std::int16_t>((max_val >> 1U) + (max_val >> 2U));
    return max_val;
}

} // namespace

bool can_use_item(std::uint8_t item_id, const ItemUseContext& ctx) noexcept {
    // ROM 0x005236: area 0x10 blocks all item usage
    if (ctx.area_id == ItemConfig::kBlockedAreaId ||
        (ctx.area_id >> 8U) == ItemConfig::kBlockedAreaId) {
        return false;
    }

    // 0x1A..0x1D: Spirit Summon Rings
    if (item_id >= 0x1AU && item_id <= 0x1DU) {
        if (ctx.summon_locked || ctx.spirit_active || ctx.sp <= 0) {
            return false;
        }
        const std::uint8_t spirit_bit = static_cast<std::uint8_t>(item_id - 0x1AU);
        return (ctx.unlocked_spirit_mask & static_cast<std::uint8_t>(1U << spirit_bit)) != 0U;
    }

    // 0x1E..0x21: HP Food
    if (item_id >= 0x1EU && item_id <= 0x21U) {
        return ctx.hp < ctx.max_hp;
    }

    // 0x22..0x25: SP Potions
    if (item_id >= 0x22U && item_id <= 0x25U) {
        return ctx.sp < ctx.max_sp;
    }

    // 0x26..0x2A: Dual Elixirs
    if (item_id >= 0x26U && item_id <= 0x2AU) {
        return (ctx.hp < ctx.max_hp) || (ctx.sp < ctx.max_sp);
    }

    return false;
}

ItemEffectResult apply_item_effect(std::uint8_t item_id, ItemUseContext& ctx) noexcept {
    ItemEffectResult res{};

    // 0x1A..0x1D: Spirit Summon Rings (0x005378..0x005384)
    if (item_id >= 0x1AU && item_id <= 0x1DU) {
        res.applied = true;
        res.summon_requested = true;
        res.summon_spirit_index = static_cast<std::uint8_t>(item_id - 0x1AU);
        res.summon_timer = ItemConfig::kSummonTimer;
        res.sp_delta = static_cast<std::int16_t>(ctx.max_sp - ctx.sp);
        ctx.sp = ctx.max_sp;
        ctx.spirit_active = true;
        return res;
    }

    // 0x1E..0x21: HP Food (25/50/75/100% MaxHP)
    if (item_id >= 0x1EU && item_id <= 0x21U) {
        static constexpr std::uint8_t kPct[4] = {25U, 50U, 75U, 100U};
        const std::int16_t add = calc_pct(ctx.max_hp, kPct[item_id - 0x1EU]);
        const std::int16_t next_hp = static_cast<std::int16_t>(std::min<int>(ctx.max_hp, ctx.hp + add));
        res.hp_delta = static_cast<std::int16_t>(next_hp - ctx.hp);
        ctx.hp = next_hp;
        res.sound_cmd = ItemConfig::kSfxHeal;
        res.applied = true;
        return res;
    }

    // 0x22..0x25: SP Potions (25/50/75/100% MaxSP)
    if (item_id >= 0x22U && item_id <= 0x25U) {
        static constexpr std::uint8_t kPct[4] = {25U, 50U, 75U, 100U};
        const std::int16_t add = calc_pct(ctx.max_sp, kPct[item_id - 0x22U]);
        const std::int16_t next_sp = static_cast<std::int16_t>(std::min<int>(ctx.max_sp, ctx.sp + add));
        res.sp_delta = static_cast<std::int16_t>(next_sp - ctx.sp);
        ctx.sp = next_sp;
        res.sound_cmd = ItemConfig::kSfxHeal;
        res.applied = true;
        return res;
    }

    // 0x26..0x2A: Dual Elixirs (25/50/75/100/100% both)
    if (item_id >= 0x26U && item_id <= 0x2AU) {
        static constexpr std::uint8_t kPct[5] = {25U, 50U, 75U, 100U, 100U};
        const std::uint8_t pct = kPct[item_id - 0x26U];
        const std::int16_t next_hp = static_cast<std::int16_t>(std::min<int>(ctx.max_hp, ctx.hp + calc_pct(ctx.max_hp, pct)));
        const std::int16_t next_sp = static_cast<std::int16_t>(std::min<int>(ctx.max_sp, ctx.sp + calc_pct(ctx.max_sp, pct)));
        res.hp_delta = static_cast<std::int16_t>(next_hp - ctx.hp);
        res.sp_delta = static_cast<std::int16_t>(next_sp - ctx.sp);
        ctx.hp = next_hp;
        ctx.sp = next_sp;
        res.sound_cmd = ItemConfig::kSfxHeal;
        res.applied = true;
        return res;
    }

    return res;
}

ItemEffectResult use_inventory_slot(
    save::PersistentWramBlock& wram,
    std::size_t slot_index,
    ItemUseContext& ctx,
    spirits::ActiveSpiritController& spirit_ctrl,
    std::int32_t spawn_x,
    std::int32_t spawn_y,
    std::span<const std::uint8_t> rom,
    genesis::VdpState* vdp) noexcept {
    const auto res = use_inventory_slot(wram, slot_index, ctx, nullptr, spawn_x, spawn_y);
    if (res.applied && res.summon_requested) {
        spirit_ctrl.summon_spirit_by_index(res.summon_spirit_index, spawn_x, spawn_y, 0, rom, vdp);
    }
    return res;
}

ItemEffectResult use_inventory_slot(
    save::PersistentWramBlock& wram,
    std::size_t slot_index,
    ItemUseContext& ctx,
    spirits::DyttoSpiritController* dytto,
    std::int32_t spawn_x,
    std::int32_t spawn_y) noexcept {
    const std::uint8_t item_id = wram.item_id(slot_index);
    const std::uint8_t count = wram.item_count(slot_index);
    if (item_id == 0U || count == 0U || !can_use_item(item_id, ctx)) {
        return ItemEffectResult{};
    }

    ItemEffectResult res = apply_item_effect(item_id, ctx);
    if (!res.applied) {
        return res;
    }

    // Consumables 0x1E..0x2A decrement stack count; spirit rings 0x1A..0x1D are reusable.
    if (item_id >= 0x1EU && item_id <= 0x2AU) {
        const std::uint8_t rem = static_cast<std::uint8_t>(count - 1U);
        wram.set_item_slot(slot_index, rem > 0U ? item_id : 0U, rem);
        res.consumed = true;
    }

    if (res.summon_requested && res.summon_spirit_index == 0U && dytto != nullptr) {
        dytto->init_spawn(spawn_x, spawn_y, 0, 2U, false);
    }

    return res;
}

bool discard_inventory_slot(
    save::PersistentWramBlock& wram,
    std::size_t slot_index,
    std::span<const std::uint8_t> rom) noexcept {
    const std::uint8_t item_id = wram.item_id(slot_index);
    const std::uint8_t count = wram.item_count(slot_index);
    if (item_id == 0U || count == 0U || is_item_non_discardable(rom, item_id)) {
        return false;
    }
    wram.set_item_slot(slot_index, 0U, 0U);
    return true;
}

} // namespace oasis::game::items
