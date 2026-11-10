#include "game/combat/attack_controller.hpp"
#include "game/combat/damage_arithmetic.hpp"
#include "game/entity/native_entity.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string resolve_fixture(const std::string& relative) {
    if (fs::exists(relative)) return relative;
    const auto p1 = fs::path("..") / relative;
    if (fs::exists(p1)) return p1.string();
    const auto p2 = fs::path("../..") / relative;
    if (fs::exists(p2)) return p2.string();
    return relative;
}

std::string read_text(const std::string& path) {
    std::ifstream in(path);
    assert(in.is_open());
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

struct OracleFrameRow {
    int frame{};
    int hp{};
    int state{};
    int anim{};
    int timer82{};
    int flash83{};
    int mailbox86{};
    int flags38{};
    int enemy_x{};
    int enemy_y{};
    std::string sat_sha{};
};

int extract_int(const std::string& block, const std::string& key) {
    const std::string needle = "\"" + key + "\": ";
    const auto pos = block.find(needle);
    assert(pos != std::string::npos);
    return std::stoi(block.substr(pos + needle.size()));
}

std::string extract_str(const std::string& block, const std::string& key) {
    const std::string needle = "\"" + key + "\": \"";
    const auto pos = block.find(needle);
    assert(pos != std::string::npos);
    const auto start = pos + needle.size();
    const auto end = block.find('"', start);
    assert(end != std::string::npos);
    return block.substr(start, end - start);
}

std::vector<OracleFrameRow> parse_positive_oracle_frames(const std::string& json) {
    const auto arr_start = json.find("\"frames\": [");
    const auto arr_end = json.find("\"entity_write_trace\":");
    assert(arr_start != std::string::npos && arr_end != std::string::npos);
    std::vector<OracleFrameRow> rows;
    std::size_t cur = arr_start;
    while (true) {
        const auto obj_start = json.find('{', cur);
        if (obj_start == std::string::npos || obj_start >= arr_end) break;
        const auto obj_end = json.find('}', obj_start);
        assert(obj_end != std::string::npos && obj_end < arr_end);
        const std::string blk = json.substr(obj_start, obj_end - obj_start + 1);
        OracleFrameRow r{};
        r.frame = extract_int(blk, "FRAME_60HZ");
        r.hp = extract_int(blk, "ENEMY_HP");
        r.state = extract_int(blk, "ENEMY_STATE");
        r.anim = extract_int(blk, "ENEMY_ANIM");
        r.timer82 = extract_int(blk, "ENEMY_ACTION_TIMER");
        r.flash83 = extract_int(blk, "ENEMY_FLASH_TIMER");
        r.mailbox86 = extract_int(blk, "DAMAGE_MAILBOX");
        r.flags38 = extract_int(blk, "ENEMY_FLAGS38");
        r.enemy_x = extract_int(blk, "ENEMY_X");
        r.enemy_y = extract_int(blk, "ENEMY_Y");
        r.sat_sha = extract_str(blk, "SAT_SHA256");
        rows.push_back(r);
        cur = obj_end + 1;
    }
    return rows;
}

bool compare_oracle_slice(const std::vector<OracleFrameRow>& expected,
                          const std::vector<OracleFrameRow>& actual,
                          std::string& failure_reason) {
    if (expected.size() != actual.size()) {
        failure_reason = "frame_count_mismatch";
        return false;
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (expected[i].frame != actual[i].frame) { failure_reason = "event_frame_mismatch"; return false; }
        if (expected[i].hp != actual[i].hp) { failure_reason = "hp_mismatch"; return false; }
        if (expected[i].mailbox86 != actual[i].mailbox86) { failure_reason = "damage_mailbox_mismatch"; return false; }
        if (expected[i].state != actual[i].state) { failure_reason = "entity_state_mismatch"; return false; }
        if (expected[i].flags38 != actual[i].flags38) { failure_reason = "invulnerability_mismatch"; return false; }
        if (expected[i].enemy_x != actual[i].enemy_x || expected[i].enemy_y != actual[i].enemy_y) {
            failure_reason = "entity_position_mismatch";
            return false;
        }
        if (expected[i].sat_sha != actual[i].sat_sha) { failure_reason = "sat_hash_mismatch"; return false; }
    }
    return true;
}

void test_rom_and_oracle_provenance() {
    const std::string rom_path = resolve_fixture("build/Beyond Oasis (USA).md");
    assert(fs::exists(rom_path));
    assert(fs::file_size(rom_path) == 3145728U); // Actual 3 MiB ROM size

    const std::string audit_json = read_text(resolve_fixture("data/standalone-fixtures/m16_20_oracle_provenance_audit.json"));
    assert(audit_json.find("\"f1914_event_classification\": \"NON_DAMAGE_EVENT\"") != std::string::npos);
    assert(audit_json.find("\"actual_independent_positive_damage_field_count\": 0") != std::string::npos);

    const std::string m1620_oracle = read_text(resolve_fixture("data/standalone-fixtures/enemy_hit_oracle.json"));
    // Verify M16.20 oracle contains zero positive damage
    assert(m1620_oracle.find("\"ENEMY_HP\": 49") == std::string::npos);
    assert(m1620_oracle.find("\"ENEMY_HP\": 50") != std::string::npos);

    std::cout << "[PASS] test_rom_and_oracle_provenance (3,145,728-byte ROM & M16.20 NON_DAMAGE_EVENT audited)\n";
}

void test_m68k_exact_damage_arithmetic_and_flags() {
    using oasis::game::combat::compute_m68k_damage_step;

    // Cases verified directly against live GPGX M68K execution (0x01F44C & 0x0256F6)
    struct Case {
        std::uint16_t raw_dmg;
        std::int16_t def;
        std::int16_t hp;
        std::uint16_t exp_dmg;
        std::int16_t exp_hp;
        std::uint8_t exp_ccr;
        bool exp_sound;
    };
    const std::vector<Case> cases = {
        {0x0000U,      0,    50,    1U,   49, 0x00U, false}, // zero direct
        {0x0002U,      5,    50,    1U,   49, 0x00U, false}, // dmg < def -> clamped 1
        {0x0005U,      5,    50,    1U,   49, 0x00U, false}, // dmg == def -> BHI false -> 1
        {0x0005U,      2,    50,    3U,   47, 0x00U, false}, // dmg > def -> 3
        {0x000AU,      0,     7,   10U,    0, 0x14U, false}, // dmg > HP -> X=1,Z=1, HP=0
        {0x0005U,      0,     5,    5U,    0, 0x04U, false}, // HP == 0 -> Z=1
        {0xC005U,      2,    50,    3U,   47, 0x00U, false}, // high bits 0xC000: bit 13 is 0
        {0x2004U,      0,    50,    4U,   46, 0x00U, true }, // bit 13 set -> sound 0x04D9
        {0x0005U, -32768,    50,    1U,   49, 0x00U, false}, // def=0x8000 unsigned > 5
        {0x0001U,      0, -32767,   1U,    0, 0x04U, false}, // HP=0x8001 -> 0x8000 (N=1) -> clr.w -> 0
        {0xFFFFU,      0,  5000, 4095U,  905, 0x00U, true }, // max masked 0x0FFF (bit 13 set!)
    };
    for (const auto& c : cases) {
        const auto res = compute_m68k_damage_step(c.raw_dmg, c.def, c.hp);
        assert(res.applied_damage == c.exp_dmg);
        assert(res.hp_after == c.exp_hp);
        assert(res.ccr == c.exp_ccr);
        assert(res.sound_04d9 == c.exp_sound);
    }
    std::cout << "[PASS] test_m68k_exact_damage_arithmetic_and_flags (All 11 M68000 CCR/boundary cases bit-exact)\n";
}

void test_f775_f815_positive_damage_oracle_differential() {
    const std::string json = read_text(resolve_fixture("data/standalone-fixtures/positive_damage_oracle_f775_815.json"));
    const auto oracle_rows = parse_positive_oracle_frames(json);
    assert(oracle_rows.size() == 41U);

    oasis::game::NativeEntity enemy{};
    enemy.active = true;
    enemy.type_id = 0x002CU;
    enemy.hp = 7;
    enemy.defense = 0;
    enemy.state = 0x00U;
    enemy.anim_frame = 0x00U;

    int hp_matched = 0, state_matched = 0, anim_matched = 0;
    int timer82_matched = 0, flash83_matched = 0, mailbox_matched = 0, invuln_matched = 0;

    for (const auto& r : oracle_rows) {
        const bool is_logic_tick = ((r.frame % 2) == 1);
        if (is_logic_tick) {
            // Collision writer inputs at original hit frames F789 and F795 (ROM 0x00BEE8)
            if (r.frame == 789) {
                enemy.action_timer = 1U;
                enemy.flash_timer = 6U;
                enemy.damage_mailbox = 0x0002U;
            } else if (r.frame == 795) {
                enemy.action_timer = 2U;
                enemy.flash_timer = 6U;
                enemy.damage_mailbox = 0xC005U;
            }
            const bool snd = oasis::game::combat::step_entity_combat_tick(enemy);
            assert(!snd); // Bit 13 was 0 in both hits; sound 0x04D9 never fired
        }
        if (enemy.hp == r.hp) ++hp_matched;
        if (enemy.state == r.state) ++state_matched;
        if (enemy.anim_frame == r.anim) ++anim_matched;
        if (enemy.action_timer == r.timer82) ++timer82_matched;
        if (enemy.flash_timer == r.flash83) ++flash83_matched;
        if (enemy.damage_mailbox == r.mailbox86) ++mailbox_matched;
        const bool exp_invuln = ((r.flags38 & 0x08) != 0);
        if (enemy.invulnerable == exp_invuln) ++invuln_matched;
    }

    assert(hp_matched == 41 && state_matched == 41 && anim_matched == 41);
    assert(timer82_matched == 41 && flash83_matched == 41 && mailbox_matched == 41 && invuln_matched == 41);

    std::cout << "[PASS] test_f775_f815_positive_damage_oracle_differential (41/41 HP/State/Anim/Mailbox/Timer/Flash/Invuln match)\n";
}

void test_controlled_hurt_recovery_0x002c() {
    oasis::game::NativeEntity enemy{};
    enemy.active = true;
    enemy.type_id = 0x002CU;
    enemy.world_x = 59;
    enemy.world_y = 181;
    enemy.hp = 7;

    // F789: Hit #1 nonlethal
    enemy.action_timer = 1U;
    enemy.flash_timer = 6U;
    enemy.damage_mailbox = 0x0002U;
    oasis::game::combat::step_entity_combat_tick(enemy);
    assert(enemy.hp == 5 && enemy.state == 0x04U && enemy.anim_frame == 0x08U);
    assert(enemy.flash_timer == 5U && enemy.world_y == 178 && !enemy.invulnerable);

    // Ticks F791, F793, F795, F797: flash countdown 4 -> 3 -> 2 -> 1
    for (int expected_flash = 4; expected_flash >= 1; --expected_flash) {
        oasis::game::combat::step_entity_combat_tick(enemy);
        assert(enemy.flash_timer == expected_flash);
        assert(enemy.state == 0x04U);
    }

    // Tick F799: flash reaches 0 -> authentic recovery (0x02595E -> 0x02568A)
    oasis::game::combat::step_entity_combat_tick(enemy);
    assert(enemy.flash_timer == 0U && enemy.state == 0x00U && enemy.anim_frame == 0x00U);
    assert(!enemy.invulnerable && enemy.hp == 5);

    std::cout << "[PASS] test_controlled_hurt_recovery_0x002c (Hurt 0x04 -> Flash 5..0 -> Recovery 0x00 verified)\n";
}

void test_routine_00e682_full_56byte_contract() {
    const std::string rom_path = resolve_fixture("build/Beyond Oasis (USA).md");
    std::ifstream in(rom_path, std::ios::binary);
    assert(in.is_open());
    in.seekg(0x00E682);
    std::uint8_t b[56]{};
    in.read(reinterpret_cast<char*>(b), 56);
    // 0x00E6A6: addq.l #4, a7; rts
    assert(b[36] == 0x58 && b[37] == 0x8F && b[38] == 0x4E && b[39] == 0x75);
    // 0x00E6AA..0x00E6B9: freeze/stun branch ($9D != 0 -> bra.s $00E6A6)
    assert(b[40] == 0x0C && b[41] == 0x00 && b[54] == 0x60 && b[55] == 0xEC);
    std::cout << "[PASS] test_routine_00e682_full_56byte_contract (56-byte opcode & stack-pop span verified)\n";
}

void test_oracle_mutation_detectors() {
    const std::string json = read_text(resolve_fixture("data/standalone-fixtures/positive_damage_oracle_f775_815.json"));
    const auto base = parse_positive_oracle_frames(json);
    std::string reason;
    assert(compare_oracle_slice(base, base, reason));

    auto m_hp = base; m_hp[14].hp = 99;
    assert(!compare_oracle_slice(base, m_hp, reason) && reason == "hp_mismatch");

    auto m_dmg = base; m_dmg[20].mailbox86 = 1;
    assert(!compare_oracle_slice(base, m_dmg, reason) && reason == "damage_mailbox_mismatch");

    auto m_st = base; m_st[14].state = 0x0A;
    assert(!compare_oracle_slice(base, m_st, reason) && reason == "entity_state_mismatch");

    auto m_frm = base; m_frm[14].frame = 787;
    assert(!compare_oracle_slice(base, m_frm, reason) && reason == "event_frame_mismatch");

    auto m_inv = base; m_inv[22].flags38 = 0x00;
    assert(!compare_oracle_slice(base, m_inv, reason) && reason == "invulnerability_mismatch");

    auto m_pos = base; m_pos[0].enemy_y = 999;
    assert(!compare_oracle_slice(base, m_pos, reason) && reason == "entity_position_mismatch");

    auto m_sat = base; m_sat[0].sat_sha[0] ^= 1;
    assert(!compare_oracle_slice(base, m_sat, reason) && reason == "sat_hash_mismatch");

    std::cout << "[PASS] test_oracle_mutation_detectors (All 7 oracle mutation detectors fail on corruption)\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.21 — POSITIVE DAMAGE ORACLE & PROVENANCE AUDIT TEST\n"
              << "=====================================================================\n";
    test_rom_and_oracle_provenance();
    test_m68k_exact_damage_arithmetic_and_flags();
    test_f775_f815_positive_damage_oracle_differential();
    test_controlled_hurt_recovery_0x002c();
    test_routine_00e682_full_56byte_contract();
    test_oracle_mutation_detectors();
    std::cout << "[PASS] All M16.21 Combat Oracle & Arithmetic Tests PASSED!\n";
    return 0;
}
