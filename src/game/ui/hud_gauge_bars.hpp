#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace oasis::game::ui {

enum class HudGaugeId : std::uint8_t {
    AliHp = 0,
    AliSp = 1,
    EnemyHp = 2,
};

struct HudGaugeConfig {
    static constexpr std::uint32_t kEntryAddr = 0x0000267CU;
    static constexpr std::uint32_t kFillKernelAddr = 0x0000271CU;
    static constexpr std::uint8_t kMaxRows = 48U; // 0x30
    static constexpr std::uint32_t kScaleNumerator = 47U;
    static constexpr std::uint32_t kEraseOrMask = 0x00EEEE00U;

    // Ali HP (0x00267C)
    static constexpr std::uint32_t kAliHpVdpCmd = 0x70E00003U;
    static constexpr std::uint32_t kAliHpFillPattern = 0x4E2222E4U;

    // Ali SP (0x0026B2)
    static constexpr std::uint32_t kAliSpVdpCmd = 0x72800003U;
    static constexpr std::uint32_t kAliSpFillPattern = 0x4E6666E4U;

    // Target Enemy HP (0x0026D6)
    static constexpr std::uint32_t kEnemyHpVdpCmd = 0x74200003U;
    static constexpr std::uint32_t kEnemyHpFillPattern = 0xDE4444EDU;
};

// ROM 0x00271C: Computes vertical bar fill height in rows (0..48):
// 0 -> 0, cur >= max_val -> 48, else ((cur * 47) / max_val) + 1.
[[nodiscard]] std::uint8_t compute_gauge_fill_rows(
    std::uint16_t cur,
    std::uint16_t max_val) noexcept;

struct HudGaugeRowWrite {
    HudGaugeId gauge{HudGaugeId::AliHp};
    std::uint32_t vdp_cmd{0U};
    std::uint8_t row_index{0U}; // 0..47
    std::uint32_t pattern_word{0U};
    bool is_erase{false};
};

// Models cached (cur, max) ($FF1840) and cached row heights ($FF184C) for differential
// VDP row-delta emission at 0x002744..0x0027A4.
class HudGaugeState {
public:
    void reset() noexcept;

    // Updates all 3 gauges differentially; returns VDP row-delta writes emitted this frame.
    // Enemy HP is gated by `enemy_hud_timer_ff184f > 0 && !demo_active`.
    std::vector<HudGaugeRowWrite> update_differential(
        std::uint16_t ali_hp,
        std::uint16_t ali_max_hp,
        std::uint16_t ali_sp,
        std::uint16_t ali_max_sp,
        std::uint16_t enemy_hp,
        std::uint16_t enemy_max_hp,
        std::uint8_t enemy_hud_timer_ff184f,
        bool demo_active = false);

    [[nodiscard]] std::uint8_t cached_rows(HudGaugeId id) const noexcept {
        return cached_rows_[static_cast<std::size_t>(id)];
    }

private:
    void update_single_gauge(
        HudGaugeId id,
        std::uint32_t base_vdp_cmd,
        std::uint32_t fill_pattern,
        std::uint16_t cur,
        std::uint16_t max_val,
        std::vector<HudGaugeRowWrite>& out);

    std::array<std::uint16_t, 3> cached_cur_{};
    std::array<std::uint16_t, 3> cached_max_{};
    std::array<std::uint8_t, 3> cached_rows_{};
    std::array<bool, 3> initialized_{};
};

} // namespace oasis::game::ui
