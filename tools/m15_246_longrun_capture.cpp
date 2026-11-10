#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/gameplay_session.hpp"
#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "cpuhook.h"
#include <libretro.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr unsigned kPostHook = 1U << 14;
constexpr std::uint32_t kRomLimit = 0x300000U;

unsigned (*g_reg)(unsigned){};
int (*g_peek)(unsigned){};
unsigned (*g_cycles)(){};

oasis::hybrid::session::InputJournal* g_journal{};
unsigned g_frame{}, g_poll_idx{}, g_frame_polls{}, g_cur_mask{};

std::vector<std::uint8_t> g_rom_bytes;
std::uint32_t g_vbl_vec{}, g_hbl_vec{};
int g_irq_depth{};
unsigned g_prev_pc{}, g_cur_pc{};

// Streaming trace, video, and audio buffers hashed via canonical SHA-256
std::vector<std::uint8_t> g_trace_chunk, g_video_digests, g_audio_digests;
std::vector<std::string> g_frame_trace_hashes;
std::vector<std::uint8_t> g_frame_vid, g_frame_aud;
std::uint64_t g_total_steps{}, g_idle_steps{}, g_vbl_ticks{};

std::vector<std::uint64_t> g_rom_pc_counts;

struct CalleeFrame {
    unsigned callee_pc{};
    unsigned caller_pc{};
    unsigned stop_sp{};
    unsigned start_cycles{};
    std::uint64_t start_steps{};
    std::uint64_t writes{};
};
struct CalleeStats {
    std::uint64_t entries{};
    std::uint64_t dynamic_steps{};
    std::uint64_t master_cycles{};
    std::uint64_t ram_writes{};
    std::uint64_t nonlocal_returns{};
    std::map<unsigned, std::uint64_t> callers, return_pcs, entity_w0, entity_states, call_depths;
    std::set<unsigned> unique_pcs;
};
std::vector<CalleeFrame> g_callee_stack;
std::map<unsigned, CalleeStats> g_callees;
std::map<unsigned, CalleeStats> g_handlers;

struct CheckpointRec {
    unsigned frame{};
    std::string state_sha, ram_sha, trace_sha, vid_sha, aud_sha;
    std::uint64_t total_steps{}, non_idle_steps{}, vbl_ticks{};
    unsigned end_pc{};
};
std::vector<CheckpointRec> g_checkpoints;

unsigned read_be(unsigned addr, int bytes) {
    unsigned v = 0;
    for (int i = 0; i < bytes; ++i) {
        const int b = g_peek((addr + static_cast<unsigned>(i)) & 0xFFFFFFU);
        v = (v << 8U) | (b >= 0 ? static_cast<unsigned>(b) & 0xFFU : 0U);
    }
    return v;
}

bool is_idle_pc(unsigned pc) {
    return (pc >= 0x008ACAU && pc <= 0x008AD0U) || (pc >= 0x008B72U && pc <= 0x008B7AU);
}

bool environment(unsigned cmd, void* data) {
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) {
        *static_cast<retro_pixel_format*>(data) = RETRO_PIXEL_FORMAT_RGB565; return true;
    }
    if (cmd == RETRO_ENVIRONMENT_GET_CAN_DUPE) { *static_cast<bool*>(data) = true; return true; }
    if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) {
        *static_cast<const char**>(data) = "/tmp"; return true;
    }
    return cmd == RETRO_ENVIRONMENT_SET_VARIABLES || cmd == RETRO_ENVIRONMENT_SET_MEMORY_MAPS ||
           cmd == RETRO_ENVIRONMENT_SET_GEOMETRY || cmd == RETRO_ENVIRONMENT_SET_CONTROLLER_INFO ||
           cmd == RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS || cmd == RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE;
}

