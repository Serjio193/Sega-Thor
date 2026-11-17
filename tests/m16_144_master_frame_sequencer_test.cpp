#include "game/player/master_frame_sequencer.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    assert(MasterFrameSequencerConfig::kMasterFrameSequencerAddr == 0x008AA6U);
    assert(MasterFrameSequencerConfig::kMasterFrameSequencerEndAddr == 0x008B8CU);
    assert(MasterFrameSequencerConfig::kScreenShakeDispatcherAddr == 0x008B8EU);
    assert(MasterFrameSequencerConfig::kScreenShakeDispatcherEndAddr == 0x008BAAU);
    assert(MasterFrameSequencerConfig::kJoypadMenuHotkeysAddr == 0x008BACU);
    assert(MasterFrameSequencerConfig::kJoypadMenuHotkeysEndAddr == 0x008C58U);

    assert(MasterFrameSequencerConfig::kPaletteFlashHandlerAddr == 0x00530CU);
    assert(MasterFrameSequencerConfig::kEarthquakeShakeHandlerAddr == 0x007CC6U);
    assert(MasterFrameSequencerConfig::kMenuHandlerAddr == 0x003B3EU);
    assert(MasterFrameSequencerConfig::kTitleSequenceAddr == 0x002934U);

    assert(MasterFrameSequencerConfig::kPauseMenuAddr == 0x00A4C8U);
    assert(MasterFrameSequencerConfig::kJoypadPollAddr == 0x008BACU);
    assert(MasterFrameSequencerConfig::kScreenShakeStepAddr == 0x008B8EU);
    assert(MasterFrameSequencerConfig::kEventVmTickAddr == 0x00F788U);
    assert(MasterFrameSequencerConfig::kDialogueHudStepAddr == 0x00557AU);
    assert(MasterFrameSequencerConfig::kSpiritManaStepAddr == 0x00DEECU);
    assert(MasterFrameSequencerConfig::kSecondaryWeaponFlightAddr == 0x00E0BAU);
    assert(MasterFrameSequencerConfig::kHazardBlastStepAddr == 0x00E0F4U);
    assert(MasterFrameSequencerConfig::kAliMachineStepAddr == 0x008E22U);
    assert(MasterFrameSequencerConfig::kEntitiesListStepAddr == 0x008E90U);
    assert(MasterFrameSequencerConfig::kEntityGcReclaimAddr == 0x008DD4U);
    assert(MasterFrameSequencerConfig::kCameraBoundaryClampAddr == 0x00C998U);
    assert(MasterFrameSequencerConfig::kPlaneScrollSyncAddr == 0x00C980U);
    assert(MasterFrameSequencerConfig::kSatComposeAddr == 0x00A196U);
}

static void test_screen_shake_dispatcher() {
    // 0: None
    std::uint8_t trig0 = 0U;
    auto res0 = dispatch_screen_shake(trig0);
    assert(res0.action == ScreenShakeAction::None);
    assert(res0.remaining_trigger == 0U);
    assert(trig0 == 0U);

    // Bit 7 clear: PaletteFlash
    std::uint8_t trig1 = 0x04U;
    auto res1 = dispatch_screen_shake(trig1);
    assert(res1.action == ScreenShakeAction::PaletteFlash);
    assert(res1.remaining_trigger == 0x04U);
    assert(trig1 == 0x04U);

    // Bit 7 set: ScreenShake
    std::uint8_t trig2 = 0x84U;
    auto res2 = dispatch_screen_shake(trig2);
    assert(res2.action == ScreenShakeAction::ScreenShake);
    assert(res2.remaining_trigger == 0x04U);
    assert(trig2 == 0x04U);

    // Bit 7 only: ScreenShake with remaining 0
    std::uint8_t trig3 = 0x80U;
    auto res3 = dispatch_screen_shake(trig3);
    assert(res3.action == ScreenShakeAction::ScreenShake);
    assert(res3.remaining_trigger == 0x00U);
    assert(trig3 == 0x00U);
}

