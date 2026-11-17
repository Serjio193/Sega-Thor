#include "game/audio/sound_test_ost.hpp"

namespace oasis::game::audio {

const std::array<SoundTestTrackEntry, SoundTestConfig::kTrackCount> kSoundTestTracks = {{
    {0U, 21U, 0x0115U, "Introduction"},
    {1U, 19U, 0x0113U, "Awakening"},
    {2U, 28U, 0x011CU, "To begin"},
    {3U, 0U, 0x0100U, "Battle"},
    {4U, 6U, 0x0106U, "Peaceful"},
    {5U, 4U, 0x0104U, "The Story of Thor"},
    {6U, 15U, 0x010FU, "Holiness"},
    {7U, 12U, 0x010CU, "Aqua"},
    {8U, 14U, 0x010EU, "Burning cave"},
    {9U, 16U, 0x0110U, "Fortress"},
    {10U, 11U, 0x010BU, "Vessel"},
    {11U, 1U, 0x0101U, "Indication"},
    {12U, 17U, 0x0111U, "Stone place"},
    {13U, 9U, 0x0109U, "Boundless cliff"},
    {14U, 24U, 0x0118U, "Raging wind"},
    {15U, 5U, 0x0105U, "Abyss"},
    {16U, 18U, 0x0112U, "Mysterious green"},
    {17U, 22U, 0x0116U, "Chaos"},
    {18U, 25U, 0x0119U, "Evil territory"},
    {19U, 7U, 0x0107U, "Water cave"},
    {20U, 2U, 0x0102U, "The huge creature"},
    {21U, 23U, 0x0117U, "Voice from darkside"},
    {22U, 29U, 0x011DU, "Last battle"},
    {23U, 3U, 0x0103U, "Triumph"},
    {24U, 8U, 0x0108U, "Item get!"},
    {25U, 13U, 0x010DU, "Magic jewel"},
    {26U, 10U, 0x010AU, "Encounter"},
    {27U, 20U, 0x0114U, "Requiem"},
    {28U, 31U, 0x011FU, "Concentration"},
    {29U, 30U, 0x011EU, "Fire ball"},
    {30U, 32U, 0x0120U, "Deep hole"},
    {31U, 26U, 0x011AU, "Confession"},
    {32U, 27U, 0x011BU, "Ending"},
    {33U, 33U, 0x0121U, "Master mind"},
}};

std::optional<SoundTestTrackEntry> lookup_sound_test_track(
    std::span<const std::uint8_t> rom,
    std::uint8_t menu_index) noexcept {
    if (menu_index >= SoundTestConfig::kTrackCount) {
        return std::nullopt;
    }
    SoundTestTrackEntry entry = kSoundTestTracks[menu_index];
    const std::size_t map_off =
        static_cast<std::size_t>(SoundTestConfig::kTrackIdMapAddr) + menu_index;
    if (map_off < rom.size()) {
        entry.track_byte = rom[map_off];
        entry.bgm_cmd = static_cast<std::uint16_t>(0x0100U | entry.track_byte);
    }
    return entry;
}

void SoundTestMenuState::reset() noexcept {
    *this = SoundTestMenuState{};
}

void SoundTestMenuState::cycle_category(int delta) noexcept {
    const int cur = static_cast<int>(category);
    const int next = ((cur + delta) % 4 + 4) % 4;
    category = static_cast<SoundTestCategory>(next);
}

void SoundTestMenuState::cycle_value(int delta) noexcept {
    switch (category) {
        case SoundTestCategory::Bgm: {
            const int n = static_cast<int>(SoundTestConfig::kTrackCount);
            bgm_index = static_cast<std::uint8_t>(((static_cast<int>(bgm_index) + delta) % n + n) % n);
            break;
        }
        case SoundTestCategory::Sfx:
            sfx_index = static_cast<std::uint8_t>(static_cast<int>(sfx_index) + delta);
            break;
        case SoundTestCategory::Pcm:
            pcm_index = static_cast<std::uint8_t>(static_cast<int>(pcm_index) + delta);
            break;
        case SoundTestCategory::Exit:
            break;
    }
}

void SoundTestMenuState::play_selected(combat::SoundCommandQueue& queue, int frame_60hz) {
    switch (category) {
        case SoundTestCategory::Bgm: {
            const auto track = kSoundTestTracks[bgm_index % SoundTestConfig::kTrackCount];
            last_emitted_cmd = track.bgm_cmd;
            queue.push(frame_60hz, last_emitted_cmd);
            break;
        }
        case SoundTestCategory::Sfx:
            last_emitted_cmd = 0x00800300U | sfx_index;
            queue.push(frame_60hz, last_emitted_cmd);
            break;
        case SoundTestCategory::Pcm:
            last_emitted_cmd = 0x00800400U | pcm_index;
            queue.push(frame_60hz, last_emitted_cmd);
            break;
        case SoundTestCategory::Exit:
            exit_requested = true;
            break;
    }
}

void SoundTestMenuState::stop_bgm(combat::SoundCommandQueue& queue, int frame_60hz) {
    last_emitted_cmd = SoundTestConfig::kStopBgmCmd;
    queue.push(frame_60hz, last_emitted_cmd);
}

} // namespace oasis::game::audio
