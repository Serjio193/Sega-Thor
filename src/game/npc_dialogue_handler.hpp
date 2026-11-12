#pragma once

#include "game/entity/native_entity.hpp"
#include "game/event/event_vm.hpp"
#include "game/player/player.hpp"
#include "game/render/dialogue_window.hpp"
#include "core/runtime.hpp"

#include <span>

namespace oasis::game {

// Helper to handle Screen(1,8) NPC Village Elder interaction and dialogue
bool update_npc_dialogue_state(
    player::PlayerState& player,
    const core::ControllerState& controller,
    event::EventVm& event_vm,
    render::DialogueWindow& dialogue_window,
    std::span<const NativeEntity> entities,
    std::span<const std::uint8_t> rom_bytes) noexcept;

} // namespace oasis::game