static void test_joypad_menu_hotkeys() {
    // 1. Release mask check
    std::uint8_t menu_req = 0xFFU;
    std::uint8_t key_rep = 0x00U;
    std::uint16_t ring_cnt = 10U;

    // A/B/C released (0x0F held, bits 4,5,6 clear)
    auto res = poll_joypad_menu_hotkeys(
        0x0FU, 0x00U, 0x00U,
        0U, 0U, 0U, 0U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(!res.menu_triggered);
    assert(res.key_repeat_reset);
    assert(res.ring_count_cleared);
    assert(key_rep == 0xFFU);
    assert(ring_cnt == 0U);

    // 2. Start button edge (menu 0)
    menu_req = 0xFFU;
    key_rep = 0x00U;
    ring_cnt = 10U;
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x80U, 0x00U,
        0U, 0U, 0U, 0U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(res.menu_triggered);
    assert(res.menu_index == 0U);
    assert(menu_req == 0U);
    assert(!res.key_repeat_reset);

    // 3. Extra button 2 (Mode / Map menu 3)
    menu_req = 0xFFU;
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x00U, 0x0004U,
        0U, 0U, 0U, 0U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(res.menu_triggered);
    assert(res.menu_index == 3U);
    assert(menu_req == 3U);

    // 4. Extra button 1 (Item ring menu 1)
    menu_req = 0xFFU;
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x00U, 0x0002U,
        0U, 0U, 0U, 0U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(res.menu_triggered);
    assert(res.menu_index == 1U);
    assert(menu_req == 1U);

    // 5. Extra button 0 (Weapon ring menu 2)
    menu_req = 0xFFU;
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x00U, 0x0001U,
        0U, 0U, 0U, 0U,
        2U, 0U, // ali_substate == 2 is permitted
        menu_req, key_rep, ring_cnt);
    assert(res.menu_triggered);
    assert(res.menu_index == 2U);
    assert(menu_req == 2U);

    // 6. Guards: ali_substate > 2 blocks Item/Weapon rings
    menu_req = 0xFFU;
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x00U, 0x0003U,
        0U, 0U, 0U, 0U,
        3U, 0U, // ali_substate == 3 > 2
        menu_req, key_rep, ring_cnt);
    assert(!res.menu_triggered);

    // 7. Guards: spirit_active != 0 blocks Item/Weapon rings
    menu_req = 0xFFU;
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x00U, 0x0003U,
        0U, 0U, 0U, 0U,
        0U, 1U, // spirit active
        menu_req, key_rep, ring_cnt);
    assert(!res.menu_triggered);

    // 8. Lock guards abort all menus
    // Lock 1987
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x80U, 0x0007U,
        1U, 0U, 0U, 0U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(!res.menu_triggered);

    // Slow motion bit 2
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x80U, 0x0007U,
        0U, 4U, 0U, 0U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(!res.menu_triggered);

    // Lock 1983
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x80U, 0x0007U,
        0U, 0U, 1U, 0U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(!res.menu_triggered);

    // Lock 1982
    res = poll_joypad_menu_hotkeys(
        0x70U, 0x80U, 0x0007U,
        0U, 0U, 0U, 1U,
        0U, 0U,
        menu_req, key_rep, ring_cnt);
    assert(!res.menu_triggered);
}

static void test_master_frame_sequencer_normal() {
    GameplayFrameState state{};
    state.global_tick = 100U;
    state.anim_subtick = 10U;
    state.frame_count = 50U;
    state.joypad_held = 0x70U; // keep buttons held

    std::vector<PipelinePhase> executed;
    auto status = step_gameplay_frame(state, &executed);

    assert(status == FrameSequencerStatus::Continue);
    assert(state.global_tick == 102U);
    assert(state.anim_subtick == 11U);
    assert(state.frame_count == 51U);
    assert((state.flags_ff164d & 0x08U) != 0U); // bset #3
    assert((state.flags_ff164d & 0x20U) != 0U); // bset #5
    assert(state.frame_countdown == 2U);       // normal speed

    // Verifies phase pipeline execution order
    assert(executed.size() >= 13U);
    assert(executed[0] == PipelinePhase::JoypadPoll);
    assert(executed[1] == PipelinePhase::ScreenShakeStep);
    assert(executed[2] == PipelinePhase::EventVmTick);
    assert(executed[3] == PipelinePhase::DialogueHudStep);
    assert(executed[4] == PipelinePhase::SpiritManaStep);
    assert(executed[5] == PipelinePhase::SecondaryWeapon);
    assert(executed[6] == PipelinePhase::HazardBlast);
    assert(executed[7] == PipelinePhase::AliMachineStep);
    assert(executed[8] == PipelinePhase::EntitiesListStep);
    assert(executed[9] == PipelinePhase::EntityGcReclaim);
    assert(executed[10] == PipelinePhase::CameraClamp);
    assert(executed[11] == PipelinePhase::PlaneScrollSync);
    assert(executed[12] == PipelinePhase::SatCompose);
}

static void test_master_frame_sequencer_slowmo_and_pause() {
    GameplayFrameState state{};
    state.slow_motion_flags = 0x01U; // slow motion enabled
    state.pause_flag = 0x01U;        // pause menu active
    state.pause_busy = 0x00U;
    state.joypad_held = 0x70U;

    std::vector<PipelinePhase> executed;
    auto status = step_gameplay_frame(state, &executed);

    assert(status == FrameSequencerStatus::Continue);
    assert(state.frame_countdown == 3U); // slow-motion countdown
    assert(!executed.empty());
    assert(executed[0] == PipelinePhase::PauseMenu);
}

static void test_master_frame_sequencer_demo() {
    GameplayFrameState state{};
    state.demo_active = true;
    state.demo_playing = false; // finished demo

    auto status = step_gameplay_frame(state);
    assert(status == FrameSequencerStatus::ExitToTitle);

    // Active & playing demo clears enemy gauge
    state.demo_playing = true;
    state.enemy_gauge_timer = 90U;
    state.joypad_held = 0x70U;

    std::vector<PipelinePhase> executed;
    status = step_gameplay_frame(state, &executed);

    assert(status == FrameSequencerStatus::Continue);
    assert(state.enemy_gauge_timer == 0U);
    bool found_clear_phase = false;
    for (auto p : executed) {
        if (p == PipelinePhase::ClearEnemyGaugeInDemo) {
            found_clear_phase = true;
            break;
        }
    }
    assert(found_clear_phase);
}

int main() {
    std::cout << "[oasis_m16_144_master_frame_sequencer_test] running tests..." << std::endl;
    test_rom_constants();
    test_screen_shake_dispatcher();
    test_joypad_menu_hotkeys();
    test_master_frame_sequencer_normal();
    test_master_frame_sequencer_slowmo_and_pause();
    test_master_frame_sequencer_demo();
    std::cout << "[oasis_m16_144_master_frame_sequencer_test] all tests passed!" << std::endl;
    return 0;
}
