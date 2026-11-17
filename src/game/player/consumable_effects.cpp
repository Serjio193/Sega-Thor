#include "game/player/consumable_effects.hpp"

namespace oasis::game::player {

bool can_use_consumable(
    std::uint8_t item_id,
    const ConsumableContext& ctx) noexcept {
    return items::can_use_item(item_id, ctx);
}

ConsumableResult apply_consumable_effect(
    std::uint8_t item_id,
    ConsumableContext& ctx) noexcept {
    return items::apply_item_effect(item_id, ctx);
}

} // namespace oasis::game::player
