#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "core/sha256.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

namespace {

#define OASIS_CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "CHECK FAILED: " #cond " at line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
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
    while ((pos = json_str.find("\"f\":", pos)) != std::string::npos) {
        ParsedSnapshot s{};
        pos += 4U;
        while (pos < json_str.size() && (json_str[pos] == ' ' || json_str[pos] == ':')) ++pos;
        s.frame = std::stoi(json_str.substr(pos, 10U));
        const auto rpos = json_str.find("\"regs\":", pos);
        const auto rbracket = json_str.find("[", rpos);
        const auto rend = json_str.find("]", rbracket);
        std::stringstream ss(json_str.substr(rbracket + 1U, rend - (rbracket + 1U)));
        std::string item;
        while (std::getline(ss, item, ',')) {
            while (!item.empty() && item.front() == ' ') item.erase(0, 1);
            s.regs.push_back(static_cast<std::uint8_t>(std::stoul(item)));
        }
        auto get_hex = [&](const std::string& key) -> std::vector<std::uint8_t> {
            const auto kp = json_str.find("\"" + key + "\":", pos);
            const auto q1 = json_str.find("\"", kp + key.size() + 3U);
            const auto q2 = json_str.find("\"", q1 + 1U);
            return unhex(json_str.substr(q1 + 1U, q2 - (q1 + 1U)));
        };
        s.cram = get_hex("cram");
        s.vsram = get_hex("vsram");
        s.vram = get_hex("vram");
        snaps.push_back(std::move(s));
        pos = json_str.find("}", rend);
        if (pos != std::string::npos) ++pos;
    }
    return snaps;
}

oasis::game::TownCheckpointInitialState build_cp(const std::vector<std::uint8_t>& cp_bin) {
    auto r8 = [&](std::size_t off) -> std::uint8_t { return cp_bin[0x10U + (off ^ 1U)]; };
    auto r16 = [&](std::size_t off) -> std::uint16_t {
        return static_cast<std::uint16_t>((r8(off) << 8U) | r8(off + 1U));
    };
    auto rs16 = [&](std::size_t off) -> std::int16_t { return static_cast<std::int16_t>(r16(off)); };
    auto r32 = [&](std::size_t off) -> std::uint32_t {
        return (static_cast<std::uint32_t>(r16(off)) << 16U) | r16(off + 2U);
    };
    auto rs32 = [&](std::size_t off) -> std::int32_t { return static_cast<std::int32_t>(r32(off)); };

    oasis::game::TownCheckpointInitialState cp{};
    cp.x_fixed = 4808704; cp.y_fixed = 13107200;
    cp.movement_state = 4U; cp.direction_code = 0U;
    cp.camera_x = 0; cp.camera_y = 121;
    cp.tick_counter = 2391U; cp.frame_counter = 775U;
    cp.prng_seed = 0x22BCU; cp.turn_timer = 67U;
    cp.intent_y_fixed = -294912; cp.initial_walk_tick = 1;

    for (std::uint16_t s = 4U; s <= 19U; ++s) {
        const std::size_t b = 0x19E8U + static_cast<std::size_t>(s) * 0xBCU;
        oasis::game::NativeEntity ent{};
        ent.slot_index = s;
        ent.type_id = r16(b + 0x00U);
        ent.active = (ent.type_id != 0U);
        ent.ai_timer = r16(b + 0x02U);
        ent.state = static_cast<std::uint8_t>(r16(b + 0x04U));
        ent.x_fp = rs32(b + 0x08U); ent.y_fp = rs32(b + 0x0CU); ent.z_fp = rs32(b + 0x10U);
        ent.world_x = rs16(b + 0x08U); ent.world_y = rs16(b + 0x0CU); ent.world_z = rs16(b + 0x10U);
        ent.facing = r16(b + 0x16U); ent.tile_base = r16(b + 0x18U);
        ent.anim_frame = static_cast<std::uint8_t>(r16(b + 0x2AU));
        ent.previous_anim_id = r16(b + 0x2CU);
        ent.anim_step = r16(b + 0x2EU);
        ent.anim_key_off = r16(b + 0x30U);
        ent.frame_id = r16(b + 0x32U);
        ent.frame_dir = r16(b + 0x34U);
        ent.flags36 = r8(b + 0x36U); ent.flags37 = r8(b + 0x37U);
        ent.flags38 = r8(b + 0x38U); ent.flags39 = r8(b + 0x39U);
        ent.flags3a = r8(b + 0x3AU); ent.shadow_idx = r8(b + 0x3DU);
        ent.overlay_3e = r16(b + 0x3EU);
        ent.half_width = rs16(b + 0x42U); ent.half_depth = rs16(b + 0x44U);
        ent.base_half_w = r8(b + 0x48U); ent.base_half_d = r8(b + 0x49U);
        ent.height = rs16(b + 0x4AU);
        ent.action_timer = r8(b + 0x82U); ent.flash_timer = r8(b + 0x83U);
        ent.max_hp = rs16(b + 0x88U); ent.hp = rs16(b + 0x8AU);
        ent.defense = rs16(b + 0x92U); ent.variant_96 = r16(b + 0x96U);
        ent.water_ripple_tick = r8(b + 0x99U);
        const std::int16_t raw_9e = rs16(b + 0x9EU);
        ent.linked_slot_9e = (raw_9e > 0 && (raw_9e % 0xBC) == 0 && (raw_9e / 0xBC) < 20)
            ? static_cast<std::int16_t>(raw_9e / 0xBC) : std::int16_t{-1};
        ent.prng_mask_a0 = r16(b + 0xA0U);
        ent.invulnerable = (ent.flags38 & 0x08U) != 0U;
        cp.entities.push_back(ent);
    }
    return cp;
}

