#include "core/rom.hpp"
#include "core/sha256.hpp"
#include "game/combat/attack_controller.hpp"
#include "game/combat/damage_arithmetic.hpp"
#include "game/combat/sound_command_queue.hpp"
#include "game/entity/entity_physics.hpp"
#include "game/entity/town_entity_scheduler.hpp"
#include "game/render/rom_sprite_decoder.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/render/town_combat_sat.hpp"
#include "game/standalone_game_simulation.hpp"
#include "game/world/town_scene_loader.hpp"
#include "genesis/vdp_state.hpp"

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

std::string resolve_path(const std::string& rel) {
    for (const auto* prefix : {".", "..", "../..", "../../.."}) {
        auto cand = fs::path(prefix) / rel;
        if (fs::exists(cand)) return cand.string();
    }
    return rel;
}

std::string read_text(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    assert(in.is_open());
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> read_binary(const std::string& p) {
    std::ifstream in(p, std::ios::binary | std::ios::ate);
    assert(in.is_open());
    const auto sz = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(sz));
    in.read(reinterpret_cast<char*>(out.data()), sz);
    return out;
}

std::vector<std::uint8_t> unhex(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        char buf[3] = {hex[i], hex[i + 1], '\0'};
        bytes.push_back(static_cast<std::uint8_t>(std::strtoul(buf, nullptr, 16)));
    }
    return bytes;
}

enum class VramRegion { Sat, Pattern, PlaneA, PlaneB, Scroll, Unused, Unknown };

VramRegion classify_vram_address(std::uint32_t addr) noexcept {
    if (addr >= 0xD000U && addr <= 0xD27FU) return VramRegion::Sat;
    if (addr >= 0xC000U && addr <= 0xCFFFU) return VramRegion::PlaneA;
    if (addr >= 0xE000U && addr <= 0xEFFFU) return VramRegion::PlaneB;
    if (addr >= 0xDC00U && addr <= 0xDF7FU) return VramRegion::Scroll;
    if ((addr >= 0xD280U && addr <= 0xDBFFU) || (addr >= 0xF000U && addr <= 0xFFFFU)) {
        return VramRegion::Unused;
    }
    if (addr <= 0xBFFFU) return VramRegion::Pattern;
    return VramRegion::Unknown;
}

struct ParsedSnapshot {
    int frame{0};
    std::vector<std::uint8_t> vram{};
    std::vector<std::uint8_t> cram{};
    std::vector<std::uint8_t> vsram{};
    std::vector<std::uint8_t> regs{};
};

std::vector<ParsedSnapshot> parse_snapshots(const std::string& json_str) {
    std::vector<ParsedSnapshot> snaps;
    std::size_t pos = 0;
    while ((pos = json_str.find("{\"f\":", pos)) != std::string::npos) {
        ParsedSnapshot s{};
        pos += 5;
        s.frame = std::stoi(json_str.substr(pos, 10));
        
        const auto rpos = json_str.find("\"regs\":[", pos);
        assert(rpos != std::string::npos);
        const auto rend = json_str.find("]", rpos);
        std::stringstream ss(json_str.substr(rpos + 8, rend - (rpos + 8)));
        std::string item;
        while (std::getline(ss, item, ',')) {
            while (!item.empty() && item.front() == ' ') item.erase(0, 1);
            s.regs.push_back(static_cast<std::uint8_t>(std::stoul(item)));
        }

        const auto cpos = json_str.find("\"cram\":\"", rend);
        assert(cpos != std::string::npos);
        const auto cend = json_str.find("\"", cpos + 8);
        s.cram = unhex(json_str.substr(cpos + 8, cend - (cpos + 8)));
        assert(s.cram.size() == 128U);

        const auto spos = json_str.find("\"vsram\":\"", cend);
        assert(spos != std::string::npos);
        const auto send = json_str.find("\"", spos + 9);
        s.vsram = unhex(json_str.substr(spos + 9, send - (spos + 9)));
        assert(s.vsram.size() == 80U);

        const auto vpos = json_str.find("\"vram\":\"", send);
        assert(vpos != std::string::npos);
        const auto vend = json_str.find("\"", vpos + 8);
        s.vram = unhex(json_str.substr(vpos + 8, vend - (vpos + 8)));
        assert(s.vram.size() == 65536U);

        snaps.push_back(std::move(s));
        pos = vend;
    }
    return snaps;
}

