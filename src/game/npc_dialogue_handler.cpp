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
                } else if (current_msg_id == 0U) {
                    // Princess / Water Spirit interaction in Screen(1,6) (0x018CDE):
                    // Sets Global Flag 3 ($FF0DBC:3) at 0x018D38!
                    // Also sets Area Flag $FF0E1D:2 (0x018D30) and Global Flag $FF0D7A:0 (0x018D40).
                    event_vm.set_event_flag(3U, true, 1U);
                }
            }
        }
        return true;
    }

    const auto px = static_cast<std::int32_t>(player.x_fixed / 0x10000);
    const auto py = static_cast<std::int32_t>(player.y_fixed / 0x10000);
    for (const auto& ent : entities) {
        // Entity type 0x003C (Village NPCs / Elder / Villagers) and 0x0020 (Princess / Water Spirit)
        if (ent.active && (ent.type_id == 0x003CU || ent.type_id == 0x0020U)) {
            // Collision / proximity condition: dx <= 28 && dy <= 28 (0x018BBC / 0x018C28 / 0x008330)
            const int dx = std::abs(px - ent.world_x);
            const int dy = std::abs(py - ent.world_y);
            if (dx <= 28 && dy <= 28 && (player.direction_code == 0 || py > ent.world_y)) {
                const bool btn_talk = controller.pressed(core::Button::A) ||
                                      controller.pressed(core::Button::B) ||
                                      controller.pressed(core::Button::C);
                if (btn_talk) {
                    // Resolve message ID from entity state:
                    // For ent 0x0020 (Princess in Screen 1,6), message ID is 0 (0x018CDE)
                    // For ent 0x003C, fallback to 5 for Village Elder
                    const std::uint16_t msg_id = (ent.type_id == 0x0020U) ? 0U : ((ent.message_id != 0U) ? ent.message_id : 5U);
                    const auto seq = event::decode_dialogue_by_id(rom_bytes, msg_id);
                    dialogue_window.open(seq);
                    event_vm.trigger_dialogue(seq);
                    player.movement_state = 0x36U; // Lock player in talk state (0x018C72 / 0x018CDE)
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace oasis::game