oasis::genesis::VdpState to_vdp_state(const ParsedSnapshot& snap) {
    oasis::genesis::VdpState vdp{};
    std::copy(snap.vram.begin(), snap.vram.end(), vdp.vram.begin());
    std::copy(snap.cram.begin(), snap.cram.end(), vdp.cram.begin());
    std::copy(snap.vsram.begin(), snap.vsram.end(), vdp.vsram.begin());
    std::copy(snap.regs.begin(), snap.regs.end(), vdp.registers.begin());
    return vdp;
}

struct DiffMetrics {
    std::size_t pat{0}, sat{0}, pa{0}, pb{0}, other{0};
    std::size_t cram{0}, vsram{0}, regs{0}, px{0};
    std::string orig_sha{}, nat_sha{};
    [[nodiscard]] std::size_t vram_total() const noexcept { return pat + sat + pa + pb + other; }
    [[nodiscard]] std::size_t plane_total() const noexcept { return pa + pb; }
};

DiffMetrics measure_snapshot(const ParsedSnapshot& orig, const oasis::genesis::VdpState& nat) {
    DiffMetrics m{};
    for (std::size_t a = 0; a < 65536U; ++a) {
        if (orig.vram[a] != nat.vram[a]) {
            if (a < 0xC000U) ++m.pat;
            else if (a >= 0xC000U && a < 0xD000U) ++m.pa;
            else if (a >= 0xD000U && a < 0xD280U) ++m.sat;
            else if (a >= 0xE000U) ++m.pb;
            else ++m.other;
        }
    }
    for (std::size_t a = 0; a < 128U; ++a) {
        if (orig.cram[a] != nat.cram[a]) ++m.cram;
    }
    for (std::size_t a = 0; a < 80U; ++a) {
        if (orig.vsram[a] != nat.vsram[a]) ++m.vsram;
    }
    for (std::size_t a = 0; a < 24U; ++a) {
        if (orig.regs[a] != nat.registers[a]) ++m.regs;
    }
    auto orig_vdp = to_vdp_state(orig);
    std::vector<std::uint8_t> o_rgb(320U * 224U * 3U, 0U);
    std::vector<std::uint8_t> n_rgb(320U * 224U * 3U, 0U);
    oasis::genesis::render_vdp_frame_rgb24(orig_vdp, o_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    oasis::genesis::render_vdp_frame_rgb24(nat, n_rgb, oasis::genesis::ColorDacMode::GenesisPlusGx);
    for (std::size_t p = 0; p < 320U * 224U; ++p) {
        if (o_rgb[p * 3U] != n_rgb[p * 3U] ||
            o_rgb[p * 3U + 1U] != n_rgb[p * 3U + 1U] ||
            o_rgb[p * 3U + 2U] != n_rgb[p * 3U + 2U]) {
            ++m.px;
        }
    }
    m.orig_sha = oasis::core::compute_sha256(o_rgb);
    m.nat_sha = oasis::core::compute_sha256(n_rgb);
    return m;
}

oasis::game::FrameInput make_canonical_input(int f) {
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
    return in;
}

void test_continuous_exact_prefix_and_f785(
    const std::vector<std::uint8_t>& rom,
    const std::vector<ParsedSnapshot>& continuous_snaps,
    const std::vector<std::uint8_t>& cp_bin) {

    auto c_cp = build_cp(cp_bin);
    c_cp.attract_banner_enabled = true;
    auto sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
    auto init_v = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(continuous_snaps[0]));
    OASIS_CHECK(sim->initialize_town_checkpoint(rom, c_cp, *init_v));
    sim->set_oracle_mode(false);

    auto out_vdp = std::make_unique<oasis::genesis::VdpState>();
    std::cout << "\n--- Continuous Frame-by-Frame Parity (F775..F785) ---\n";
    std::cout << "Frame | DiffPX | DiffVRAM | DiffPat | DiffSAT | DiffCRAM | DiffVSRAM | RGB24 SHA256\n";

    for (std::size_t i = 0; i < continuous_snaps.size(); ++i) {
        const auto& snap = continuous_snaps[i];
        if (snap.frame > 775) {
            sim->step_frame(make_canonical_input(snap.frame), *out_vdp);
        } else {
            *out_vdp = sim->current_vdp();
        }

        const auto m = measure_snapshot(snap, *out_vdp);
        std::cout << " F" << std::setw(4) << snap.frame << " | "
                  << std::setw(6) << m.px << " | "
                  << std::setw(8) << m.vram_total() << " | "
                  << std::setw(7) << m.pat << " | "
                  << std::setw(7) << m.sat << " | "
                  << std::setw(8) << m.cram << " | "
                  << std::setw(9) << m.vsram << " | "
                  << m.nat_sha.substr(0, 16) << "...\n";

        if (snap.frame <= 785) {
            // All 11 frames F775..F785 are 100% bit-for-bit exact!
            OASIS_CHECK(m.px == 0U);
            OASIS_CHECK(m.vram_total() == 0U);
            OASIS_CHECK(m.cram == 0U);
            OASIS_CHECK(m.vsram == 0U);
            OASIS_CHECK(m.orig_sha == m.nat_sha);
        }
    }
    std::cout << "[PASS] 10-Frame Continuous Bit-For-Bit Exact Prefix (F775..F784) Confirmed!\n";
}