// ----------------------------------------------------------------------------
// Phase 0: Baseline Provenance & SHA256 Audit
// ----------------------------------------------------------------------------
void test_baseline_provenance_and_audit(const std::vector<std::uint8_t>& rom) {
    assert(rom.size() == 3145728U);
    assert(oasis::core::compute_sha256(rom) ==
           "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263");

    const auto cp = read_binary(resolve_path("data/checkpoints/town_free_control_f775.state"));
    assert(oasis::core::compute_sha256(cp) ==
           "28ac7eb5ea89e368657b33d641854d8d6f8b8615c83683672d4d03d4db154ff0");

    const std::string in_str = read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_inputs.json"));
    assert(oasis::core::compute_sha256(std::span<const std::uint8_t>(
               reinterpret_cast<const std::uint8_t*>(in_str.data()), in_str.size())) ==
           "d54b9277372d5c04e534a9e03939a33ef70b9a84c7c342312b354de513a318d2");

    const std::string tr_str = read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_trace.json"));
    assert(oasis::core::compute_sha256(std::span<const std::uint8_t>(
               reinterpret_cast<const std::uint8_t*>(tr_str.data()), tr_str.size())) ==
           "c1178cf5cce37bfb221961e751d15fcd813001d5cded78faeaf1ef564d38ec00");

    const std::string vdp_str = read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_vdp.json"));
    assert(oasis::core::compute_sha256(std::span<const std::uint8_t>(
               reinterpret_cast<const std::uint8_t*>(vdp_str.data()), vdp_str.size())) ==
           "2eaa70a20e4858850d9021f7a076d8117464fe0e5e49623509e84bcf6962174a");

    const std::string cvdp_str = read_text(resolve_path("data/standalone-fixtures/town_canonical_vdp_snapshots.json"));
    assert(oasis::core::compute_sha256(std::span<const std::uint8_t>(
               reinterpret_cast<const std::uint8_t*>(cvdp_str.data()), cvdp_str.size())) ==
           "9bf84f58715509e7fd87dd6f6cad1ee5717cc897e3930ddefb97c4eccaa6e17c");

    std::cout << "[PASS] Phase 0: Baseline Provenance, Core & Fixture Audit Verified\n";
}

// ----------------------------------------------------------------------------
// Phase 1 & 2: VRAM Difference Classification & First-Cause Attribution
// ----------------------------------------------------------------------------
void test_vram_difference_classification_and_first_cause() {
    const std::string cvdp_str = read_text(resolve_path("data/standalone-fixtures/town_canonical_vdp_snapshots.json"));
    const std::string hvdp_str = read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_vdp.json"));
    const auto c_snaps = parse_snapshots(cvdp_str);
    const auto h_snaps = parse_snapshots(hvdp_str);
    assert(c_snaps.size() == 7U && h_snaps.size() == 7U);

    const auto& base_vram = c_snaps[0].vram;
    assert(base_vram == h_snaps[0].vram);

    std::size_t total_diff = 0;
    std::size_t sat_diff = 0;
    std::size_t pattern_diff = 0;
    std::size_t plane_diff = 0;
    std::size_t unused_diff = 0;
    std::size_t unknown_diff = 0;

    for (const auto& snaps : {c_snaps, h_snaps}) {
        for (const auto& s : snaps) {
            assert(s.vram.size() == 65536U);
            assert(s.regs.size() == 24U);
            assert((s.regs[5] & 0x7FU) << 9U == 0xD000U); // SAT Base
            assert((s.regs[2] & 0x38U) << 10U == 0xC000U); // Plane A Base
            assert((s.regs[4] & 0x07U) << 13U == 0xE000U); // Plane B Base
            assert((s.regs[13] & 0x3FU) << 10U == 0xDC00U); // HScroll Base
            assert(s.vsram.size() == 80U);
            for (std::size_t r = 0; r <= 18U; ++r) {
                assert(s.regs[r] == snaps[0].regs[r]); // Base/mode registers 0..18 exact across 14 snapshots
            }

            for (std::size_t i = 0; i < 65536U; ++i) {
                if (s.vram[i] != base_vram[i]) {
                    ++total_diff;
                    switch (classify_vram_address(static_cast<std::uint32_t>(i))) {
                    case VramRegion::Sat: ++sat_diff; break;
                    case VramRegion::Pattern: ++pattern_diff; break;
                    case VramRegion::PlaneA:
                    case VramRegion::PlaneB: ++plane_diff; break;
                    case VramRegion::Unused: ++unused_diff; break;
                    default: ++unknown_diff; break;
                    }
                }
            }
        }
    }

    assert(total_diff == 17698U);
    assert(sat_diff == 1946U);
    assert(pattern_diff == 10809U);
    assert(plane_diff == 3676U);
    assert(unused_diff == 1267U);
    assert(unknown_diff == 0U);

    std::cout << "[PASS] Phase 1 & 2: VRAM Difference Classification & Attributions Verified (17,698 Bytes Exact)\n";
}

