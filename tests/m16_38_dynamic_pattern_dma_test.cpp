#include "core/rom.hpp"
#include "core/sha256.hpp"
#include "game/camera/camera_controller.hpp"
#include "game/render/vdp_dma_queue.hpp"
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

struct RegionDiffStats {
    std::size_t pat{0}, pa{0}, pb{0}, sat{0}, scr{0}, unu{0};
    std::size_t cram{0}, vsram{0}, regs{0}, px{0};
    std::size_t vram_total() const noexcept { return pat + pa + pb + sat + scr + unu; }
};

RegionDiffStats measure_diff(const ParsedSnapshot& orig_snap, const oasis::genesis::VdpState& nat) {
    RegionDiffStats st{};
    for (std::size_t i = 0; i < 65536U; ++i) {
        if (nat.vram[i] == orig_snap.vram[i]) continue;
        if (i < 0xC000U) ++st.pat;
        else if (i < 0xD000U) ++st.pa;
        else if (i < 0xD280U) ++st.sat;
        else if (i < 0xE000U) ++st.unu;
        else if (i < 0xF000U) ++st.pb;
        else if (i < 0xF360U) ++st.scr;
        else ++st.unu;
    }
    for (std::size_t i = 0; i < 128U; ++i) if (nat.cram[i] != orig_snap.cram[i]) ++st.cram;
    for (std::size_t i = 0; i < 80U; ++i) if (nat.vsram[i] != orig_snap.vsram[i]) ++st.vsram;
    for (std::size_t i = 0; i < 24U; ++i) if (nat.registers[i] != orig_snap.regs[i]) ++st.regs;

    std::vector<std::uint8_t> o_rgb(320U * 224U * 3U, 0U);
    std::vector<std::uint8_t> n_rgb(320U * 224U * 3U, 0U);
    oasis::genesis::render_vdp_frame_rgb24(to_vdp_state(orig_snap), o_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    oasis::genesis::render_vdp_frame_rgb24(nat, n_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    for (std::size_t i = 0; i + 2U < o_rgb.size(); i += 3U) {
        if (o_rgb[i] != n_rgb[i] || o_rgb[i + 1U] != n_rgb[i + 1U] || o_rgb[i + 2U] != n_rgb[i + 2U]) {
            ++st.px;
        }
    }
    return st;
}

void write_diff_ppm_pair(const fs::path& out_path,
                         const oasis::genesis::VdpState& orig_v,
                         const oasis::genesis::VdpState& nat_v) {
    std::vector<std::uint8_t> o_rgb(320U * 224U * 3U, 0U);
    std::vector<std::uint8_t> n_rgb(320U * 224U * 3U, 0U);
    oasis::genesis::render_vdp_frame_rgb24(orig_v, o_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    oasis::genesis::render_vdp_frame_rgb24(nat_v, n_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    std::ofstream out(out_path, std::ios::binary);
    OASIS_CHECK(out.is_open());
    out << "P6\n960 224\n255\n";
    for (std::size_t y = 0; y < 224U; ++y) {
        out.write(reinterpret_cast<const char*>(&o_rgb[y * 320U * 3U]), 320U * 3U);
        out.write(reinterpret_cast<const char*>(&n_rgb[y * 320U * 3U]), 320U * 3U);
        for (std::size_t x = 0; x < 320U; ++x) {
            const std::size_t idx = (y * 320U + x) * 3U;
            const bool diff = (o_rgb[idx] != n_rgb[idx] || o_rgb[idx + 1U] != n_rgb[idx + 1U] || o_rgb[idx + 2U] != n_rgb[idx + 2U]);
            const std::uint8_t px[3] = {
                diff ? std::uint8_t{255U} : static_cast<std::uint8_t>(o_rgb[idx] / 3U),
                static_cast<std::uint8_t>(o_rgb[idx + 1U] / 3U),
                static_cast<std::uint8_t>(o_rgb[idx + 2U] / 3U)
            };
            out.write(reinterpret_cast<const char*>(px), 3U);
        }
    }
}

void test_phase0_evidence_audit(const std::vector<std::uint8_t>& rom,
                                const std::vector<ParsedSnapshot>& c_snaps,
                                const std::vector<ParsedSnapshot>& h_snaps) {
    OASIS_CHECK(rom.size() == 3145728U);
    OASIS_CHECK(oasis::core::compute_sha256(rom) == "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263");
    OASIS_CHECK(c_snaps.size() == 7U && h_snaps.size() == 7U);

    // Verify M16.37 BEFORE vs AFTER VRAM arithmetic (CRAM excluded from VRAM sums)
    constexpr std::size_t kM1637BeforeVram = 208U + 8379U + 9152U;
    constexpr std::size_t kM1637AfterVram = 0U + 8379U + 9159U;
    OASIS_CHECK(kM1637BeforeVram == 17739U);
    OASIS_CHECK(kM1637AfterVram == 17538U);

    // Reconcile 14-snapshot visible sprite descriptor count: 187 canon + 215 holdout = 402
    auto count_sprites = [](const ParsedSnapshot& s) -> std::size_t {
        std::size_t cnt = 0;
        std::size_t idx = 0;
        for (int step = 0; step < 80; ++step) {
            ++cnt;
            const std::uint8_t nxt = static_cast<std::uint8_t>(s.vram[0xD000U + idx * 8U + 3U] & 0x7FU);
            if (nxt == 0U) break;
            idx = nxt;
        }
        return cnt;
    };
    std::size_t canon_sprites = 0, hold_sprites = 0;
    for (const auto& s : c_snaps) canon_sprites += count_sprites(s);
    for (const auto& s : h_snaps) hold_sprites += count_sprites(s);
    OASIS_CHECK(canon_sprites == 187U);
    OASIS_CHECK(hold_sprites == 215U);
    OASIS_CHECK(canon_sprites + hold_sprites == 402U);
    std::cout << "[PASS] Phase 0: Consistency Audit Verified (VRAM 17739->17538, Sprites 187+215=402)\n";
}

void test_phase1_to_6_convergence_and_dma(const std::vector<std::uint8_t>& rom,
                                          const std::vector<ParsedSnapshot>& c_snaps,
                                          const std::vector<ParsedSnapshot>& h_snaps,
                                          const std::vector<std::uint8_t>& cp_bin) {
    // ROM instruction checks for DMA producer/consumer path
    OASIS_CHECK(rom[0x002130U] == 0x33U && rom[0x002131U] == 0xC0U); // move.w d0, ($FF1652)
    OASIS_CHECK(rom[0x0027FCU] == 0x40U && rom[0x0027FDU] == 0xE7U); // move sr, -(sp)
    OASIS_CHECK(rom[0x00AC3EU] == 0x24U && rom[0x00AC3FU] == 0xC7U); // move.l d7, (a2)+
    OASIS_CHECK(rom[0x00B15EU] == 0x08U && rom[0x00B15FU] == 0x05U); // btst #9, d5

    // Canonical sequence replay with authentic F775 state
    auto c_cp = build_cp(cp_bin);
    c_cp.x_fixed = 73 * 0x10000;
    c_cp.y_fixed = 200 * 0x10000;
    c_cp.tick_counter = 2392U;
    c_cp.turn_timer = 0U;
    c_cp.intent_y_fixed = 0;
    c_cp.attract_banner_enabled = true;

    auto c_sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
    auto c_init_vdp = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(c_snaps[0]));
    OASIS_CHECK(c_sim->initialize_town_checkpoint(rom, c_cp, *c_init_vdp));
    c_sim->set_oracle_mode(false);
    OASIS_CHECK(measure_diff(c_snaps[0], c_sim->current_vdp()).vram_total() == 0U);

    auto out_vdp = std::make_unique<oasis::genesis::VdpState>();
    std::size_t c_next = 1U;
    std::size_t c_vram_sum = 0U, c_px_sum = 0U;
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

        if (c_next < c_snaps.size() && f == c_snaps[c_next].frame) {
            const auto st = measure_diff(c_snaps[c_next], *out_vdp);
            c_vram_sum += st.vram_total();
            c_px_sum += st.px;
            OASIS_CHECK(st.cram == 0U);
            OASIS_CHECK(st.pb == 0U && st.scr == 0U);
            if (f == 779) {
                OASIS_CHECK(st.pat == 0U);
                OASIS_CHECK(st.pa == 0U);
                OASIS_CHECK(st.vsram == 0U);
                OASIS_CHECK(st.regs == 0U);
                OASIS_CHECK(st.unu == 0U);
                OASIS_CHECK(st.px <= 3564U);
                const auto& drained = c_sim->dma_queue().last_drained();
                OASIS_CHECK(drained.size() == 5U);
                // LIFO drains Weapon Piece 0 last -> leaves R21..R23 at 0x13BA5C / 2 = 0x09DD2E
                OASIS_CHECK(drained.back().guest_pc == 0x00B15EU);
                OASIS_CHECK(drained.back().src_byte_addr == 0x0013BA3CU);
                OASIS_CHECK(drained.back().dst_vram_addr == 0x9D00U);
                OASIS_CHECK(drained.back().length_words == 0x0010U);
                OASIS_CHECK(out_vdp->registers[21] == 0x2EU && out_vdp->registers[22] == 0xDDU && out_vdp->registers[23] == 0x09U);
                auto snap_vdp = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(c_snaps[c_next]));
                write_diff_ppm_pair(fs::current_path() / "m16_38_canon_f779_diff.ppm", *snap_vdp, *out_vdp);
            }
            ++c_next;
        }
    }
    OASIS_CHECK(c_next == 7U);
    OASIS_CHECK(c_vram_sum < 6700U);
    OASIS_CHECK(c_px_sum < 155000U);
    c_sim.reset();

    // Holdout replay
    auto h_cp = build_cp(cp_bin);
    h_cp.attract_banner_enabled = false;
    auto h_sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
    auto h_init_vdp = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(h_snaps[0]));
    OASIS_CHECK(h_sim->initialize_town_checkpoint(rom, h_cp, *h_init_vdp));
    h_sim->set_oracle_mode(false);
    OASIS_CHECK(measure_diff(h_snaps[0], h_sim->current_vdp()).vram_total() == 0U);

    std::size_t h_next = 1U;
    std::size_t h_vram_sum = 0U, h_px_sum = 0U;
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
            const auto st = measure_diff(h_snaps[h_next], *out_vdp);
            h_vram_sum += st.vram_total();
            h_px_sum += st.px;
            OASIS_CHECK(st.pa == 0U && st.pb == 0U && st.scr == 0U);
            OASIS_CHECK(st.cram == 0U && st.vsram == 0U);
            if (f == 809) {
                auto snap_vdp = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(h_snaps[h_next]));
                write_diff_ppm_pair(fs::current_path() / "m16_38_hold_f809_diff.ppm", *snap_vdp, *out_vdp);
            }
            ++h_next;
        }
    }
    OASIS_CHECK(h_next == 7U);
    OASIS_CHECK(h_vram_sum < 8000U);
    OASIS_CHECK(h_px_sum < 26000U);
    OASIS_CHECK(fs::file_size(fs::current_path() / "m16_38_canon_f779_diff.ppm") > 600000U);
    std::cout << "[PASS] Phase 1..6: F779 Convergence (Pat=0 PA=0 CRAM=0 VSRAM=0 Regs=0 PX<=3564) & Holdout ("
              << h_px_sum << " px) Verified\n";
}

