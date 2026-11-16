#include "game/npc_dialogue_handler.hpp"
#include "game/spirits/spirit_awakening.hpp"

#include <cmath>
#include <cstdlib>

namespace oasis::game {

bool update_npc_dialogue_state(
    player::PlayerState& player,
    const core::ControllerState& controller,
    event::EventVm& event_vm,
    render::DialogueWindow& dialogue_window,
    std::span<const NativeEntity> entities,
    std::span<const std::uint8_t> rom_bytes,
    std::uint16_t room_id,
    spirits::DyttoSpiritController* dytto_controller) noexcept {
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
                } else if (const auto shrine = spirits::dispatch_spirit_awakening(room_id);
                           shrine.has_value() && current_msg_id == shrine->message_id) {
                    // Spirit Awakening Dispatcher (0x018CAA):
                    // For Room 0x0106 (0x018CDE, Dytto Water Shrine):
                    // - Sets Area Flag $FF0E1D:2 (0x018D30, bank 2 idx 10)
                    // - Sets Global Flag 3 $FF0DBC:3 (0x018D38, Dytto acquired)
                    // - Sets Active Spirit $FF0D7A:0 (0x018D40, Dytto active)
                    // - Spawns Dytto (0x0018) in State 0x1A via 0x014668
                    if (shrine->area_flag_idx.has_value()) {
                        event_vm.set_event_flag(*shrine->area_flag_idx, true, 2U);
                    }
                    event_vm.set_event_flag(shrine->global_flag_idx, true, 1U);
                    if (shrine->active_spirit_bit.has_value()) {
                        event_vm.set_active_spirit_bit(*shrine->active_spirit_bit, true);
                    }
                    if (dytto_controller != nullptr && room_id == 0x0106U) {
                        std::int32_t ax = static_cast<std::int32_t>(player.x_fixed / 0x10000);
                        std::int32_t ay = static_cast<std::int32_t>(player.y_fixed / 0x10000);
                        std::int32_t az = static_cast<std::int32_t>(player.z_fixed / 0x10000);
                        for (const auto& ent : entities) {
                            if (ent.active && ent.type_id == 0x0020U) {
                                ax = ent.world_x;
                                ay = ent.world_y;
                                az = ent.world_z;
                                break;
                            }
                        }
                        dytto_controller->init_spawn(ax, ay, az, 2U, true);
                    }
                }
            }
        }
        return true;
    }

    const auto px = static_cast<std::int32_t>(player.x_fixed / 0x10000);
    const auto py = static_cast<std::int32_t>(player.y_fixed / 0x10000);
    for (const auto& ent : entities) {
        // Entity type 0x003C (Village NPCs / Elder / Villagers) and 0x0020 (Shrine Altar / Prop)
        if (ent.active && (ent.type_id == 0x003CU || ent.type_id == 0x0020U)) {
            // Collision / proximity condition: dx <= 28 && dy <= 28 (0x018BBC / 0x018C28 / 0x008330)
            const int dx = std::abs(px - ent.world_x);
            const int dy = std::abs(py - ent.world_y);
            if (dx <= 28 && dy <= 28 && (player.direction_code == 0 || py > ent.world_y)) {
                const bool btn_talk = controller.pressed(core::Button::A) ||
                                      controller.pressed(core::Button::B) ||
                                      controller.pressed(core::Button::C);
                if (btn_talk) {
                    std::uint16_t msg_id = (ent.message_id != 0U) ? ent.message_id : 5U;
                    if (ent.type_id == 0x0020U) {
                        const auto shrine = spirits::dispatch_spirit_awakening(room_id);
                        if (!shrine.has_value()) {
                            return false;
                        }
                        msg_id = shrine->message_id;
                    }
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