void video_cb(const void* data, unsigned w, unsigned h, std::size_t pitch) {
    if (!data || !w || !h) return;
    const auto* p = static_cast<const std::uint8_t*>(data);
    const std::size_t row_bytes = static_cast<std::size_t>(w) * 2U;
    g_frame_vid.clear();
    g_frame_vid.reserve(h * row_bytes);
    for (unsigned y = 0; y < h; ++y) {
        const auto* row = p + static_cast<std::size_t>(y) * pitch;
        g_frame_vid.insert(g_frame_vid.end(), row, row + row_bytes);
    }
}
void audio_cb(std::int16_t l, std::int16_t r) {
    const std::int16_t lr[2]{l, r};
    const auto* p = reinterpret_cast<const std::uint8_t*>(lr);
    g_frame_aud.insert(g_frame_aud.end(), p, p + 4);
}
std::size_t audio_batch_cb(const std::int16_t* data, std::size_t frames) {
    if (data && frames) {
        const auto* p = reinterpret_cast<const std::uint8_t*>(data);
        g_frame_aud.insert(g_frame_aud.end(), p, p + frames * 4U);
    }
    return frames;
}
void input_poll_cb() {
    if (!g_journal) return;
    if (g_poll_idx && g_frame_polls == g_poll_idx) g_journal->end_poll();
    g_cur_mask = g_journal->begin_poll(g_frame, g_poll_idx, 0, oasis::hybrid::session::InputSource::FallbackSchedule);
    ++g_poll_idx; ++g_frame_polls;
}
std::int16_t input_state_cb(unsigned port, unsigned dev, unsigned idx, unsigned id) {
    if (!g_journal) return 0;
    const auto v = (port == 0 && dev == RETRO_DEVICE_JOYPAD && id < 16 && (g_cur_mask & (1U << id))) ? 1 : 0;
    return g_journal->record_query({static_cast<std::uint8_t>(port), static_cast<std::uint8_t>(dev),
                                    static_cast<std::uint16_t>(idx), static_cast<std::uint16_t>(id)},
                                   static_cast<std::int16_t>(v));
}

bool is_bsr_or_jsr_at(unsigned pc) {
    if (pc + 1U >= kRomLimit) return false;
    const std::uint16_t w = (std::uint16_t(g_rom_bytes[pc]) << 8) | g_rom_bytes[pc + 1U];
    return (w & 0xFF00U) == 0x6100U || (w & 0xFFC0U) == 0x4E80U;
}

std::uint32_t g_pre_sp{}, g_pre_sr{};
bool g_pre_is_call{};

void append_u32_le(std::vector<std::uint8_t>& buf, std::uint32_t v) {
    buf.push_back(v & 0xFFU); buf.push_back((v >> 8) & 0xFFU);
    buf.push_back((v >> 16) & 0xFFU); buf.push_back((v >> 24) & 0xFFU);
}

void profile_hook(hook_type_t type, int, unsigned address, unsigned) {
    if (type == HOOK_M68K_E) {
        g_cur_pc = address & 0xFFFFFFU;
        if (g_cur_pc == g_vbl_vec || g_cur_pc == g_hbl_vec) {
            if (g_cur_pc == g_vbl_vec) ++g_vbl_ticks;
            ++g_irq_depth;
        }
        ++g_total_steps;
        append_u32_le(g_trace_chunk, g_cur_pc);
        if (is_idle_pc(g_cur_pc)) ++g_idle_steps;
        if (g_irq_depth == 0) {
            g_pre_sp = g_reg(15);
            g_pre_sr = g_reg(16);
            g_pre_is_call = is_bsr_or_jsr_at(g_cur_pc);
            if (g_cur_pc < kRomLimit) g_rom_pc_counts[g_cur_pc >> 1U]++;
            for (const auto& cf : g_callee_stack) g_callees[cf.callee_pc].unique_pcs.insert(g_cur_pc);
        }
        return;
    }
    if (type == static_cast<hook_type_t>(kPostHook)) {
        const unsigned next_pc = g_reg(16) & 0xFFFFFFU;
        const unsigned sp = g_reg(15);
        if (g_irq_depth > 0) {
            if (g_cur_pc + 1U < kRomLimit && g_rom_bytes[g_cur_pc] == 0x4EU && g_rom_bytes[g_cur_pc + 1U] == 0x73U)
                --g_irq_depth;
            return;
        }
        while (!g_callee_stack.empty() && sp >= g_callee_stack.back().stop_sp) {
            auto cf = g_callee_stack.back();
            g_callee_stack.pop_back();
            auto& cs = g_callees[cf.callee_pc];
            cs.dynamic_steps += (g_total_steps - cf.start_steps);
            cs.master_cycles += (g_cycles() - cf.start_cycles);
            cs.ram_writes += cf.writes;
            if (sp > cf.stop_sp) ++cs.nonlocal_returns;
            cs.return_pcs[next_pc]++;
        }
        if (g_pre_is_call && sp + 4U == g_pre_sp) {
            auto& cs = g_callees[next_pc];
            cs.entries++;
            cs.callers[g_cur_pc]++;
            const unsigned a6 = g_reg(14);
            cs.entity_w0[read_be(a6, 2)]++;
            cs.entity_states[read_be(a6 + 4U, 2)]++;
            cs.call_depths[g_callee_stack.size() + 1U]++;
            g_callee_stack.push_back({next_pc, g_cur_pc, sp + 4U, g_cycles(), g_total_steps, 0});
        } else if (g_cur_pc == 0x0059B8U) {
            auto& hs = g_handlers[next_pc];
            hs.entries++;
            hs.entity_states[g_reg(0) & 0xFFFFU]++;
            hs.callers[g_cur_pc]++;
        }
        g_prev_pc = g_cur_pc;
    } else if ((type & HOOK_M68K_W) != 0 && g_irq_depth == 0) {
        if (!g_callee_stack.empty()) g_callee_stack.back().writes++;
    }
}