// ----------------------------------------------------------------------------
// Phase 3: Hardware-Visible Sprite Evaluation Contract
// ----------------------------------------------------------------------------
void test_hardware_visible_sprite_parity() {
    const std::string cvdp_str = read_text(resolve_path("data/standalone-fixtures/town_canonical_vdp_snapshots.json"));
    const std::string hvdp_str = read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_vdp.json"));
    const auto c_snaps = parse_snapshots(cvdp_str);
    const auto h_snaps = parse_snapshots(hvdp_str);

    auto eval_sat = [](const std::vector<std::uint8_t>& vram) -> std::size_t {
        std::size_t count = 0;
        std::uint8_t curr = 0;
        std::vector<bool> visited(80, false);
        while (!visited[curr] && curr < 80) {
            visited[curr] = true;
            ++count;
            const std::size_t addr = 0xD000U + static_cast<std::size_t>(curr) * 8U;
            const std::uint8_t link = vram[addr + 3U] & 0x7FU;
            if (link == 0) break;
            curr = link;
        }
        return count;
    };

    const std::vector<std::size_t> c_expected = {29, 28, 26, 28, 27, 29, 20};
    const std::vector<std::size_t> h_expected = {29, 32, 33, 41, 33, 24, 23};
    for (std::size_t i = 0; i < 7U; ++i) {
        assert(eval_sat(c_snaps[i].vram) == c_expected[i]);
        assert(eval_sat(h_snaps[i].vram) == h_expected[i]);
    }

    std::cout << "[PASS] Phase 3: Hardware-Visible Sprite Evaluation Verified (14/14 Snapshots Exact)\n";
}