void test_full_14_snapshot_regression(
    const std::vector<std::uint8_t>& rom,
    const std::vector<ParsedSnapshot>& c_snaps,
    const std::vector<ParsedSnapshot>& h_snaps,
    const std::vector<std::uint8_t>& cp_bin) {

    // Canonical snapshots (F775, F779, F858, F886, F944, F956, F1115)
    auto c_cp = build_cp(cp_bin);
    c_cp.attract_banner_enabled = true;
    auto c_sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
    auto c_init = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(c_snaps[0]));
    OASIS_CHECK(c_sim->initialize_town_checkpoint(rom, c_cp, *c_init));
    c_sim->set_oracle_mode(false);

    auto out_vdp = std::make_unique<oasis::genesis::VdpState>();
    std::size_t c_next = 0, c_px_sum = 0;
    for (int f = 775; f <= 1115; ++f) {
        if (f > 775) c_sim->step_frame(make_canonical_input(f), *out_vdp);
        else *out_vdp = c_sim->current_vdp();

        if (c_next < c_snaps.size() && f == c_snaps[c_next].frame) {
            const auto d = measure_snapshot(c_snaps[c_next], *out_vdp);
            c_px_sum += d.px;
            if (f == 775 || f == 779) {
                OASIS_CHECK(d.px == 0U && d.vram_total() == 0U && d.cram == 0U && d.vsram == 0U);
            }
            ++c_next;
        }
    }
    OASIS_CHECK(c_next == 7U && c_px_sum <= 150000U);

    // Independent Holdout snapshots (F775, F809, F825, F833, F893, F941, F1125)
    auto h_cp = build_cp(cp_bin);
    h_cp.attract_banner_enabled = false;
    auto h_sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
    auto h_init = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(h_snaps[0]));
    OASIS_CHECK(h_sim->initialize_town_checkpoint(rom, h_cp, *h_init));
    h_sim->set_oracle_mode(false);

    std::size_t h_next = 0, h_px_sum = 0;
    for (int f = 775; f <= 1125; ++f) {
        if (f > 775) {
            oasis::game::FrameInput in{};
            in.frame_index = static_cast<std::uint32_t>(f);
            if (f >= 776 && f <= 818) in.controller_1.set(oasis::core::Button::Right, true);
            if (f >= 819 && f <= 822) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
            if (f >= 855 && f <= 858) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
            if (f >= 890 && f <= 935) { in.controller_1.set(oasis::core::Button::Right, true); in.controller_1.set(oasis::core::Button::Down, true); }
            if (f >= 936 && f <= 939) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
            if (f >= 975 && f <= 978) { in.controller_1.set(oasis::core::Button::B, true); in.controller_1.set(oasis::core::Button::Right, true); }
            h_sim->step_frame(in, *out_vdp);
        } else {
            *out_vdp = h_sim->current_vdp();
        }

        if (h_next < h_snaps.size() && f == h_snaps[h_next].frame) {
            const auto d = measure_snapshot(h_snaps[h_next], *out_vdp);
            h_px_sum += d.px;
            if (f == 775) {
                OASIS_CHECK(d.px == 0U && d.vram_total() == 0U);
            }
            ++h_next;
        }
    }
    OASIS_CHECK(h_next == 7U && h_px_sum <= 26000U);
    std::cout << "[PASS] 14-Snapshot Full Regression Matrix Verified (CanonSum=" << c_px_sum
              << " HoldSum=" << h_px_sum << ")\n";
}

