#include "game/audio/sound_test_ost.hpp"

#include <cassert>

int main() {
    using oasis::game::audio::kSoundTestTracks;
    using oasis::game::audio::lookup_sound_test_track;
    using oasis::game::audio::SoundTestCategory;
    using oasis::game::audio::SoundTestMenuState;
    using oasis::game::combat::SoundCommandQueue;

    // 1. Verify all 34 OST tracks & key spot-checks.
    assert(kSoundTestTracks.size() == 34U);
    assert(lookup_sound_test_track({}, 0U)->bgm_cmd == 0x0115U);
    assert(lookup_sound_test_track({}, 0U)->title == "Introduction");
    assert(lookup_sound_test_track({}, 5U)->bgm_cmd == 0x0104U);
    assert(lookup_sound_test_track({}, 5U)->title == "The Story of Thor");
    assert(lookup_sound_test_track({}, 22U)->bgm_cmd == 0x011DU);
    assert(lookup_sound_test_track({}, 22U)->title == "Last battle");
    assert(lookup_sound_test_track({}, 32U)->bgm_cmd == 0x011BU);
    assert(lookup_sound_test_track({}, 32U)->title == "Ending");
    assert(lookup_sound_test_track({}, 33U)->bgm_cmd == 0x0121U);
    assert(lookup_sound_test_track({}, 33U)->title == "Master mind");
    assert(!lookup_sound_test_track({}, 34U).has_value());

    // 2. Unlock chord 0x90000000.
    assert(SoundTestMenuState::is_unlock_chord(0x90000000U));
    assert(!SoundTestMenuState::is_unlock_chord(0U));

    // 3. Menu cycling & SoundCommandQueue emission.
    SoundTestMenuState menu;
    SoundCommandQueue queue;
    menu.cycle_value(5);
    assert(menu.bgm_index == 5U);
    menu.play_selected(queue, 10);
    assert(menu.last_emitted_cmd == 0x0104U);
    assert(queue.latest_command_id() == 0x0104U);

    menu.stop_bgm(queue, 11);
    assert(queue.latest_command_id() == 0x0002U);

    menu.cycle_category(1); // Sfx
    menu.cycle_value(3);
    menu.play_selected(queue, 12);
    assert(queue.latest_command_id() == 0x0303U);

    menu.cycle_category(2); // Exit
    assert(menu.category == SoundTestCategory::Exit);
    menu.play_selected(queue, 13);
    assert(menu.exit_requested);

    return 0;
}
