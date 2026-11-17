#include "game/audio/sound_channels.hpp"

#include <cassert>

int main() {
    using oasis::game::audio::SoundChannelBank;
    using oasis::game::audio::SoundChannelConfig;
    using oasis::game::audio::SoundChannelKind;

    SoundChannelBank bank;
    assert(bank.channels().size() == 11U);
    assert(bank.channels()[0].ram_addr == 0x00FF0022U && bank.channels()[0].kind == SoundChannelKind::Fm);
    assert(bank.channels()[5].ram_addr == 0x00FF0414U && bank.channels()[5].kind == SoundChannelKind::Fm);
    assert(bank.channels()[6].ram_addr == 0x00FF049EU && bank.channels()[6].kind == SoundChannelKind::Psg);
    assert(bank.channels()[8].ram_addr == 0x00FF05B2U && bank.channels()[8].kind == SoundChannelKind::Psg);
    assert(bank.channels()[9].ram_addr == 0x00FF0628U && bank.channels()[9].kind == SoundChannelKind::SfxFm);
    assert(bank.channels()[10].ram_addr == 0x00FF06F2U && bank.channels()[10].kind == SoundChannelKind::SfxPsg);

    // Activate all 11 channels, then start_fade_all(2) (0x060516).
    for (auto& ch : bank.channels()) {
        ch.active = true;
    }
    bank.start_fade_all(SoundChannelConfig::kFadeInitStep);
    assert(bank.is_fading());
    assert(bank.fade_step() == 2U);
    assert(bank.fade_timer() == 0x7EU);
    for (const auto& ch : bank.channels()) {
        assert((ch.flags_75 & SoundChannelConfig::kFadeBit4) != 0U);
    }

    bank.step_frame();
    bank.step_frame();
    assert(bank.channels()[0].attenuation == 1U);

    // Finish all remaining fade frames -> channels mute and clear fade bit 4.
    while (bank.is_fading()) {
        bank.step_frame();
    }
    for (const auto& ch : bank.channels()) {
        assert(!ch.active);
        assert((ch.flags_75 & SoundChannelConfig::kFadeBit4) == 0U);
    }

    // stop_all (0x0604E6) mutes all 11 channels and clears global flags.
    bank.channels()[0].active = true;
    bank.stop_all();
    assert(!bank.channels()[0].active);
    assert(bank.global_flags_ff0010() == 0U);

    return 0;
}
