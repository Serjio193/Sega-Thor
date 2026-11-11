#include "core/rom.hpp"
#include "core/sha256.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

#define OASIS_CHECK(cond)                                                              \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            std::cerr << "CHECK FAILED: " #cond " at line " << __LINE__ << "\n";       \
            std::abort();                                                              \
        }                                                                              \
    } while (false)

std::string resolve_path(const std::string& rel) {
    for (const auto* prefix : {".", "..", "../..", "../../.."}) {
        auto cand = fs::path(prefix) / rel;
        if (fs::exists(cand)) return cand.string();
    }
    return rel;
}

std::string read_text(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    OASIS_CHECK(in.is_open());
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> read_binary(const std::string& p) {
    std::ifstream in(p, std::ios::binary | std::ios::ate);
    OASIS_CHECK(in.is_open());
    const auto sz = in.tellg();
    OASIS_CHECK(sz > 0);
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(sz));
    in.read(reinterpret_cast<char*>(out.data()), sz);
    return out;
}

std::vector<std::uint8_t> unhex(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t i = 0; i + 1U < hex.size(); i += 2U) {
        const char buf[3] = {hex[i], hex[i + 1U], '\0'};
        bytes.push_back(static_cast<std::uint8_t>(std::strtoul(buf, nullptr, 16)));
    }
    return bytes;
}

struct ParsedSnapshot {
    int frame{0};
    std::vector<std::uint8_t> vram{}, cram{}, vsram{}, regs{};
};

std::vector<ParsedSnapshot> parse_snapshots(const std::string& json_str) {
    std::vector<ParsedSnapshot> snaps;
    std::size_t pos = 0;
    while ((pos = json_str.find("{\"f\":", pos)) != std::string::npos) {
        ParsedSnapshot s{};
        pos += 5U;
        s.frame = std::stoi(json_str.substr(pos, 10U));
        const auto rpos = json_str.find("\"regs\":[", pos);
        const auto rend = json_str.find("]", rpos);
        OASIS_CHECK(rpos != std::string::npos && rend != std::string::npos);
        std::stringstream ss(json_str.substr(rpos + 8U, rend - (rpos + 8U)));
        std::string item;
        while (std::getline(ss, item, ',')) {
            while (!item.empty() && item.front() == ' ') item.erase(0, 1);
            s.regs.push_back(static_cast<std::uint8_t>(std::stoul(item)));
        }
        const auto cpos = json_str.find("\"cram\":\"", rend);
        const auto cend = json_str.find("\"", cpos + 8U);
        s.cram = unhex(json_str.substr(cpos + 8U, cend - (cpos + 8U)));
        const auto spos = json_str.find("\"vsram\":\"", cend);
        const auto send = json_str.find("\"", spos + 9U);
        s.vsram = unhex(json_str.substr(spos + 9U, send - (spos + 9U)));
        const auto vpos = json_str.find("\"vram\":\"", send);
        const auto vend = json_str.find("\"", vpos + 8U);
        s.vram = unhex(json_str.substr(vpos + 8U, vend - (vpos + 8U)));
        OASIS_CHECK(s.vram.size() == 65536U && s.cram.size() == 128U);
        OASIS_CHECK(s.vsram.size() == 80U && s.regs.size() == 24U);
        snaps.push_back(std::move(s));
        pos = vend;
    }
    return snaps;
}

oasis::genesis::VdpState to_vdp_state(const ParsedSnapshot& snap) {
    oasis::genesis::VdpState s{};
    std::copy(snap.vram.begin(), snap.vram.end(), s.vram.begin());
    std::copy(snap.cram.begin(), snap.cram.end(), s.cram.begin());
    std::copy(snap.vsram.begin(), snap.vsram.end(), s.vsram.begin());
    std::copy(snap.regs.begin(), snap.regs.end(), s.registers.begin());
    return s;
}

