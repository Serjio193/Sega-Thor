#pragma once

#include "game/entity/native_entity.hpp"
#include "game/event/event_vm.hpp"
#include "game/player/player.hpp"
#include "game/render/dialogue_window.hpp"
#include "game/spirits/dytto_controller.hpp"
#include "core/runtime.hpp"

#include <span>

namespace oasis::game {

// Helper to handle NPC (0x003C) and Spirit Shrine Altar (0x0020 -> 0x018CAA) interaction and dialogue.
bool update_npc_dialogue_state(
    player::PlayerState& player,
    const core::ControllerState& controller,
    event::EventVm& event_vm,
    render::DialogueWindow& dialogue_window,
    std::span<const NativeEntity> entities,
    std::span<const std::uint8_t> rom_bytes,
    std::uint16_t room_id = 0x0106U,
    spirits::DyttoSpiritController* dytto_controller = nullptr) noexcept;

} // namespace oasis::game
