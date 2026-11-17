#include "game/ui/continue_and_checkpoint.hpp"

#include <algorithm>

namespace oasis::game::ui {

std::vector<std::uint8_t> encode_sram_save_block(std::span<const std::uint8_t> payload_596) {
    std::vector<std::uint8_t> sram(ContinueAndCheckpointConfig::kSramBlockBytes, 0x00U);
    std::uint16_t sum = 0U;
    for (std::size_t i = 0U; i < ContinueAndCheckpointConfig::kPayloadBytes; ++i) {
        const std::uint8_t b = (i < payload_596.size()) ? payload_596[i] : 0U;
        sram[i * 2U] = b;
        sum = static_cast<std::uint16_t>(sum + b);
    }
    sram[ContinueAndCheckpointConfig::kPayloadBytes * 2U] =
        static_cast<std::uint8_t>((sum >> 8U) & 0xFFU);
    sram[(ContinueAndCheckpointConfig::kPayloadBytes + 1U) * 2U] =
        static_cast<std::uint8_t>(sum & 0xFFU);
    return sram;
}

std::optional<std::array<std::uint8_t, ContinueAndCheckpointConfig::kPayloadBytes>>
decode_sram_save_block(std::span<const std::uint8_t> odd_sram_bytes) noexcept {
    if (odd_sram_bytes.size() < ContinueAndCheckpointConfig::kSramBlockBytes) {
        return std::nullopt;
    }
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kPayloadBytes> payload{};
    std::uint16_t sum = 0U;
    for (std::size_t i = 0U; i < ContinueAndCheckpointConfig::kPayloadBytes; ++i) {
        const std::uint8_t b = odd_sram_bytes[i * 2U];
        payload[i] = b;
        sum = static_cast<std::uint16_t>(sum + b);
    }
    const std::uint16_t stored_sum = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(odd_sram_bytes[ContinueAndCheckpointConfig::kPayloadBytes * 2U]) << 8U) |
        static_cast<std::uint16_t>(odd_sram_bytes[(ContinueAndCheckpointConfig::kPayloadBytes + 1U) * 2U]));
    if (stored_sum != sum) {
        return std::nullopt;
    }
    return payload;
}

ContinueLoadResult load_continue_game(
    std::span<const std::uint8_t> primary_sram,
    std::span<const std::uint8_t> backup_sram) noexcept {
    ContinueLoadResult res{};
    bool used_backup = false;
    auto decoded = decode_sram_save_block(primary_sram);
    if (!decoded.has_value()) {
        decoded = decode_sram_save_block(backup_sram);
        used_backup = decoded.has_value();
    }
    if (!decoded.has_value()) {
        res.soft_reset_requested = true;
        res.soft_reset_target = ContinueAndCheckpointConfig::kSoftResetTargetAddr;
        return res;
    }

    res.success = true;
    res.used_backup = used_backup;
    res.wram_596 = *decoded;
    std::copy_n(res.wram_596.begin(), ContinueAndCheckpointConfig::kEntityCopyBytes, res.entity_376.begin());
    res.invuln_frames = ContinueAndCheckpointConfig::kPostLoadInvulnFrames; // 30 (0x1E)
    res.reload_spirit_resources = true;
    return res;
}

void CheckpointRetryManager::reset() noexcept {
    has_snapshot_ = false;
    snap_room_id_ = 0U;
    snap_cam_x_ = 0;
    snap_cam_y_ = 0;
    snap_ali_x_ = 0;
    snap_ali_y_ = 0;
    snap_wram_596_.fill(0U);
    snap_entity_376_.fill(0U);
}

bool CheckpointRetryManager::capture_checkpoint(
    const CheckpointLiveState& live,
    bool demo_active) noexcept {
    if (demo_active) {
        return false; // Skipped in demo ($FF1996 != 0)
    }
    has_snapshot_ = true;
    snap_room_id_ = live.room_id;
    snap_cam_x_ = live.cam_x;
    snap_cam_y_ = live.cam_y;
    snap_ali_x_ = live.ali_x;
    snap_ali_y_ = live.ali_y;
    snap_wram_596_ = live.wram_596;
    snap_entity_376_ = live.entity_376;
    return true;
}

bool CheckpointRetryManager::restore_on_retry(CheckpointLiveState& live) const noexcept {
    if (!has_snapshot_) {
        return false;
    }
    // Preserve $FF0D76, $FF1A7E, and MaxHP ($FF1A70)
    const std::uint16_t keep_ff0d76 = live.preserved_ff0d76;
    const std::uint16_t keep_ff1a7e = live.preserved_ff1a7e;
    const std::uint16_t keep_max_hp = live.max_hp;

    live.room_id = snap_room_id_;
    live.cam_x = snap_cam_x_;
    live.cam_y = snap_cam_y_;
    live.ali_x = snap_ali_x_;
    live.ali_y = snap_ali_y_;
    live.wram_596 = snap_wram_596_;
    live.entity_376 = snap_entity_376_;

    live.preserved_ff0d76 = keep_ff0d76;
    live.preserved_ff1a7e = keep_ff1a7e;
    live.max_hp = keep_max_hp;
    live.hp = keep_max_hp; // HP = MaxHP
    ++live.continue_count; // $FF0E3E += 1
    live.invuln_frames = ContinueAndCheckpointConfig::kPostLoadInvulnFrames; // 30 (0x1E)
    live.reload_spirit_resources = true;
    return true;
}

} // namespace oasis::game::ui