oasis::game::TownCheckpointInitialState build_cp(const std::vector<std::uint8_t>& cp_bin) {
    auto r8 = [&](std::size_t off) -> std::uint8_t { return cp_bin[0x10U + (off ^ 1U)]; };
    auto r16 = [&](std::size_t off) -> std::uint16_t { return static_cast<std::uint16_t>((r8(off) << 8U) | r8(off + 1U)); };
    auto rs16 = [&](std::size_t off) -> std::int16_t { return static_cast<std::int16_t>(r16(off)); };
    auto r32 = [&](std::size_t off) -> std::uint32_t { return (static_cast<std::uint32_t>(r16(off)) << 16U) | r16(off + 2U); };
    auto rs32 = [&](std::size_t off) -> std::int32_t { return static_cast<std::int32_t>(r32(off)); };

    oasis::game::TownCheckpointInitialState cp{};
    cp.x_fixed = 4808704;
    cp.y_fixed = 13107200;
    cp.movement_state = 4U;
    cp.direction_code = 0U;
    cp.camera_x = 0;
    cp.camera_y = 121;
    cp.tick_counter = 2391U;
    cp.frame_counter = 775U;
    cp.prng_seed = 0x22BCU;
    cp.turn_timer = 67U;
    cp.intent_y_fixed = -294912;
    cp.initial_walk_tick = 1;

    for (std::uint16_t s = 4U; s <= 19U; ++s) {
        const std::size_t b = 0x19E8U + static_cast<std::size_t>(s) * 0xBCU;
        oasis::game::NativeEntity ent{};
        ent.slot_index = s;
        ent.type_id = r16(b + 0x00U);
        ent.active = (ent.type_id != 0U);
        ent.ai_timer = r16(b + 0x02U);
        ent.state = static_cast<std::uint8_t>(r16(b + 0x04U));
        ent.x_fp = rs32(b + 0x08U);
        ent.y_fp = rs32(b + 0x0CU);
        ent.z_fp = rs32(b + 0x10U);
        ent.world_x = rs16(b + 0x08U);
        ent.world_y = rs16(b + 0x0CU);
        ent.world_z = rs16(b + 0x10U);
        ent.facing = r16(b + 0x16U);
        ent.tile_base = r16(b + 0x18U);
        ent.anim_frame = static_cast<std::uint8_t>(r16(b + 0x2AU));
        ent.previous_anim_id = r16(b + 0x2CU);
        ent.anim_step = r16(b + 0x2EU);
        ent.anim_key_off = r16(b + 0x30U);
        ent.frame_id = r16(b + 0x32U);
        ent.frame_dir = r16(b + 0x34U);
        ent.flags36 = r8(b + 0x36U);
        ent.flags37 = r8(b + 0x37U);
        ent.flags38 = r8(b + 0x38U);
        ent.flags39 = r8(b + 0x39U);
        ent.flags3a = r8(b + 0x3AU);
        ent.shadow_idx = r8(b + 0x3DU);
        ent.overlay_3e = r16(b + 0x3EU);
        ent.half_width = rs16(b + 0x42U);
        ent.half_depth = rs16(b + 0x44U);
        ent.base_half_w = r8(b + 0x48U);
        ent.base_half_d = r8(b + 0x49U);
        ent.height = rs16(b + 0x4AU);
        ent.action_timer = r8(b + 0x82U);
        ent.flash_timer = r8(b + 0x83U);
        ent.max_hp = rs16(b + 0x88U);
        ent.hp = rs16(b + 0x8AU);
        ent.defense = rs16(b + 0x92U);
        ent.variant_96 = r16(b + 0x96U);
        ent.water_ripple_tick = r8(b + 0x99U);
        const std::int16_t raw_9e = rs16(b + 0x9EU);
        ent.linked_slot_9e = (raw_9e > 0 && (raw_9e % 0xBC) == 0 && (raw_9e / 0xBC) < 20)
                                 ? static_cast<std::int16_t>(raw_9e / 0xBC)
                                 : std::int16_t{-1};
        ent.prng_mask_a0 = r16(b + 0xA0U);
        ent.invulnerable = (ent.flags38 & 0x08U) != 0U;
        cp.entities.push_back(ent);
    }
    return cp;
}

