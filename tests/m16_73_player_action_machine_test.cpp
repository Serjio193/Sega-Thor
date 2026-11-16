#include "game/player/player_action_machine.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

void write_be16(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t v) {
    buf[off] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>(v & 0xFFU);
}

} // namespace

int main() {
    using oasis::game::player::lookup_player_state_handler_addr;
    using oasis::game::player::PlayerActionMachineConfig;
    using oasis::game::player::PlayerActionState;
    using oasis::game::player::PlayerPreTickState;

    // 1. 59-state table lookup (0x0059BA..0x005A2F).
    std::vector<std::uint8_t> rom(0x008000U, 0x00U);
    write_be16(rom, PlayerActionMachineConfig::kStateTableAddr + 0x00U, 2108U); // -> 0x0061F6
    write_be16(rom, PlayerActionMachineConfig::kStateTableAddr + 0x02U, 2344U); // -> 0x0062E4
    write_be16(rom, PlayerActionMachineConfig::kStateTableAddr + 0x74U, 100U);  // -> 0x005A2E + 100 = 0x005A92

    assert(lookup_player_state_handler_addr(rom, 0x00U) == 0x000061F6U);
    assert(lookup_player_state_handler_addr(rom, 0x02U) == 0x000062E4U);
    assert(lookup_player_state_handler_addr(rom, 0x74U) == 0x00005A92U);
    assert(!lookup_player_state_handler_addr(rom, 0x01U).has_value());
    assert(!lookup_player_state_handler_addr(rom, 0x76U).has_value());

    // 2. Low HP (0xA0038C, period (hp>>1)+10) & Low SP (0xA004DD, period sp+12).
    PlayerPreTickState p;
    p.hp = 20U;
    p.sp = 10U;
    p.active_spirit = 0x0018U;
    p.step_pre_dispatch_tick();
    assert(p.sound_commands.size() == 2U);
    assert(p.sound_commands[0] == 0x00A0038CU);
    assert(p.low_hp_timer == 20U); // (20 >> 1) + 10
    assert(p.sound_commands[1] == 0x00A004DDU);
    assert(p.low_sp_timer == 22U); // 10 + 12

    // 3. Safe respawn coord capture when state in {0x00, 0x02, 0x04} && !airborne.
    p.x = 320;
    p.y = 480;
    p.state = 0x02U;
    p.airborne = false;
    p.step_pre_dispatch_tick();
    assert(p.safe_respawn_x == 320 && p.safe_respawn_y == 480);

    p.x = 400;
    p.airborne = true;
    p.step_pre_dispatch_tick();
    assert(p.safe_respawn_x == 320); // Not overwritten while airborne

    // 4. Hazard damage (16 or 8 frames) and Passive SP regen (8 or 4 frames).
    PlayerPreTickState env;
    env.hp = 100U;
    env.max_hp = 100U;
    env.sp = 50U;
    env.max_sp = 100U;
    env.hazard_ring = true;
    env.tile_hazard_or_water = true;
    env.regen_speed_boost = true; // 8-frame hazard, 4-frame SP regen
    for (int f = 0; f < 8; ++f) {
        env.step_pre_dispatch_tick();
    }
    assert(env.hp == 99U);
    assert(env.sp == 52U);

    // 5. Death (0x5E) -> Auto-Revive (0x6C, +3 HP even / +3 SP odd) vs Game Over (0x60, 40 frames).
    PlayerPreTickState rev;
    rev.state = static_cast<std::uint16_t>(PlayerActionState::Death);
    rev.hp = 0U; rev.max_hp = 6U;
    rev.sp = 0U; rev.max_sp = 6U;
    rev.death_anim_finished = true;
    rev.auto_revive_slot_present = true;
    rev.step_death_and_revive();
    assert(rev.state == 0x006CU);
    assert(rev.active_item_id == 0x002AU);
    assert(!rev.auto_revive_slot_present);
    assert(rev.sound_commands.back() == 0x00D00355U);

    // Frame 0 (even): +3 HP; Frame 1 (odd): +3 SP; Frame 2: +3 HP; Frame 3: +3 SP -> full -> Idle
    rev.step_death_and_revive(); assert(rev.hp == 3U && rev.sp == 0U);
    rev.step_death_and_revive(); assert(rev.hp == 3U && rev.sp == 3U);
    rev.step_death_and_revive(); assert(rev.hp == 6U && rev.sp == 3U);
    rev.step_death_and_revive(); assert(rev.hp == 6U && rev.sp == 6U);
    assert(rev.state == 0x0000U);

    PlayerPreTickState go;
    go.state = static_cast<std::uint16_t>(PlayerActionState::Death);
    go.death_anim_finished = true;
    go.auto_revive_slot_present = false;
    go.step_death_and_revive();
    assert(go.state == 0x0060U);
    assert(go.game_over_fade_timer == 40U);

    return 0;
}