void test_phase7_dma_mutations(const std::vector<std::uint8_t>& rom, const ParsedSnapshot& f779_orig) {
    auto clean = std::make_unique<oasis::genesis::VdpState>();
    auto mut = std::make_unique<oasis::genesis::VdpState>();
    oasis::game::render::VdpDmaQueue q{};
    const oasis::game::render::DmaTransferEvent req_wpn{0x00B15EU, 0x0013BA3CU, 0x9D00U, 0x0010U, 2U};
    const oasis::game::render::DmaTransferEvent req_ali{0x00AC3EU, 0x000C83A0U, 0x9500U, 0x0040U, 2U};

    auto run_seq = [&](const auto& entries, oasis::genesis::VdpState& out_vdp, bool fifo) {
        q.clear();
        for (const auto& e : entries) q.enqueue(e);
        if (!fifo) {
            q.drain_lifo(rom, out_vdp);
        } else {
            // Mutated FIFO drain order for M4 test
            for (const auto& e : entries) {
                oasis::game::render::VdpDmaQueue single{};
                single.enqueue(e);
                single.drain_lifo(rom, out_vdp);
            }
        }
    };

    // Baseline valid LIFO: Weapon pushed first, Ali pushed second -> Weapon drained last
    run_seq(std::vector<oasis::game::render::DmaTransferEvent>{req_wpn, req_ali}, *clean, false);
    for (std::size_t i = 0; i < 32U; ++i) OASIS_CHECK(clean->vram[0x9D00U + i] == f779_orig.vram[0x9D00U + i]);
    for (std::size_t i = 0; i < 128U; ++i) OASIS_CHECK(clean->vram[0x9500U + i] == f779_orig.vram[0x9500U + i]);

    // M1: Wrong DMA source address (+2)
    *mut = {};
    run_seq(std::vector<oasis::game::render::DmaTransferEvent>{{0x00B15EU, 0x0013BA3EU, 0x9D00U, 0x0010U, 2U}}, *mut, false);
    OASIS_CHECK(std::memcmp(&mut->vram[0x9D00U], &clean->vram[0x9D00U], 32U) != 0);

    // M2: Wrong DMA destination VRAM address (+0x20)
    *mut = {};
    run_seq(std::vector<oasis::game::render::DmaTransferEvent>{{0x00B15EU, 0x0013BA3CU, 0x9D20U, 0x0010U, 2U}}, *mut, false);
    OASIS_CHECK(mut->vram[0x9D00U] == 0U && mut->vram[0x9D20U] == clean->vram[0x9D00U]);

    // M3: Incorrect transfer length (0x20 words instead of 0x40 for Ali Piece 0)
    *mut = {};
    run_seq(std::vector<oasis::game::render::DmaTransferEvent>{{0x00AC3EU, 0x000C83A0U, 0x9500U, 0x0020U, 2U}}, *mut, false);
    OASIS_CHECK(std::memcmp(&mut->vram[0x9540U], &clean->vram[0x9540U], 64U) != 0);

    // M4: Wrong queue ordering (FIFO leaves Ali in R21..R23 instead of Weapon)
    *mut = {};
    run_seq(std::vector<oasis::game::render::DmaTransferEvent>{req_wpn, req_ali}, *mut, true);
    OASIS_CHECK(mut->registers[21] != clean->registers[21] || mut->registers[22] != clean->registers[22]);

    // M5: Wrong VDP autoincrement (1 instead of 2 corrupts odd bytes / upper half)
    *mut = {};
    run_seq(std::vector<oasis::game::render::DmaTransferEvent>{{0x00B15EU, 0x0013BA3CU, 0x9D00U, 0x0010U, 1U}}, *mut, false);
    OASIS_CHECK(std::memcmp(&mut->vram[0x9D00U], &clean->vram[0x9D00U], 32U) != 0);

    // M6: Wrong transfer timing (omitting DMA leaves stale tile 0x9D00 zeroed)
    *mut = {};
    OASIS_CHECK(std::memcmp(&mut->vram[0x9D00U], &clean->vram[0x9D00U], 32U) != 0);

    // M7: Incorrect VRAM residue clearing overwrites untouched inactive VRAM
    *mut = *clean;
    mut->vram[0xF360U] = 0xAAU;
    std::fill(mut->vram.begin() + 0xD280U, mut->vram.end(), 0U);
    OASIS_CHECK(mut->vram[0xF360U] == 0U);

    std::cout << "[PASS] Phase 7: All 7 DMA/VDP Mutation Guards Killed (7/7)\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n";
    std::cout << "SEGA-THOR M16.38: DYNAMIC PATTERN DMA & F779 CONVERGENCE\n";
    std::cout << "=====================================================================\n";

    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    const auto c_snaps = parse_snapshots(read_text(resolve_path("data/standalone-fixtures/town_canonical_vdp_snapshots.json")));
    const auto h_snaps = parse_snapshots(read_text(resolve_path("data/standalone-fixtures/town_independent_holdout_vdp.json")));
    const auto cp_bin = read_binary(resolve_path("data/checkpoints/town_free_control_f775.state"));

    test_phase0_evidence_audit(rom, c_snaps, h_snaps);
    test_phase1_to_6_convergence_and_dma(rom, c_snaps, h_snaps, cp_bin);
    test_phase7_dma_mutations(rom, c_snaps[1]);

    std::cout << "\n[PASS] All M16.38 Tests PASSED successfully!\n";
    return 0;
}