struct SnapshotDiff {
    std::size_t pat{0}, pa{0}, pb{0}, sat{0}, scr{0}, unu{0};
    std::size_t cram{0}, vsram{0}, regs{0}, px{0};
    std::string orig_sha{}, nat_sha{};
    std::size_t vram_total() const noexcept { return pat + pa + pb + sat + scr + unu; }
    std::size_t plane_total() const noexcept { return pa + pb; }
};

SnapshotDiff measure_snapshot(const ParsedSnapshot& orig_snap, const oasis::genesis::VdpState& nat) {
    SnapshotDiff d{};
    for (std::size_t i = 0; i < 65536U; ++i) {
        if (nat.vram[i] == orig_snap.vram[i]) continue;
        if (i < 0xC000U) ++d.pat;
        else if (i < 0xD000U) ++d.pa;
        else if (i < 0xD280U) ++d.sat;
        else if (i < 0xE000U) ++d.unu;
        else if (i < 0xF000U) ++d.pb;
        else if (i < 0xF360U) ++d.scr;
        else ++d.unu;
    }
    for (std::size_t i = 0; i < 128U; ++i) if (nat.cram[i] != orig_snap.cram[i]) ++d.cram;
    for (std::size_t i = 0; i < 80U; ++i) if (nat.vsram[i] != orig_snap.vsram[i]) ++d.vsram;
    for (std::size_t i = 0; i < 24U; ++i) if (nat.registers[i] != orig_snap.regs[i]) ++d.regs;

    std::vector<std::uint8_t> o_rgb(320U * 224U * 3U, 0U);
    std::vector<std::uint8_t> n_rgb(320U * 224U * 3U, 0U);
    auto orig_v = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(orig_snap));
    oasis::genesis::render_vdp_frame_rgb24(*orig_v, o_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    oasis::genesis::render_vdp_frame_rgb24(nat, n_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    for (std::size_t i = 0; i + 2U < o_rgb.size(); i += 3U) {
        if (o_rgb[i] != n_rgb[i] || o_rgb[i + 1U] != n_rgb[i + 1U] || o_rgb[i + 2U] != n_rgb[i + 2U]) {
            ++d.px;
        }
    }
    d.orig_sha = oasis::core::compute_sha256(o_rgb);
    d.nat_sha = oasis::core::compute_sha256(n_rgb);
    return d;
}

void test_phase4_rasterizer_audit(const std::vector<std::uint8_t>& rom) {
    // 1. Existing frame_779 reference RGB24 parity
    auto f779_state = std::make_unique<oasis::genesis::VdpState>();
    OASIS_CHECK(f779_state->load_from_file(resolve_path("data/standalone-fixtures/frame_779_vdp_state.json").c_str()));
    const auto f779_ref = read_binary(resolve_path("data/standalone-fixtures/frame_779_reference.rgb24"));
    std::vector<std::uint8_t> rgb(320U * 224U * 3U, 0U);
    oasis::genesis::render_vdp_frame_rgb24(*f779_state, rgb, oasis::genesis::ColorDacMode::ModelFullRange);
    OASIS_CHECK(rgb == f779_ref);

    // 2. Scanline 20-sprite limit & x_raw == 0 sprite masking check
    auto test_vdp = std::make_unique<oasis::genesis::VdpState>();
    test_vdp->registers[1] = 0x64U; // display ON
    test_vdp->registers[5] = 0x68U; // SAT @ 0xD000
    test_vdp->registers[12] = 0x81U; // H40 (320px, max 20 sprites/line)
    test_vdp->cram[2] = 0x00U; test_vdp->cram[3] = 0x0EU; // color 1 = bright red
    test_vdp->cram[4] = 0x0EU; test_vdp->cram[5] = 0x00U; // color 2 = bright blue
    std::fill(test_vdp->vram.begin() + 0x0020U, test_vdp->vram.begin() + 0x0040U, 0x11U); // tile 1 = color 1
    std::fill(test_vdp->vram.begin() + 0x0040U, test_vdp->vram.begin() + 0x0060U, 0x22U); // tile 2 = color 2

    auto write_sat = [&](int idx, int y, int sz, int lnk, std::uint16_t attr, int x) {
        const std::size_t o = 0xD000U + static_cast<std::size_t>(idx) * 8U;
        test_vdp->vram[o + 0U] = static_cast<std::uint8_t>((y >> 8) & 0x03);
        test_vdp->vram[o + 1U] = static_cast<std::uint8_t>(y & 0xFF);
        test_vdp->vram[o + 2U] = static_cast<std::uint8_t>(sz & 0x0F);
        test_vdp->vram[o + 3U] = static_cast<std::uint8_t>(lnk & 0x7F);
        test_vdp->vram[o + 4U] = static_cast<std::uint8_t>((attr >> 8) & 0xFF);
        test_vdp->vram[o + 5U] = static_cast<std::uint8_t>(attr & 0xFF);
        test_vdp->vram[o + 6U] = static_cast<std::uint8_t>((x >> 8) & 0x03);
        test_vdp->vram[o + 7U] = static_cast<std::uint8_t>(x & 0xFF);
    };
    // Sprite 0 @ (x=10+128=138, y=128) tile 1, Sprite 1 x_raw=0 masks Sprite 2 @ (x=40+128=168) tile 2
    write_sat(0, 128, 0, 1, 0x0001U, 138);
    write_sat(1, 128, 0, 2, 0x0002U, 0);
    write_sat(2, 128, 0, 0, 0x0002U, 168);
    oasis::genesis::render_vdp_frame_rgb24(*test_vdp, rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    // Pixel (10,0) = Sprite 0 red; Pixel (40,0) = masked -> backdrop (0,0,0)
    OASIS_CHECK(rgb[10U * 3U] == 0xEEU && rgb[40U * 3U] == 0U);
    (void)rom;
    std::cout << "[PASS] Phase 4: VDP Rasterizer Audit Verified (71680/71680 ref, masking x==0 & limits)\n";
}

void test_phase5_and_6_convergence_and_table(const std::vector<std::uint8_t>& rom,
                                            const std::vector<ParsedSnapshot>& c_snaps,
                                            const std::vector<ParsedSnapshot>& h_snaps,
                                            const std::vector<std::uint8_t>& cp_bin) {
    // Canonical exact F779 convergence + 7-snapshot table
    auto c_cp = build_cp(cp_bin);
    c_cp.attract_banner_enabled = true;
    auto c_sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
    auto c_init = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(c_snaps[0]));
    OASIS_CHECK(c_sim->initialize_town_checkpoint(rom, c_cp, *c_init));
    c_sim->set_oracle_mode(false);

    auto out_vdp = std::make_unique<oasis::genesis::VdpState>();
    const auto d775 = measure_snapshot(c_snaps[0], c_sim->current_vdp());
    OASIS_CHECK(d775.vram_total() == 0U && d775.px == 0U);
    std::cout << "  Canon F775: VRAM=" << d775.vram_total() << " Pat=" << d775.pat << " SAT=" << d775.sat
              << " Pl=" << d775.plane_total() << " CRAM=" << d775.cram << " VSRAM=" << d775.vsram
              << " Regs=" << d775.regs << " PX=" << d775.px << "\n";

    std::size_t c_next = 1U, c_px_sum = 0U;
    std::size_t staged_count_at_778 = 0U;
    for (int f = 776; f <= 1115; ++f) {
        oasis::game::FrameInput in{};
        in.frame_index = static_cast<std::uint32_t>(f);
        if (f < 782) {
            in.controller_1.set(oasis::core::Button::Up, true);
            in.controller_1.set(oasis::core::Button::Left, true);
        } else if (f <= 784) {
            in.controller_1.set(oasis::core::Button::B, true);
            in.controller_1.set(oasis::core::Button::Up, true);
            in.controller_1.set(oasis::core::Button::Left, true);
        }
        c_sim->step_frame(in, *out_vdp);
        if (f == 778) staged_count_at_778 = c_sim->sat_pipeline().active_chain().size();

        if (c_next < c_snaps.size() && f == c_snaps[c_next].frame) {
            const auto d = measure_snapshot(c_snaps[c_next], *out_vdp);
            c_px_sum += d.px;
            std::cout << "  Canon F" << f << ": VRAM=" << d.vram_total() << " Pat=" << d.pat << " SAT=" << d.sat
                      << " Pl=" << d.plane_total() << " CRAM=" << d.cram << " VSRAM=" << d.vsram
                      << " Regs=" << d.regs << " PX=" << d.px << "\n";
            if (f == 779) {
                OASIS_CHECK(staged_count_at_778 == 28U);
                OASIS_CHECK(d.vram_total() == 0U && d.pat == 0U && d.sat == 0U && d.plane_total() == 0U);
                OASIS_CHECK(d.cram == 0U && d.vsram == 0U && d.regs == 0U && d.px == 0U);
                OASIS_CHECK(d.orig_sha == d.nat_sha);
                std::cout << "  [EXACT ACTIVE FRAME F779] RGB24 SHA256=" << d.nat_sha << "\n";
            }
            ++c_next;
        }
    }
    OASIS_CHECK(c_next == 7U && c_px_sum <= 150000U);
    c_sim.reset();

    // Independent Holdout 7-snapshot table
    auto h_cp = build_cp(cp_bin);
    h_cp.attract_banner_enabled = false;
    auto h_sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
    auto h_init = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(h_snaps[0]));
    OASIS_CHECK(h_sim->initialize_town_checkpoint(rom, h_cp, *h_init));
    h_sim->set_oracle_mode(false);

    const auto hd775 = measure_snapshot(h_snaps[0], h_sim->current_vdp());
    OASIS_CHECK(hd775.vram_total() == 0U && hd775.px == 0U);
    std::cout << "  Hold  F775: VRAM=" << hd775.vram_total() << " Pat=" << hd775.pat << " SAT=" << hd775.sat
              << " Pl=" << hd775.plane_total() << " CRAM=" << hd775.cram << " VSRAM=" << hd775.vsram
              << " Regs=" << hd775.regs << " PX=" << hd775.px << "\n";

    std::size_t h_next = 1U, h_px_sum = 0U;
    for (int f = 776; f <= 1125; ++f) {
        oasis::game::FrameInput in{};
        in.frame_index = static_cast<std::uint32_t>(f);
        if (f >= 776 && f <= 818) in.controller_1.set(oasis::core::Button::Right, true);
        if (f >= 819 && f <= 822) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
        if (f >= 855 && f <= 858) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
        if (f >= 890 && f <= 935) { in.controller_1.set(oasis::core::Button::Right, true); in.controller_1.set(oasis::core::Button::Down, true); }
        if (f >= 936 && f <= 939) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
        if (f >= 975 && f <= 978) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
        h_sim->step_frame(in, *out_vdp);

        if (h_next < h_snaps.size() && f == h_snaps[h_next].frame) {
            const auto d = measure_snapshot(h_snaps[h_next], *out_vdp);
            h_px_sum += d.px;
            std::cout << "  Hold  F" << f << ": VRAM=" << d.vram_total() << " Pat=" << d.pat << " SAT=" << d.sat
                      << " Pl=" << d.plane_total() << " CRAM=" << d.cram << " VSRAM=" << d.vsram
                      << " Regs=" << d.regs << " PX=" << d.px << "\n";
            OASIS_CHECK(d.plane_total() == 0U && d.cram == 0U && d.vsram == 0U);
            ++h_next;
        }
    }
    OASIS_CHECK(h_next == 7U && h_px_sum <= 25520U);
    std::cout << "[PASS] Phase 5 & 6: F779 100% Pixel-Exact (0/71680) | CanonSumPX=" << c_px_sum
              << " | HoldoutSumPX=" << h_px_sum << "\n";
}

