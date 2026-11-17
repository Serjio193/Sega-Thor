#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::ui {

struct ContinueAndCheckpointConfig {
    static constexpr std::uint32_t kContinueEntryAddr = 0x0000161CU;
    static constexpr std::uint32_t kChecksumLoaderAddr = 0x000016F4U;
    static constexpr std::uint32_t kSoftResetTargetAddr = 0x0000020EU;
    static constexpr std::uint32_t kSnapshotAddr = 0x000017DAU;
    static constexpr std::uint32_t kRetryRestoreAddr = 0x00001828U;
    static constexpr std::uint32_t kSpiritResourceLoaderAddr = 0x00014522U;

    static constexpr std::size_t kPayloadBytes = 596U;    // 0x12A words ($FF0BFE..$FF0E51)
    static constexpr std::size_t kEntityCopyBytes = 376U; // 0xBC words (Ali $FF19E8 + Spirit $FF1AA4)
    static constexpr std::size_t kSramBlockBytes = (kPayloadBytes + 2U) * 2U; // odd-byte stride 2
    static constexpr std::uint16_t kPostLoadInvulnFrames = 30U; // 0x1E ($FF198E)
};

struct ContinueLoadResult {
    bool success{false};
    bool used_backup{false};
    bool soft_reset_requested{false};
    std::uint32_t soft_reset_target{0U};
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kPayloadBytes> wram_596{};
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kEntityCopyBytes> entity_376{};
    std::uint16_t invuln_frames{0U};
    bool reload_spirit_resources{false};
};

// Encodes 596-byte WRAM payload + u16_be byte-sum checksum at odd-byte stride 2.
[[nodiscard]] std::vector<std::uint8_t> encode_sram_save_block(
    std::span<const std::uint8_t> payload_596);

// ROM 0x0016F4: Decodes and verifies 596-byte (0x12A-word) SRAM block + u16_be byte-sum checksum.
[[nodiscard]] std::optional<std::array<std::uint8_t, ContinueAndCheckpointConfig::kPayloadBytes>>
decode_sram_save_block(std::span<const std::uint8_t> odd_sram_bytes) noexcept;

// ROM 0x00161C: Tries primary SRAM, falls back to backup SRAM, or requests soft-reset (0x00020E)
// if both fail. Restores 376B Ali+Spirit state and grants 30 (0x1E) invulnerability frames.
[[nodiscard]] ContinueLoadResult load_continue_game(
    std::span<const std::uint8_t> primary_sram,
    std::span<const std::uint8_t> backup_sram) noexcept;

struct CheckpointLiveState {
    std::uint16_t room_id{0U};
    std::int32_t cam_x{0};
    std::int32_t cam_y{0};
    std::int32_t ali_x{0};
    std::int32_t ali_y{0};
    std::uint16_t hp{200U};
    std::uint16_t max_hp{200U};         // $FF1A70 (preserved on retry)
    std::uint16_t preserved_ff0d76{0U}; // $FF0D76 (preserved on retry)
    std::uint16_t preserved_ff1a7e{0U}; // $FF1A7E (preserved on retry)
    std::uint16_t continue_count{0U};   // $FF0E3E (incremented on retry)
    std::uint16_t invuln_frames{0U};    // $FF198E (30 on retry)
    bool reload_spirit_resources{false};
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kPayloadBytes> wram_596{};
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kEntityCopyBytes> entity_376{};
};

// ROM 0x0017DA & 0x001828: In-RAM checkpoint snapshot & Game-Over Retry restore manager.
class CheckpointRetryManager {
public:
    void reset() noexcept;

    // ROM 0x0017DA: Captures snapshot unless `demo_active` is true ($FF1996 != 0).
    bool capture_checkpoint(const CheckpointLiveState& live, bool demo_active = false) noexcept;

    // ROM 0x001828: Restores snapshot into `live` while preserving $FF0D76, $FF1A7E, and max_hp,
    // setting hp = max_hp, incrementing continue_count ($FF0E3E), and granting 30 invuln frames.
    bool restore_on_retry(CheckpointLiveState& live) const noexcept;

    [[nodiscard]] bool has_snapshot() const noexcept { return has_snapshot_; }

private:
    bool has_snapshot_{false};
    std::uint16_t snap_room_id_{0U};
    std::int32_t snap_cam_x_{0};
    std::int32_t snap_cam_y_{0};
    std::int32_t snap_ali_x_{0};
    std::int32_t snap_ali_y_{0};
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kPayloadBytes> snap_wram_596_{};
    std::array<std::uint8_t, ContinueAndCheckpointConfig::kEntityCopyBytes> snap_entity_376_{};
};

} // namespace oasis::game::ui
