#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/gameplay_session.hpp"
#include "core/rom.hpp"
#include "cpuhook.h"
#include <libretro.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
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

unsigned read_be(unsigned addr, int bytes) {
    unsigned v = 0;
    for (int i = 0; i < bytes; ++i) {
        const int b = g_peek((addr + static_cast<unsigned>(i)) & 0xFFFFFFU);
        v = (v << 8U) | (b >= 0 ? static_cast<unsigned>(b) & 0xFFU : 0U);
    }
    return v;
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
void video_cb(const void*, unsigned, unsigned, std::size_t) {}
void audio_cb(std::int16_t, std::int16_t) {}
std::size_t audio_batch_cb(const std::int16_t*, std::size_t c) { return c; }
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

struct HandlerStats {
    std::uint64_t entries{};
    std::uint64_t dynamic_steps{};
    std::uint64_t exclusive_steps{};
    std::uint64_t master_cycles{};
    std::uint64_t ram_writes{};
    std::uint64_t nonlocal_returns{};
    std::set<unsigned> unique_pcs;
    std::map<unsigned, std::uint64_t> callers;
    std::map<unsigned, std::uint64_t> indices;
    std::map<unsigned, std::uint64_t> entity_w0;
    std::map<unsigned, std::uint64_t> entity_states;
    std::map<unsigned, std::uint64_t> return_pcs;
    std::map<unsigned, std::uint64_t> call_depths;
};

struct ActiveHandler {
    unsigned callee_pc{};
    unsigned caller_pc{};
    unsigned stop_sp{};
    unsigned start_cycles{};
    unsigned steps{};
    unsigned writes{};
    std::uint16_t saved_sr{};
    bool in_irq{};
};

struct ActiveCaller5670 {
    bool active{};
    unsigned caller_pc{};
    unsigned stop_sp{};
    unsigned ret_on_stack{};
    unsigned entry_a6{};
    unsigned start_cycles{};
    unsigned steps{};
    unsigned writes{};
    bool reached_59a8{};
};

struct TimerCall {
    unsigned caller_pc{};
    unsigned entry_a7{};
    unsigned a6{};
    unsigned start_cycles{};
    std::uint8_t t130_pre{};
    std::uint8_t t157_pre{};
    bool saw_addq_a7{};
};

std::vector<std::uint64_t> g_rom_pc_counts;
std::vector<std::uint8_t> g_rom_bytes;
std::uint32_t g_vbl_vec{}, g_hbl_vec{};
int g_irq_depth{};
std::uint64_t g_total_steps{};
std::uint64_t g_idle_steps{};
unsigned g_prev_pc{};
unsigned g_cur_pc{};

std::map<unsigned, HandlerStats> g_handlers;
std::vector<ActiveHandler> g_handler_stack;

ActiveCaller5670 g_5670{};
std::uint64_t g_5670_entries{}, g_5670_from_5586{}, g_5670_other_caller{};
std::uint64_t g_5670_to_59b8{}, g_5670_early_rts{}, g_5670_steps{}, g_5670_nonlocal{};
std::map<unsigned, std::uint64_t> g_5670_callers, g_5670_exits;

std::vector<TimerCall> g_e682_calls;
std::uint64_t g_e682_entries{}, g_e682_normal_rts{}, g_e682_nonlocal_rts{};
std::map<unsigned, std::uint64_t> g_e682_callers, g_e682_ret_pcs;

std::uint64_t g_59a8_entries{}, g_59a8_direct{}, g_59a8_via_5670{};
std::map<unsigned, std::uint64_t> g_59a8_callers;

unsigned count_call_depth(unsigned sp) {
    unsigned depth = 0;
    for (unsigned s = sp; s < 0x00FF0C00U && depth < 16U; s += 4U) {
        const unsigned lw = read_be(s, 4) & 0xFFFFFFU;
        if (lw >= 0x000200U && lw < kRomLimit) ++depth;
    }
    return depth;
}

void profile_hook(hook_type_t type, int /*width*/, unsigned address, unsigned /*value*/) {
    address &= 0xFFFFFFU;
    if (type == HOOK_M68K_E) {
        g_cur_pc = address;
        ++g_total_steps;
        if ((address == g_vbl_vec && g_vbl_vec) || (address == g_hbl_vec && g_hbl_vec)) {
            ++g_irq_depth;
        }
        if ((address >= 0x008ACAU && address <= 0x008AD0U) ||
            (address >= 0x008B72U && address <= 0x008B7AU)) {
            ++g_idle_steps;
        }
        if (g_irq_depth == 0 && address < kRomLimit) ++g_rom_pc_counts[address >> 1U];

        const unsigned sp = g_reg(15);
        if (g_irq_depth == 0 && !g_handler_stack.empty()) {
            auto& ah = g_handler_stack.back();
            auto& hs = g_handlers[ah.callee_pc];
            hs.dynamic_steps++;
            ah.steps++;
            if (address < kRomLimit) hs.unique_pcs.insert(address);
        }

        if (address == 0x005670U) {
            ++g_5670_entries;
            const unsigned ret_stk = read_be(sp, 4) & 0xFFFFFFU;
            g_5670_callers[g_prev_pc]++;
            if (g_prev_pc == 0x005586U && ret_stk == 0x00558AU) ++g_5670_from_5586;
            else ++g_5670_other_caller;
            if (!g_5670.active) {
                g_5670 = ActiveCaller5670{};
                g_5670.active = true;
                g_5670.caller_pc = g_prev_pc;
                g_5670.stop_sp = sp + 4U;
                g_5670.ret_on_stack = ret_stk;
                g_5670.entry_a6 = g_reg(14);
                g_5670.start_cycles = g_cycles();
            }
        }
        if (g_5670.active) {
            ++g_5670_steps;
            ++g_5670.steps;
            if (address == 0x0059A8U) g_5670.reached_59a8 = true;
        }

        if (address == 0x0059A8U) {
            ++g_59a8_entries;
            if (g_5670.active) ++g_59a8_via_5670;
            else {
                ++g_59a8_direct;
                g_59a8_callers[g_prev_pc]++;
            }
        }

        if (address == 0x00E682U) {
            ++g_e682_entries;
            g_e682_callers[g_prev_pc]++;
            TimerCall tc{};
            tc.caller_pc = g_prev_pc;
            tc.entry_a7 = sp;
            tc.a6 = g_reg(14);
            tc.start_cycles = g_cycles();
            tc.t130_pre = static_cast<std::uint8_t>(g_peek((tc.a6 + 130U) & 0xFFFFFFU));
            tc.t157_pre = static_cast<std::uint8_t>(g_peek((tc.a6 + 157U) & 0xFFFFFFU));
            g_e682_calls.push_back(tc);
        } else if (!g_e682_calls.empty()) {
            if (address == 0x00E6A6U) g_e682_calls.back().saw_addq_a7 = true;
        }
        g_prev_pc = address;
    } else if (type == static_cast<hook_type_t>(kPostHook)) {
        const unsigned next_pc = g_reg(16) & 0xFFFFFFU;
        const unsigned sp = g_reg(15);
        if (g_irq_depth > 0 && g_cur_pc + 1U < g_rom_bytes.size() &&
            g_rom_bytes[g_cur_pc] == 0x4EU && g_rom_bytes[g_cur_pc + 1U] == 0x73U) {
            --g_irq_depth;
            return;
        }
        if (g_irq_depth > 0) return;

        if (!g_e682_calls.empty()) {
            auto& tc = g_e682_calls.back();
            if (g_cur_pc == 0x00E6A8U && tc.saw_addq_a7) {
                ++g_e682_nonlocal_rts;
                g_e682_ret_pcs[next_pc]++;
                g_e682_calls.pop_back();
            } else if (g_cur_pc == 0x00E698U) {
                ++g_e682_normal_rts;
                g_e682_calls.pop_back();
            }
        }

        if (g_5670.active) {
            if (g_cur_pc == 0x0059B8U) {
                ++g_5670_to_59b8;
                g_5670_exits[next_pc]++;
                g_5670.active = false;
            } else if (sp >= g_5670.stop_sp) {
                if (sp > g_5670.stop_sp) ++g_5670_nonlocal;
                else ++g_5670_early_rts;
                g_5670_exits[next_pc]++;
                g_5670.active = false;
            }
        }

        // Check active handler completion when SP returns above stop_sp
        while (!g_handler_stack.empty() && sp >= g_handler_stack.back().stop_sp) {
            auto ah = g_handler_stack.back();
            g_handler_stack.pop_back();
            auto& hs = g_handlers[ah.callee_pc];
            hs.master_cycles += (g_cycles() - ah.start_cycles);
            hs.ram_writes += ah.writes;
            if (sp > ah.stop_sp) ++hs.nonlocal_returns;
            hs.return_pcs[next_pc]++;
        }

        // Launch handler tracking right after 0x0059B8 JMP (A0)
        if (g_cur_pc == 0x0059B8U) {
            const unsigned d0_w = g_reg(0) & 0xFFFFU;
            const unsigned idx = d0_w / 2U;
            const unsigned a6 = g_reg(14);
            const unsigned w0 = read_be(a6, 2);
            const unsigned st = read_be(a6 + 4U, 2);
            const unsigned caller = g_5670.caller_pc ? g_5670.caller_pc : (read_be(sp, 4) & 0xFFFFFFU);
            auto& hs = g_handlers[next_pc];
            hs.entries++;
            hs.callers[caller]++;
            hs.indices[idx]++;
            hs.entity_w0[w0]++;
            hs.entity_states[st]++;
            hs.call_depths[count_call_depth(sp)]++;

            ActiveHandler ah{};
            ah.callee_pc = next_pc;
            ah.caller_pc = caller;
            ah.stop_sp = sp + 4U;
            ah.start_cycles = g_cycles();
            g_handler_stack.push_back(ah);
        }
    } else if ((type & HOOK_M68K_W) != 0) {
        if (!g_handler_stack.empty()) g_handler_stack.back().writes++;
        if (g_5670.active) g_5670.writes++;
    }
}

void write_map(std::ostream& os, const std::map<unsigned, std::uint64_t>& m) {
    os << '{';
    bool first = true;
    for (const auto& [k, v] : m) {
        if (!first) os << ',';
        first = false;
        os << "\"0x" << std::hex << k << "\":" << std::dec << v;
    }
    os << '}';
}

void write_dec_map(std::ostream& os, const std::map<unsigned, std::uint64_t>& m) {
    os << '{';
    bool first = true;
    for (const auto& [k, v] : m) {
        if (!first) os << ',';
        first = false;
        os << '"' << k << "\":" << v;
    }
    os << '}';
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "Usage: m15_245_gpgx_profiler <core.so> <rom> <state> <polls> <frames> <out.json>\n";
        return 2;
    }
    oasis::hybrid::ExternalLibrary core(argv[1]);
    const auto rom = oasis::Rom::load(argv[2]);
    auto retro_set_env = core.get<void(*)(retro_environment_t)>("retro_set_environment");
    auto retro_set_vid = core.get<void(*)(retro_video_refresh_t)>("retro_set_video_refresh");
    auto retro_set_aud = core.get<void(*)(retro_audio_sample_t)>("retro_set_audio_sample");
    auto retro_set_aub = core.get<void(*)(retro_audio_sample_batch_t)>("retro_set_audio_sample_batch");
    auto retro_set_inp = core.get<void(*)(retro_input_poll_t)>("retro_set_input_poll");
    auto retro_set_ins = core.get<void(*)(retro_input_state_t)>("retro_set_input_state");
    auto retro_init = core.get<void(*)()>("retro_init");
    auto retro_load_game = core.get<bool(*)(const retro_game_info*)>("retro_load_game");
    auto retro_run = core.get<void(*)()>("retro_run");
    auto retro_unserialize = core.get<bool(*)(const void*, std::size_t)>("retro_unserialize");
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
    std::vector<char> state((std::istreambuf_iterator<char>(st_f)), {});
    if (!retro_unserialize(state.data(), state.size())) return 4;

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
    for (unsigned f = 0; f < frames; ++f) {
        g_frame = f; g_poll_idx = 0; g_frame_polls = 0;
        retro_run();
        g_journal->end_frame(f, g_frame_polls);
    }

    std::vector<std::pair<std::uint64_t, unsigned>> top_pcs;
    top_pcs.reserve(8192);
    for (unsigned i = 0; i < g_rom_pc_counts.size(); ++i) {
        const auto c = g_rom_pc_counts[i];
        if (!c) continue;
        const unsigned pc = i << 1U;
        if ((pc >= 0x008ACAU && pc <= 0x008AD0U) || (pc >= 0x008B72U && pc <= 0x008B7AU)) continue;
        top_pcs.push_back({c, pc});
    }
    std::partial_sort(top_pcs.begin(), top_pcs.begin() + std::min<std::size_t>(200, top_pcs.size()), top_pcs.end(),
                      [](auto a, auto b) { return a.first > b.first; });
    if (top_pcs.size() > 200) top_pcs.resize(200);

    std::ofstream out(argv[6]);
    out << "{\n"
        << "  \"frames\": " << frames << ",\n"
        << "  \"total_original_steps\": " << g_total_steps << ",\n"
        << "  \"idle_steps\": " << g_idle_steps << ",\n"
        << "  \"non_idle_steps\": " << (g_total_steps - g_idle_steps) << ",\n"
        << "  \"caller_005670\": {\"entries\": " << g_5670_entries << ", \"from_005586\": " << g_5670_from_5586
        << ", \"from_other\": " << g_5670_other_caller << ", \"to_0059b8_jmp\": " << g_5670_to_59b8
        << ", \"early_rts\": " << g_5670_early_rts << ", \"nonlocal_exits\": " << g_5670_nonlocal
        << ", \"steps\": " << g_5670_steps << ", \"callers\": ";
    write_map(out, g_5670_callers);
    out << ", \"exits\": ";
    write_map(out, g_5670_exits);
    out << "},\n"
        << "  \"dispatcher_0059a8\": {\"entries\": " << g_59a8_entries << ", \"direct\": " << g_59a8_direct
        << ", \"via_005670\": " << g_59a8_via_5670 << ", \"direct_callers\": ";
    write_map(out, g_59a8_callers);
    out << "},\n"
        << "  \"timer_00e682\": {\"entries\": " << g_e682_entries << ", \"normal_rts\": " << g_e682_normal_rts
        << ", \"nonlocal_rts\": " << g_e682_nonlocal_rts << ", \"callers\": ";
    write_map(out, g_e682_callers);
    out << ", \"nonlocal_return_pcs\": ";
    write_map(out, g_e682_ret_pcs);
    out << "},\n  \"table_handlers\": {\n";
    bool first_h = true;
    for (const auto& [pc, hs] : g_handlers) {
        if (!first_h) out << ",\n";
        first_h = false;
        out << "    \"0x" << std::hex << pc << "\": {\"entries\": " << std::dec << hs.entries
            << ", \"dynamic_steps\": " << hs.dynamic_steps
            << ", \"unique_pc_count\": " << hs.unique_pcs.size()
            << ", \"guest_cycles\": " << (hs.master_cycles / 7U)
            << ", \"ram_writes\": " << hs.ram_writes
            << ", \"nonlocal_returns\": " << hs.nonlocal_returns
            << ", \"table_indices\": ";
        write_dec_map(out, hs.indices);
        out << ", \"callers\": ";
        write_map(out, hs.callers);
        out << ", \"entity_w0\": ";
        write_map(out, hs.entity_w0);
        out << ", \"entity_states\": ";
        write_map(out, hs.entity_states);
        out << ", \"return_pcs\": ";
        write_map(out, hs.return_pcs);
        out << ", \"call_depths\": ";
        write_dec_map(out, hs.call_depths);
        out << "}";
    }
    out << "\n  },\n  \"top_non_idle_pcs\": [\n";
    for (std::size_t i = 0; i < top_pcs.size(); ++i) {
        out << (i ? ",\n" : "") << "    {\"pc\": \"0x" << std::hex << top_pcs[i].second << "\", \"count\": " << std::dec << top_pcs[i].first << "}";
    }
    out << "\n  ],\n  \"entity_region_pc_counts\": {";
    bool first_rpc = true;
    for (unsigned pc = 0x005500U; pc < 0x008A00U; pc += 2U) {
        const auto c = g_rom_pc_counts[pc >> 1U];
        if (!c) continue;
        if (!first_rpc) out << ",";
        first_rpc = false;
        out << "\"0x" << std::hex << pc << "\":" << std::dec << c;
    }
    out << "}\n}\n";
    return 0;
}