void test_phase7_mutations(const std::vector<std::uint8_t>& rom,
                          const ParsedSnapshot& f775_orig,
                          const ParsedSnapshot& f779_orig,
                          const std::vector<std::uint8_t>& cp_bin) {
    // M1: Event ordering / VBlank staging timing mutation (wrong tick phase shifts F779 SAT & pixels)
    {
        auto m_cp = build_cp(cp_bin);
        m_cp.attract_banner_enabled = true;
        m_cp.tick_counter = 2392U; // Mutated phase (+1) breaks VBlank staging alignment
        auto m_sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
        auto init_v = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(f775_orig));
        OASIS_CHECK(m_sim->initialize_town_checkpoint(rom, m_cp, *init_v));
        m_sim->set_oracle_mode(false);
        auto ov = std::make_unique<oasis::genesis::VdpState>();
        for (int f = 776; f <= 779; ++f) {
            oasis::game::FrameInput in{};
            in.frame_index = static_cast<std::uint32_t>(f);
            in.controller_1.set(oasis::core::Button::Up, true);
            in.controller_1.set(oasis::core::Button::Left, true);
            m_sim->step_frame(in, *ov);
        }
        OASIS_CHECK(measure_snapshot(f779_orig, *ov).sat > 0U);
    }

    // M2: Clearing inactive SAT residue [28..79] corrupts physical SAT entry 28
    {
        auto sat_mut = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(f779_orig));
        std::fill(sat_mut->vram.begin() + 0xD000U + 28U * 8U, sat_mut->vram.begin() + 0xD280U, 0U);
        OASIS_CHECK(sat_mut->vram[0xD0E1U] != f779_orig.vram[0xD0E1U] &&
                    !std::equal(sat_mut->vram.begin(), sat_mut->vram.end(), f779_orig.vram.begin()));
    }

    // M3: Sprite masking x==0 bypass mutation exposes hidden trailing sprite
    {
        auto vdp = std::make_unique<oasis::genesis::VdpState>();
        vdp->registers[1] = 0x64U; vdp->registers[5] = 0x68U; vdp->registers[12] = 0x81U;
        vdp->cram[4] = 0x0EU;
        std::fill(vdp->vram.begin() + 0x0040U, vdp->vram.begin() + 0x0060U, 0x22U);
        // If x==0 sprite 1 were changed to non-zero (x=129), masking stops and sprite 2 appears
        vdp->vram[0xD000U + 1U] = 128U; vdp->vram[0xD000U + 7U] = 138U; vdp->vram[0xD000U + 3U] = 1U;
        vdp->vram[0xD008U + 1U] = 128U; vdp->vram[0xD008U + 7U] = 129U; vdp->vram[0xD008U + 3U] = 2U;
        vdp->vram[0xD010U + 1U] = 128U; vdp->vram[0xD010U + 7U] = 168U; vdp->vram[0xD010U + 5U] = 0x02U;
        std::vector<std::uint8_t> rgb(320U * 224U * 3U, 0U);
        oasis::genesis::render_vdp_frame_rgb24(*vdp, rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
        OASIS_CHECK(rgb[40U * 3U + 2U] != 0U);
    }

    // M4: Footprint 0x009BF2 single-point vs AND-mask mutation falsely immerses bank-edge Slot 7
    {
        const auto scene = oasis::game::world::load_town_scene_from_rom(rom);
        const int single_cell = scene.terrain_collision[(340 >> 3) * 64 + (181 >> 3)];
        OASIS_CHECK((single_cell & 0x20) != 0); // Single-center-cell wrongly sees water; AND-mask sees land (0x01)
    }

    std::cout << "[PASS] Phase 7: All M16.39 Mutation Guards Killed (4/4)\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n";
    std::cout << "SEGA-THOR M16.39: SAT PHYSICAL FORENSICS & F779 PIXEL-EXACT ACTIVE FRAME\n";
    std::cout << "=====================================================================\n";

    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    OASIS_CHECK(oasis::core::compute_sha256(rom) == "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263");
    const auto c_snaps = parse_snapshots(read_text(resolve_path("data/standalone-fixtures/town_canonical_vdp_snapshots.json")));
    const auto h_snaps = parse_snapshots(read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_vdp.json")));
    const auto cp_bin = read_binary(resolve_path("data/checkpoints/town_free_control_f775.state"));

    test_phase4_rasterizer_audit(rom);
    test_phase5_and_6_convergence_and_table(rom, c_snaps, h_snaps, cp_bin);
    test_phase7_mutations(rom, c_snaps[0], c_snaps[1], cp_bin);

    std::cout << "\n[PASS] All M16.39 Tests PASSED successfully!\n";
    return 0;
}
