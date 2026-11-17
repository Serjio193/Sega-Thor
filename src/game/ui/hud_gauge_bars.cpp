#include "game/ui/hud_gauge_bars.hpp"

namespace oasis::game::ui {

std::uint8_t compute_gauge_fill_rows(std::uint16_t cur, std::uint16_t max_val) noexcept {
    if (cur == 0U || max_val == 0U) {
        return 0U;
    }
    if (cur >= max_val) {
        return HudGaugeConfig::kMaxRows;
    }
    return static_cast<std::uint8_t>(
        ((static_cast<std::uint32_t>(cur) * HudGaugeConfig::kScaleNumerator) / max_val) + 1U);
}

void HudGaugeState::reset() noexcept {
    cached_cur_.fill(0U);
    cached_max_.fill(0U);
    cached_rows_.fill(0U);
    initialized_.fill(false);
}

void HudGaugeState::update_single_gauge(
    HudGaugeId id,
    std::uint32_t base_vdp_cmd,
    std::uint32_t fill_pattern,
    std::uint16_t cur,
    std::uint16_t max_val,
    std::vector<HudGaugeRowWrite>& out) {
    const std::size_t idx = static_cast<std::size_t>(id);
    if (initialized_[idx] && cached_cur_[idx] == cur && cached_max_[idx] == max_val) {
        return;
    }

    const std::uint8_t new_rows = compute_gauge_fill_rows(cur, max_val);
    const std::uint8_t old_rows = initialized_[idx] ? cached_rows_[idx] : 0U;

    if (new_rows > old_rows) {
        for (std::uint8_t r = old_rows; r < new_rows; ++r) {
            out.push_back(HudGaugeRowWrite{id, base_vdp_cmd, r, fill_pattern, false});
        }
    } else if (new_rows < old_rows) {
        const std::uint32_t erase_pattern = fill_pattern | HudGaugeConfig::kEraseOrMask;
        for (std::uint8_t r = new_rows; r < old_rows; ++r) {
            out.push_back(HudGaugeRowWrite{id, base_vdp_cmd, r, erase_pattern, true});
        }
    }

    cached_cur_[idx] = cur;
    cached_max_[idx] = max_val;
    cached_rows_[idx] = new_rows;
    initialized_[idx] = true;
}

std::vector<HudGaugeRowWrite> HudGaugeState::update_differential(
    std::uint16_t ali_hp,
    std::uint16_t ali_max_hp,
    std::uint16_t ali_sp,
    std::uint16_t ali_max_sp,
    std::uint16_t enemy_hp,
    std::uint16_t enemy_max_hp,
    std::uint8_t enemy_hud_timer_ff184f,
    bool demo_active) {
    std::vector<HudGaugeRowWrite> out;

    update_single_gauge(
        HudGaugeId::AliHp,
        HudGaugeConfig::kAliHpVdpCmd,
        HudGaugeConfig::kAliHpFillPattern,
        ali_hp,
        ali_max_hp,
        out);

    update_single_gauge(
        HudGaugeId::AliSp,
        HudGaugeConfig::kAliSpVdpCmd,
        HudGaugeConfig::kAliSpFillPattern,
        ali_sp,
        ali_max_sp,
        out);

    if (!demo_active && enemy_hud_timer_ff184f > 0U) {
        update_single_gauge(
            HudGaugeId::EnemyHp,
            HudGaugeConfig::kEnemyHpVdpCmd,
            HudGaugeConfig::kEnemyHpFillPattern,
            enemy_hp,
            enemy_max_hp,
            out);
    }

    return out;
}

} // namespace oasis::game::ui
