#include "game/ui/hud_gauge_bars.hpp"

#include <cassert>

int main() {
    using oasis::game::ui::compute_gauge_fill_rows;
    using oasis::game::ui::HudGaugeConfig;
    using oasis::game::ui::HudGaugeId;
    using oasis::game::ui::HudGaugeState;

    // 1. Gauge fill row formula (0x00271C).
    assert(compute_gauge_fill_rows(0U, 200U) == 0U);
    assert(compute_gauge_fill_rows(200U, 200U) == 48U);
    assert(compute_gauge_fill_rows(250U, 200U) == 48U);
    assert(compute_gauge_fill_rows(100U, 200U) == ((100U * 47U) / 200U) + 1U); // 24

    // 2. Differential VDP row-delta writes.
    HudGaugeState hud;
    const auto first = hud.update_differential(200U, 200U, 0U, 100U, 50U, 100U, 0U, false);
    assert(first.size() == 48U); // Ali HP 48 rows; SP 0 rows; Enemy HP gated off (timer == 0)
    assert(first[0].vdp_cmd == HudGaugeConfig::kAliHpVdpCmd);
    assert(first[0].pattern_word == HudGaugeConfig::kAliHpFillPattern);
    assert(!first[0].is_erase);

    // Unchanged stats -> 0 writes.
    const auto same = hud.update_differential(200U, 200U, 0U, 100U, 50U, 100U, 0U, false);
    assert(same.empty());

    // Ali HP drops from 200 -> 100 (48 -> 24 rows = 24 erase writes with 0x00EEEE00).
    // Enemy HP timer active (90) and !demo -> 24 fill writes.
    const auto delta = hud.update_differential(100U, 200U, 0U, 100U, 50U, 100U, 90U, false);
    assert(delta.size() == 48U);
    assert(delta[0].gauge == HudGaugeId::AliHp && delta[0].is_erase);
    assert(delta[0].pattern_word == (HudGaugeConfig::kAliHpFillPattern | HudGaugeConfig::kEraseOrMask));
    assert(delta[24].gauge == HudGaugeId::EnemyHp && !delta[24].is_erase);
    assert(delta[24].vdp_cmd == HudGaugeConfig::kEnemyHpVdpCmd);
    assert(delta[24].pattern_word == HudGaugeConfig::kEnemyHpFillPattern);

    return 0;
}