void write_map(std::ostream& os, const std::map<unsigned, std::uint64_t>& m, bool hex_keys = true) {
    os << '{'; bool first = true;
    for (const auto& [k, v] : m) {
        if (!first) os << ','; first = false;
        if (hex_keys) os << "\"0x" << std::hex << k << "\":" << std::dec << v;
        else os << '"' << k << "\":" << v;
    }
    os << '}';
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "Usage: m15_246_longrun_capture <core.so> <rom> <state> <polls> <frames> <out_prefix>\n";
        return 2;
    }
    oasis::hybrid::ExternalLibrary core(argv[1]);
    const auto rom = oasis::Rom::load(argv[2]);
    const auto rom_sha = oasis::calculate_sha256(rom.bytes());
    std::ifstream core_f(argv[1], std::ios::binary);
    std::vector<std::uint8_t> core_bytes((std::istreambuf_iterator<char>(core_f)), {});
    const auto core_sha = oasis::calculate_sha256(core_bytes);

    auto retro_set_env = core.get<void(*)(retro_environment_t)>("retro_set_environment");
    auto retro_set_vid = core.get<void(*)(retro_video_refresh_t)>("retro_set_video_refresh");
    auto retro_set_aud = core.get<void(*)(retro_audio_sample_t)>("retro_set_audio_sample");
    auto retro_set_aub = core.get<void(*)(retro_audio_sample_batch_t)>("retro_set_audio_sample_batch");
    auto retro_set_inp = core.get<void(*)(retro_input_poll_t)>("retro_set_input_poll");
    auto retro_set_ins = core.get<void(*)(retro_input_state_t)>("retro_set_input_state");
    auto retro_init = core.get<void(*)()>("retro_init");
    auto retro_load_game = core.get<bool(*)(const retro_game_info*)>("retro_load_game");
    auto retro_run = core.get<void(*)()>("retro_run");
    auto retro_ser_size = core.get<std::size_t(*)()>("retro_serialize_size");
    auto retro_ser = core.get<bool(*)(void*, std::size_t)>("retro_serialize");
    auto retro_unser = core.get<bool(*)(const void*, std::size_t)>("retro_unserialize");
    auto retro_hybrid_install = core.get<void(*)(void(*)(hook_type_t, int, unsigned, unsigned))>("retro_hybrid_install");
    g_reg = core.get<unsigned(*)(unsigned)>("retro_9c40_capture_register");
    g_peek = core.get<int(*)(unsigned)>("retro_9c40_capture_peek");
    g_cycles = core.get<unsigned(*)()>("retro_9c40_capture_cycles");

    retro_set_env(environment); retro_set_vid(video_cb); retro_set_aud(audio_cb);
    retro_set_aub(audio_batch_cb); retro_set_inp(input_poll_cb); retro_set_ins(input_state_cb);
    retro_init();
    retro_game_info gi{}; gi.path = argv[2]; gi.data = rom.bytes().data(); gi.size = rom.bytes().size();
    if (!retro_load_game(&gi)) return 3;

    std::ifstream st_f(argv[3], std::ios::binary);
    std::vector<std::uint8_t> init_state((std::istreambuf_iterator<char>(st_f)), {});
    const auto init_sha = oasis::calculate_sha256(init_state);
    if (!retro_unser(init_state.data(), init_state.size())) return 4;

    auto journal = oasis::hybrid::session::InputJournal::load(argv[4]);
    g_journal = &journal;
    const unsigned frames = static_cast<unsigned>(std::stoul(argv[5]));

    g_rom_bytes.assign(rom.bytes().begin(), rom.bytes().end());
    auto rd32 = [&](std::size_t off) -> std::uint32_t {
        return (std::uint32_t(g_rom_bytes[off]) << 24) | (std::uint32_t(g_rom_bytes[off + 1]) << 16) |
               (std::uint32_t(g_rom_bytes[off + 2]) << 8) | std::uint32_t(g_rom_bytes[off + 3]);
    };
    g_hbl_vec = rd32(0x70) & 0xFFFFFFU;
    g_vbl_vec = rd32(0x78) & 0xFFFFFFU;
    g_rom_pc_counts.assign(kRomLimit >> 1U, 0);
    retro_hybrid_install(profile_hook);

    const std::size_t ser_sz = retro_ser_size();
    std::vector<std::uint8_t> ser_buf(ser_sz), wram(0x10000U);
    std::string trace_digest_concat;
    for (unsigned f = 0; f < frames; ++f) {
        g_frame = f; g_poll_idx = 0; g_frame_polls = 0;
        g_trace_chunk.clear(); g_frame_vid.clear(); g_frame_aud.clear();
        retro_run();
        journal.end_frame(f, g_frame_polls);
        const auto fth = oasis::calculate_sha256(g_trace_chunk);
        trace_digest_concat += fth;
        const auto fvh = oasis::calculate_sha256(g_frame_vid);
        const auto fah = oasis::calculate_sha256(g_frame_aud);
        g_video_digests.insert(g_video_digests.end(), fvh.begin(), fvh.end());
        g_audio_digests.insert(g_audio_digests.end(), fah.begin(), fah.end());
        if ((f + 1U) % 300U == 0U || f + 1U == frames) {
            retro_ser(ser_buf.data(), ser_sz);
            for (unsigned i = 0; i < 0x10000U; ++i) wram[i] = static_cast<std::uint8_t>(g_peek(0xFF0000U + i) & 0xFF);
            g_checkpoints.push_back({f + 1U, oasis::calculate_sha256(ser_buf), oasis::calculate_sha256(wram),
                fth, fvh, fah, g_total_steps, g_total_steps - g_idle_steps, g_vbl_ticks, g_reg(16)});
        }
    }

    const auto trace_all_sha = oasis::calculate_sha256(
        {reinterpret_cast<const std::uint8_t*>(trace_digest_concat.data()), trace_digest_concat.size()});
    const auto vid_all_sha = oasis::calculate_sha256(g_video_digests);
    const auto aud_all_sha = oasis::calculate_sha256(g_audio_digests);

    std::vector<std::pair<std::uint64_t, unsigned>> top_pcs;
    for (unsigned i = 0; i < g_rom_pc_counts.size(); ++i) {
        if (!g_rom_pc_counts[i]) continue;
        const unsigned pc = i << 1U;
        if (!is_idle_pc(pc)) top_pcs.push_back({g_rom_pc_counts[i], pc});
    }
    std::partial_sort(top_pcs.begin(), top_pcs.begin() + std::min<std::size_t>(250, top_pcs.size()), top_pcs.end(),
                      [](auto a, auto b) { return a.first > b.first; });
    if (top_pcs.size() > 250) top_pcs.resize(250);

    std::vector<std::pair<std::uint64_t, unsigned>> top_callees;
    for (const auto& [pc, cs] : g_callees) top_callees.push_back({cs.dynamic_steps, pc});
    std::sort(top_callees.begin(), top_callees.end(), [](auto a, auto b) { return a.first > b.first; });

    const std::string pfx = argv[6];
    std::ofstream out(pfx + "_profile.json");
    out << "{\n  \"frames\": " << frames
        << ",\n  \"rom_sha256\": \"" << rom_sha << "\",\n  \"core_sha256\": \"" << core_sha
        << "\",\n  \"initial_state_sha256\": \"" << init_sha
        << "\",\n  \"total_original_steps\": " << g_total_steps
        << ",\n  \"idle_steps\": " << g_idle_steps
        << ",\n  \"non_idle_steps\": " << (g_total_steps - g_idle_steps)
        << ",\n  \"game_vbl_ticks\": " << g_vbl_ticks
        << ",\n  \"original_cpu_trace_sha256\": \"" << trace_all_sha
        << "\",\n  \"video_stream_sha256\": \"" << vid_all_sha
        << "\",\n  \"audio_stream_sha256\": \"" << aud_all_sha << "\",\n  \"checkpoints\": [\n";
    for (std::size_t i = 0; i < g_checkpoints.size(); ++i) {
        const auto& c = g_checkpoints[i];
        out << (i ? ",\n" : "") << "    {\"frame\": " << c.frame << ", \"state_sha256\": \"" << c.state_sha
            << "\", \"ram_sha256\": \"" << c.ram_sha << "\", \"frame_trace_sha256\": \"" << c.trace_sha
            << "\", \"video_frame_sha256\": \"" << c.vid_sha << "\", \"audio_frame_sha256\": \"" << c.aud_sha
            << "\", \"total_steps\": " << c.total_steps << ", \"non_idle_steps\": " << c.non_idle_steps
            << ", \"vbl_ticks\": " << c.vbl_ticks << "}";
    }
    out << "\n  ],\n  \"table_handlers\": {\n";
    bool fh = true;
    for (const auto& [pc, hs] : g_handlers) {
        out << (fh ? "" : ",\n") << "    \"0x" << std::hex << pc << std::dec << "\": {\"entries\": " << hs.entries
            << ", \"states\": "; write_map(out, hs.entity_states); out << "}"; fh = false;
    }
    out << "\n  },\n  \"top_callees\": {\n";
    const std::size_t n_cal = std::min<std::size_t>(60, top_callees.size());
    for (std::size_t i = 0; i < n_cal; ++i) {
        const unsigned pc = top_callees[i].second; const auto& cs = g_callees[pc];
        out << (i ? ",\n" : "") << "    \"0x" << std::hex << pc << std::dec << "\": {\"entries\": " << cs.entries
            << ", \"dynamic_steps\": " << cs.dynamic_steps << ", \"unique_pcs\": " << cs.unique_pcs.size()
            << ", \"guest_cycles\": " << (cs.master_cycles / 7U) << ", \"ram_writes\": " << cs.ram_writes
            << ", \"nonlocal_returns\": " << cs.nonlocal_returns << ", \"callers\": "; write_map(out, cs.callers);
        out << ", \"return_pcs\": "; write_map(out, cs.return_pcs);
        out << ", \"call_depths\": "; write_map(out, cs.call_depths, false);
        out << ", \"entity_w0\": "; write_map(out, cs.entity_w0);
        out << ", \"entity_states\": "; write_map(out, cs.entity_states); out << "}";
    }
    out << "\n  },\n  \"top_non_idle_pcs\": [\n";
    for (std::size_t i = 0; i < top_pcs.size(); ++i)
        out << (i ? ",\n" : "") << "    {\"pc\": \"0x" << std::hex << top_pcs[i].second << std::dec << "\", \"count\": " << top_pcs[i].first << "}";
    out << "\n  ],\n  \"all_executed_pcs_above_500\": {";
    bool fp = true;
    for (unsigned i = 0; i < g_rom_pc_counts.size(); ++i) {
        if (g_rom_pc_counts[i] < 500ULL) continue;
        out << (fp ? "" : ",") << "\"0x" << std::hex << (i << 1U) << "\":" << std::dec << g_rom_pc_counts[i];
        fp = false;
    }
    out << "}\n}\n";
    std::cout << "LONGRUN_CAPTURE_OK frames=" << frames << " total_steps=" << g_total_steps
              << " non_idle=" << (g_total_steps - g_idle_steps) << " trace_sha=" << trace_all_sha << "\n";
    return 0;
}
