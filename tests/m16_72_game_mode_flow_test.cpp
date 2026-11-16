#include "game/ui/game_mode_flow.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

void write_be32(std::vector<std::uint8_t>& buf, std::size_t off, std::uint32_t v) {
    buf[off] = static_cast<std::uint8_t>((v >> 24U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>((v >> 16U) & 0xFFU);
    buf[off + 2U] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
    buf[off + 3U] = static_cast<std::uint8_t>(v & 0xFFU);
}

} // namespace

int main() {
    using oasis::game::ui::ClearRecordsScreenState;
    using oasis::game::ui::GameModeFlowConfig;
    using oasis::game::ui::GameModeStateMachine;
    using oasis::game::ui::lookup_mode_handler_addr;
    using oasis::game::ui::TitleScreenState;
    using oasis::game::ui::TopLevelGameMode;

    // 1. 0x00045E pointer table lookup.
    std::vector<std::uint8_t> rom(0x000500U, 0x00U);
    write_be32(rom, GameModeFlowConfig::kModeTableRomAddr + 0x00U, 0x0000307AU);
    write_be32(rom, GameModeFlowConfig::kModeTableRomAddr + 0x04U, 0x000032F8U);
    write_be32(rom, GameModeFlowConfig::kModeTableRomAddr + 0x08U, 0x000089B2U);
    write_be32(rom, GameModeFlowConfig::kModeTableRomAddr + 0x0CU, 0x000089B2U);
    write_be32(rom, GameModeFlowConfig::kModeTableRomAddr + 0x10U, 0x0003A748U);

    assert(lookup_mode_handler_addr(rom, TopLevelGameMode::SegaLogo) == 0x0000307AU);
    assert(lookup_mode_handler_addr(rom, TopLevelGameMode::TitleAndFileSelect) == 0x000032F8U);
    assert(lookup_mode_handler_addr(rom, TopLevelGameMode::GameplayNew) == 0x000089B2U);
    assert(lookup_mode_handler_addr(rom, TopLevelGameMode::GameplayAlt) == 0x000089B2U);
    assert(lookup_mode_handler_addr(rom, TopLevelGameMode::EndingAndCredits) == 0x0003A748U);

    // 2. $FF10AC / $FF10AE latch semantics.
    GameModeStateMachine sm;
    assert(!sm.latch_pending_mode());
    sm.request_mode(TopLevelGameMode::TitleAndFileSelect);
    assert(sm.latch_pending_mode());
    assert(sm.current_mode == TopLevelGameMode::TitleAndFileSelect);
    assert(sm.pending_mode == GameModeFlowConfig::kNoPendingMode);

    // 3. TitleScreenState: 128-frame blink duty + 720-frame attract timeout.
    TitleScreenState title;
    assert(title.press_start_visible()); // timer 0 < 96
    for (int f = 0; f < 96; ++f) {
        title.step_frame(false, 0U);
    }
    assert(!title.press_start_visible()); // timer 96 >= 96
    for (int f = 96; f < 720; ++f) {
        title.step_frame(false, 0U);
    }
    assert(title.pending_mode == static_cast<std::uint16_t>(TopLevelGameMode::GameplayNew));
    assert(title.demo_flag);

    // 4. All 4 Start-button input chords.
    title.reset();
    title.step_frame(true, 0x90U);
    assert(title.entered_clear_records && title.last_sound_cmd == 0x00A003AAU);

    title.reset();
    title.step_frame(true, 0xD5U);
    assert(title.entered_clear_records && title.clear_sram_records_requested);
    assert(title.last_sound_cmd == 0x00A00205U);

    title.reset();
    title.step_frame(true, 0x8AU);
    assert(title.pending_mode == 0x0008U && title.demo_flag);

    title.reset();
    title.step_frame(true, 0x80U);
    assert(title.pending_mode == 0x000CU && !title.demo_flag && title.last_sound_cmd == 0x00000002U);

    // 5. ClearRecordsScreenState (0x03E4FA): 5 slots, BGM 0x0121, SRAM clear, Start -> 0x00020E.
    ClearRecordsScreenState records;
    assert(records.active_bgm_id == 0x0121U);
    records.slots[0] = {true, 36000U, 15U, 120U, 8U};
    records.clear_sram_record_magic();
    assert(records.sram_magic_2025cd == 0U && records.sram_magic_2026e1 == 0U);
    assert(!records.slots[0].populated);

    records.step_frame(true);
    assert(records.last_sound_cmd == 0x00000007U);
    assert(records.soft_reset_requested && records.soft_reset_target == 0x0000020EU);

    return 0;
}
