#include "game/npc_dialogue_handler.hpp"

#include <cmath>
#include <cstdlib>

namespace oasis::game {

bool update_npc_dialogue_state(
    player::PlayerState& player,
    const core::ControllerState& controller,
    event::EventVm& event_vm,
    render::DialogueWindow& dialogue_window,
    std::span<const NativeEntity> entities,
    std::span<const std::uint8_t> rom_bytes) noexcept {
    const bool in_dialogue = dialogue_window.is_open() || event_vm.is_waiting_dialogue();
    if (in_dialogue) {
        player.movement_state = 0x36U; // Dialogue freeze / talk state (0x018C72)
        const bool btn_press = controller.pressed(core::Button::A) ||
                               controller.pressed(core::Button::B) ||
                               controller.pressed(core::Button::C);
        event_vm.step_frame(btn_press);
        if (btn_press) {
            const auto current_msg_id = dialogue_window.message_id();
            if (!dialogue_window.advance_page()) {
                // Dialogue finished: restore player movement and record dialogue completion flag
                player.movement_state = 0U;
                if (current_msg_id == 5U) {
                    // Village Elder completion sets Event Flag 141 (0x8D)
                    event_vm.set_event_flag(0x008DU, true);
                } else if (current_msg_id == 4U) {
                    // Pier Villager interaction flag
                    event_vm.set_event_flag(0x008EU, true);
                }
            }
        }
        return true;
    }

    const auto px = static_cast<std::int32_t>(player.x_fixed / 0x10000);
    const auto py = static_cast<std::int32_t>(player.y_fixed / 0x10000);
    for (const auto& ent : entities) {
        // Entity type 0x003C (Village NPCs / Elder / Villagers)
        if (ent.active && ent.type_id == 0x003CU) {
            // Collision / proximity condition: dx <= 28 && dy <= 28 (0x018BBC / 0x018C28)
            const int dx = std::abs(px - ent.world_x);
            const int dy = std::abs(py - ent.world_y);
            if (dx <= 28 && dy <= 28 && (player.direction_code == 0 || py > ent.world_y)) {
                const bool btn_talk = controller.pressed(core::Button::A) ||
                                      controller.pressed(core::Button::B) ||
                                      controller.pressed(core::Button::C);
                if (btn_talk) {
                    // Resolve message ID from entity state (fallback to 5 for Village Elder)
                    const std::uint16_t msg_id = (ent.message_id != 0U) ? ent.message_id : 5U;
                    const auto seq = event::decode_dialogue_by_id(rom_bytes, msg_id);
                    dialogue_window.open(seq);
                    event_vm.trigger_dialogue(seq);
                    player.movement_state = 0x36U; // Lock player in talk state
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace oasis::game
