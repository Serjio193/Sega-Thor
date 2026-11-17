#include "game/ui/continue_and_checkpoint.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using oasis::game::ui::CheckpointLiveState;
    using oasis::game::ui::CheckpointRetryManager;
    using oasis::game::ui::ContinueAndCheckpointConfig;
    using oasis::game::ui::decode_sram_save_block;
    using oasis::game::ui::encode_sram_save_block;
    using oasis::game::ui::load_continue_game;

    // 1. SRAM 596-byte encode/decode + primary/backup fallback + soft reset.
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kPayloadBytes> payload{};
    payload[0] = 0x5AU;
    payload[375] = 0xC3U;
    payload[595] = 0x7EU;

    auto primary = encode_sram_save_block(payload);
    auto backup = encode_sram_save_block(payload);
    assert(decode_sram_save_block(primary).has_value());

    const auto ok_primary = load_continue_game(primary, backup);
    assert(ok_primary.success && !ok_primary.used_backup && !ok_primary.soft_reset_requested);
    assert(ok_primary.invuln_frames == 30U);
    assert(ok_primary.reload_spirit_resources);
    assert(ok_primary.entity_376[0] == 0x5AU && ok_primary.entity_376[375] == 0xC3U);
    assert(ok_primary.wram_596[595] == 0x7EU);

    // Corrupt primary -> falls back to backup.
    primary[0] ^= 0xFFU;
    const auto ok_backup = load_continue_game(primary, backup);
    assert(ok_backup.success && ok_backup.used_backup && !ok_backup.soft_reset_requested);
    assert(ok_backup.invuln_frames == 30U);

    // Corrupt both -> soft reset 0x00020E.
    backup[0] ^= 0xFFU;
    const auto both_bad = load_continue_game(primary, backup);
    assert(!both_bad.success && both_bad.soft_reset_requested);
    assert(both_bad.soft_reset_target == 0x0000020EU);

    // 2. CheckpointRetryManager: skipped in demo, preserves $FF0D76/$FF1A7E/MaxHP, HP=MaxHP, +1 continue.
    CheckpointRetryManager mgr;
    CheckpointLiveState live;
    live.room_id = 0x0106U;
    live.cam_x = 120; live.cam_y = 64;
    live.ali_x = 256; live.ali_y = 180;
    live.max_hp = 240U;
    live.hp = 240U;
    live.wram_596[10] = 0x42U;
    live.entity_376[20] = 0x99U;

    assert(!mgr.capture_checkpoint(live, true)); // Skipped in demo
    assert(!mgr.has_snapshot());

    assert(mgr.capture_checkpoint(live, false));
    assert(mgr.has_snapshot());

    // Simulate death and state changes before retry.
    live.room_id = 0x0107U;
    live.ali_x = 400;
    live.hp = 0U;
    live.max_hp = 260U;
    live.preserved_ff0d76 = 0x1234U;
    live.preserved_ff1a7e = 0x5678U;
    live.wram_596[10] = 0x00U;
    live.entity_376[20] = 0x00U;

    assert(mgr.restore_on_retry(live));
    assert(live.room_id == 0x0106U);
    assert(live.ali_x == 256 && live.ali_y == 180);
    assert(live.wram_596[10] == 0x42U);
    assert(live.entity_376[20] == 0x99U);
    assert(live.preserved_ff0d76 == 0x1234U);
    assert(live.preserved_ff1a7e == 0x5678U);
    assert(live.max_hp == 260U && live.hp == 260U);
    assert(live.continue_count == 1U);
    assert(live.invuln_frames == 30U);
    assert(live.reload_spirit_resources);

    return 0;
}