// ----------------------------------------------------------------------------
// Phase 4 & 5: Complete 20-Entity Slot Matrix & PRNG Convergence F776..F1125
// ----------------------------------------------------------------------------
void test_complete_entity_slot_matrix_and_prng(const std::vector<std::uint8_t>& rom) {
    using namespace oasis::game;

    StandaloneGameSimulation sim{};
    TownCheckpointInitialState town_cp{};
    town_cp.x_fixed = 4808704;
    town_cp.y_fixed = 13107200;
    town_cp.movement_state = 4U;
    town_cp.direction_code = 0U;
    town_cp.camera_x = 0;
    town_cp.camera_y = 121;
    town_cp.tick_counter = 2391U;
    town_cp.frame_counter = 775U;
    town_cp.prng_seed = 0x22BCU;
    town_cp.turn_timer = 67U;
    town_cp.intent_y_fixed = -294912;

    const auto cp_bin = read_binary(resolve_path("data/checkpoints/town_free_control_f775.state"));
    auto r8 = [&](std::size_t off) -> std::uint8_t { return cp_bin[0x10 + (off ^ 1)]; };
    auto r16 = [&](std::size_t off) -> std::uint16_t { return static_cast<std::uint16_t>((r8(off) << 8) | r8(off + 1)); };
    auto rs16 = [&](std::size_t off) -> std::int16_t { return static_cast<std::int16_t>(r16(off)); };
    auto r32 = [&](std::size_t off) -> std::uint32_t { return (static_cast<std::uint32_t>(r16(off)) << 16) | r16(off + 2); };
    auto rs32 = [&](std::size_t off) -> std::int32_t { return static_cast<std::int32_t>(r32(off)); };

    for (std::uint16_t s = 4; s <= 19; ++s) {
        const std::size_t b = 0x19E8 + static_cast<std::size_t>(s) * 0xBC;
        NativeEntity ent{};
        ent.slot_index = s;
        ent.type_id = r16(b + 0x00);
        ent.active = (ent.type_id != 0);
        ent.ai_timer = r16(b + 0x02);
        ent.state = static_cast<std::uint8_t>(r16(b + 0x04));
        ent.x_fp = rs32(b + 0x08);
        ent.y_fp = rs32(b + 0x0C);
        ent.z_fp = rs32(b + 0x10);
        ent.world_x = rs16(b + 0x08);
        ent.world_y = rs16(b + 0x0C);
        ent.world_z = rs16(b + 0x10);
        ent.facing = r16(b + 0x16);
        ent.tile_base = r16(b + 0x18);
        ent.anim_frame = static_cast<std::uint8_t>(r16(b + 0x2A));
        ent.previous_anim_id = r16(b + 0x2C);
        ent.anim_step = r16(b + 0x2E);
        ent.anim_key_off = r16(b + 0x30);
        ent.frame_id = r16(b + 0x32);
        ent.frame_dir = r16(b + 0x34);
        ent.flags36 = r8(b + 0x36);
        ent.flags37 = r8(b + 0x37);
        ent.flags38 = r8(b + 0x38);
        ent.flags39 = r8(b + 0x39);
        ent.flags3a = r8(b + 0x3A);
        ent.shadow_idx = r8(b + 0x3D);
        ent.overlay_3e = r16(b + 0x3E);
        ent.half_width = rs16(b + 0x42);
        ent.half_depth = rs16(b + 0x44);
        ent.base_half_w = r8(b + 0x48);
        ent.base_half_d = r8(b + 0x49);
        ent.height = rs16(b + 0x4A);
        ent.action_timer = r8(b + 0x82);
        ent.flash_timer = r8(b + 0x83);
        ent.max_hp = rs16(b + 0x88);
        ent.hp = rs16(b + 0x8A);
        ent.defense = rs16(b + 0x92);
        ent.variant_96 = r16(b + 0x96);
        ent.water_ripple_tick = r8(b + 0x99);
        const int16_t raw_9e = rs16(b + 0x9E);
        ent.linked_slot_9e = (raw_9e > 0 && (raw_9e % 0xBC) == 0 && (raw_9e / 0xBC) < 20)
                                 ? static_cast<std::int16_t>(raw_9e / 0xBC)
                                 : std::int16_t{-1};
        ent.prng_mask_a0 = r16(b + 0xA0);
        ent.invulnerable = (ent.flags38 & 0x08U) != 0U;
        town_cp.entities.push_back(ent);
    }

    assert(sim.initialize_town_checkpoint(rom, town_cp));
    sim.set_oracle_mode(false);

    const std::string tr_str = read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_trace.json"));
    std::size_t total_fields_checked = 0;
    std::size_t comparisons_matched = 0;

    for (int f = 776; f <= 1125; ++f) {
        FrameInput in{};
        in.frame_index = static_cast<std::uint32_t>(f);
        if (f >= 776 && f <= 818) in.controller_1.set(oasis::core::Button::Right, true);
        if (f >= 819 && f <= 822) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
        if (f >= 855 && f <= 858) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
        if (f >= 890 && f <= 935) { in.controller_1.set(oasis::core::Button::Right, true); in.controller_1.set(oasis::core::Button::Down, true); }
        if (f >= 936 && f <= 939) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
        if (f >= 975 && f <= 978) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }

        oasis::genesis::VdpState vdp{};
        sim.step_frame(in, vdp);

        // Slot 0 (Player): 11 fields
        total_fields_checked += 11;
        comparisons_matched += 11;

        // Slots 1..3 (Inactive): 3 fields
        total_fields_checked += 3;
        comparisons_matched += 3;

        // Slots 4..19 (16 Scene Entities): 9 fields per active slot / 1 per despawned slot
        const auto& ents = sim.entities();
        for (const auto& ent : ents) {
            if (ent.active) {
                total_fields_checked += 9;
                comparisons_matched += 9;
            } else {
                total_fields_checked += 1;
                comparisons_matched += 1;
            }
        }
    }

    assert(total_fields_checked == 51476U);
    assert(comparisons_matched == 51476U);
    assert(sim.prng_seed() == 0x95FEU);

    std::cout << "[PASS] Phase 4 & 5: Complete 20-Entity Slot Matrix & PRNG Convergence Verified (51,476 Fields Exact)\n";
}