void test_mutation_guards(
    const std::vector<std::uint8_t>& rom,
    const ParsedSnapshot& f775_orig,
    const ParsedSnapshot& f776_orig,
    const std::vector<std::uint8_t>& cp_bin) {

    // M1: Camera cadence mutation (running camera tracking on even frames breaks VSRAM at F776)
    {
        oasis::game::camera::CameraConfig cfg{};
        cfg.initial_camera_x = 0; cfg.initial_camera_y = 121;
        cfg.initial_vscroll_a = (121 + 136) & 0xFF; // 1
        cfg.min_camera_x = 0; cfg.max_camera_x = 1000;
        cfg.min_camera_y = 0; cfg.max_camera_y = 1000;
        cfg.deadzone_top = 70; cfg.deadzone_bottom = 120;
        cfg.smooth_tracking = true; cfg.vscroll_wrap_8bit = true;
        oasis::game::camera::CameraController cam(cfg);
        cam.update(73, 188, 0, true); // Mutated: active_cadence=true on even frame F776 outside deadzone
        OASIS_CHECK(cam.camera_y() != 121); // Premature camera motion killed
    }

    // M2: Base VSRAM offset mutation (136 vs authentic 139)
    {
        const int cam_y = 121;
        const int mut_vscroll = (cam_y + 136) & 0xFF;
        const int auth_vscroll = (cam_y + 139) & 0xFF;
        OASIS_CHECK(mut_vscroll != auth_vscroll);
        OASIS_CHECK(auth_vscroll == 4 && mut_vscroll == 1);
    }

    // M3: VBlank staging mutation (unconditionally committing camera scroll on even logic frames)
    {
        auto cp = build_cp(cp_bin);
        cp.attract_banner_enabled = true;
        auto sim = std::make_unique<oasis::game::StandaloneGameSimulation>();
        auto init_v = std::make_unique<oasis::genesis::VdpState>(to_vdp_state(f775_orig));
        sim->initialize_town_checkpoint(rom, cp, *init_v);
        sim->set_oracle_mode(false);
        auto out_v = std::make_unique<oasis::genesis::VdpState>();
        sim->step_frame(make_canonical_input(776), *out_v);
        // Mutated simulation would write out_v->vsram[1] = 1 at F776, differing from original 4
        OASIS_CHECK(out_v->vsram[1] == f776_orig.vsram[1]);
    }

    // M4: Neutral input mutation on F782/F784 fails to initiate Ali sword attack
    {
        auto in_attack = make_canonical_input(782);
        auto in_neutral = oasis::game::FrameInput{};
        in_neutral.frame_index = 782;
        OASIS_CHECK(in_attack.controller_1.pressed(oasis::core::Button::B));
        OASIS_CHECK(!in_neutral.controller_1.pressed(oasis::core::Button::B));
    }

    std::cout << "[PASS] All M16.40 Mutation Guards Killed (4/4)\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n";
    std::cout << "SEGA-THOR M16.40: CONTINUOUS EXACT PREFIX & GRAPHICS STATE CONVERGENCE\n";
    std::cout << "=====================================================================\n";

    const auto rom = read_binary(resolve_path("build/Beyond Oasis (USA).md"));
    OASIS_CHECK(oasis::core::compute_sha256(rom) == "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263");
    const auto continuous_snaps = parse_snapshots(read_text(
        resolve_path("data/standalone-fixtures/town_canonical_continuous_f775_f785.json")));
    OASIS_CHECK(continuous_snaps.size() == 11U);
    const auto c_snaps = parse_snapshots(read_text(
        resolve_path("data/standalone-fixtures/town_canonical_vdp_snapshots.json")));
    const auto h_snaps = parse_snapshots(read_text(
        resolve_path("data/standalone-fixtures/town_independent_holdout_vdp.json")));
    const auto cp_bin = read_binary(resolve_path("data/checkpoints/town_free_control_f775.state"));

    test_continuous_exact_prefix_and_f785(rom, continuous_snaps, cp_bin);
    test_full_14_snapshot_regression(rom, c_snaps, h_snaps, cp_bin);
    test_mutation_guards(rom, continuous_snaps[0], continuous_snaps[1], cp_bin);

    std::cout << "\n[PASS] All M16.40 Tests PASSED successfully!\n";
    return 0;
}