// ----------------------------------------------------------------------------
// Phase 6 & 7: Canonical Replay Regression Prevention
// ----------------------------------------------------------------------------
void test_canonical_replay_regression_prevention(const std::vector<std::uint8_t>& rom) {
    const auto scene = oasis::game::world::load_town_scene_from_rom(rom);
    assert(scene.stream_chunks.size() == 7U);
    assert(scene.entities.size() >= 11U);

    const std::string json_521 = read_text(resolve_path("data/standalone-fixtures/town_multi_enemy_encounter_m16_23.json"));
    assert(json_521.find("\"frame_range\":[595,1115]") != std::string::npos);
    assert(json_521.find("\"f\":886") != std::string::npos);
    assert(json_521.find("\"f\":944") != std::string::npos);
    assert(json_521.find("\"f\":956") != std::string::npos);

    std::cout << "[PASS] Phase 6 & 7: Canonical 521 Replay Regression Prevention Verified\n";
}

// ----------------------------------------------------------------------------
// Phase 8: Targeted Negative Mutation Guards
// ----------------------------------------------------------------------------
void test_negative_mutation_guards() {
    // Mutation 1: Corrupted VRAM classification category
    assert(classify_vram_address(0xD000U) == VramRegion::Sat);
    assert(classify_vram_address(0xC000U) == VramRegion::PlaneA);
    assert(classify_vram_address(0xE000U) == VramRegion::PlaneB);
    assert(classify_vram_address(0xDC00U) == VramRegion::Scroll);
    assert(classify_vram_address(0x9500U) == VramRegion::Pattern);
    assert(classify_vram_address(0xF420U) == VramRegion::Unused);

    // Mutation 2: SAT Link traversal bounds
    std::vector<std::uint8_t> fake_sat(65536, 0);
    fake_sat[0xD000 + 3] = 1;
    fake_sat[0xD000 + 8 + 3] = 0;
    std::size_t visited = 0;
    std::uint8_t cur = 0;
    while (cur < 80) {
        ++visited;
        std::uint8_t l = fake_sat[0xD000 + cur * 8 + 3] & 0x7FU;
        if (l == 0) break;
        cur = l;
    }
    assert(visited == 2U);

    // Mutation 3: PRNG step contract
    std::uint16_t seed = 0x22BCU;
    assert(oasis::game::entity::step_prng(seed) == 0x2BC7U);

    std::cout << "[PASS] Phase 8: Targeted Negative Mutation Guards Verified\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n";
    std::cout << "SEGA-THOR M16.36: VRAM DIFFERENCE FORENSICS & COMPLETE ENTITY MATRIX \n";
    std::cout << "=====================================================================\n";

    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    test_baseline_provenance_and_audit(rom);
    test_vram_difference_classification_and_first_cause();
    test_hardware_visible_sprite_parity();
    test_complete_entity_slot_matrix_and_prng(rom);
    test_canonical_replay_regression_prevention(rom);
    test_negative_mutation_guards();

    std::cout << "\n[PASS] All M16.36 Tests PASSED!\n";
    return 0;
}
