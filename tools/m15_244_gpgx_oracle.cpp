#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/gameplay_session.hpp"
#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "cpuhook.h"
#include <libretro.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr unsigned kPostHook = 1U << 14;
constexpr unsigned kProbeStopBase = 0x00FF2000U;
constexpr unsigned kProbeHandlerAddr = 0x00FF2200U;

unsigned (*g_reg)(unsigned){};
void (*g_set_reg)(unsigned, unsigned){};
int (*g_peek)(unsigned){};
void (*g_poke)(unsigned, int, unsigned){};
int (*g_sched)(unsigned){};
unsigned (*g_cycles)(){};
void (*g_stop)(){};

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

struct WriteRec { unsigned pc, addr; int width; unsigned val; };
struct StackFrame {
    unsigned caller_pc, entry_pc, entry_a7, entry_cycles, frame;
    std::array<unsigned, 4> stack_before{};
    std::array<unsigned, 4> stack_after{};
    std::vector<WriteRec> writes;
    bool saw_addq_a7{};
    unsigned rts_pc_at{};
    unsigned call_depth_before{};
    unsigned instr_count{};
};
std::vector<StackFrame> g_e682_stack;
std::vector<std::string> g_e682_proofs;

struct AuthCapture {
    bool active{};
    unsigned frame{}, entry_pc{}, entry_cycles{}, prev_pc{}, stop_sp{};
    std::array<unsigned, 8> d{}, a{};
    unsigned sr{}, ssp{}, usp{};
    std::array<unsigned, 4> stack_words{};
    std::array<std::uint8_t, 0xBC> entity_ram{};
    std::vector<unsigned> pc_trace;
    std::vector<WriteRec> writes;
    std::vector<std::pair<unsigned, unsigned>> globals_pre;
};
AuthCapture g_auth;
std::vector<std::string> g_auth_logs;
std::set<unsigned> g_seen_dispatch_pcs;
unsigned g_prev_pc{};
unsigned g_cur_insn_pc{};
bool g_watch_mode{};

std::string hex_arr(const unsigned* p, int n) {
    std::ostringstream os; os << '[';
    for (int i = 0; i < n; ++i) {
        if (i) os << ',';
        os << "\"0x" << std::hex << p[i] << std::dec << '"';
    }
    os << ']';
    return os.str();
}

void dump_e682_proof(const StackFrame& sf, unsigned exit_pc, unsigned exit_a7, unsigned exit_cyc) {
    std::array<unsigned, 4> after_words{};
    for (int i = 0; i < 4; ++i) after_words[i] = read_be(exit_a7 + static_cast<unsigned>(i * 4), 4);
    const std::uint32_t master_delta = exit_cyc - sf.entry_cycles;
    std::ostringstream os;
    os << "{\"frame\":" << sf.frame
       << ",\"caller_pc\":\"0x" << std::hex << sf.caller_pc << '"'
       << ",\"entry_pc\":\"0x" << sf.entry_pc << '"'
       << ",\"entry_a7\":\"0x" << sf.entry_a7 << '"'
       << ",\"stack_words_before\":" << hex_arr(sf.stack_before.data(), 4)
       << ",\"skipped_longword\":\"0x" << sf.stack_before[0] << '"'
       << ",\"skipped_return_pc\":\"0x" << (sf.stack_before[0] & 0xFFFFFFU) << '"'
       << ",\"actual_rts_target_pc\":\"0x" << exit_pc << '"'
       << ",\"exit_a7\":\"0x" << exit_a7 << '"'
       << ",\"stack_words_after\":" << hex_arr(after_words.data(), 4)
       << ",\"a7_delta\":" << std::dec << static_cast<int>(exit_a7 - sf.entry_a7)
       << ",\"call_depth_before\":" << sf.call_depth_before
       << ",\"call_depth_after\":" << (sf.call_depth_before >= 2U ? sf.call_depth_before - 2U : 0U)
       << ",\"master_cycles\":" << master_delta
       << ",\"m68k_cycles\":" << (master_delta / 7U)
       << ",\"instructions\":" << sf.instr_count
       << ",\"writes\":[";
    for (std::size_t i = 0; i < sf.writes.size(); ++i) {
        if (i) os << ',';
        os << "{\"addr\":\"0x" << std::hex << sf.writes[i].addr << "\",\"w\":" << std::dec << sf.writes[i].width
           << ",\"val\":\"0x" << std::hex << sf.writes[i].val << "\"}";
    }
    os << std::dec << "]}";
    g_e682_proofs.push_back(os.str());
}

void dump_auth_capture(unsigned exit_pc, const char* cont_type) {
    std::ostringstream os;
    os << "{\"classification\":\"AUTHENTIC_GAMEPLAY_ENTRY\",\"frame\":" << g_auth.frame
       << ",\"entry_pc\":\"0x" << std::hex << g_auth.entry_pc << '"'
       << ",\"exit_pc\":\"0x" << exit_pc << '"'
       << ",\"continuation_type\":\"" << cont_type << '"'
       << ",\"cycles\":" << std::dec << (g_cycles() - g_auth.entry_cycles)
       << ",\"entry_d\":" << hex_arr(g_auth.d.data(), 8)
       << ",\"entry_a\":" << hex_arr(g_auth.a.data(), 8)
       << ",\"entry_sr\":\"0x" << std::hex << g_auth.sr << '"'
       << ",\"entry_stack\":" << hex_arr(g_auth.stack_words.data(), 4)
       << ",\"exit_d\":";
    std::array<unsigned, 8> ed{}, ea{};
    for (int i = 0; i < 8; ++i) { ed[i] = g_reg(i); ea[i] = g_reg(8 + i); }
    os << hex_arr(ed.data(), 8) << ",\"exit_a\":" << hex_arr(ea.data(), 8)
       << ",\"exit_sr\":\"0x" << std::hex << g_reg(17) << '"'
       << ",\"pc_trace\":" << hex_arr(g_auth.pc_trace.data(), static_cast<int>(g_auth.pc_trace.size()))
       << ",\"writes\":[";
    for (std::size_t i = 0; i < g_auth.writes.size(); ++i) {
        if (i) os << ',';
        os << "{\"addr\":\"0x" << std::hex << g_auth.writes[i].addr << "\",\"w\":" << std::dec << g_auth.writes[i].width
           << ",\"val\":\"0x" << std::hex << g_auth.writes[i].val << "\"}";
    }
    os << "],\"globals_pre\":[";
    for (std::size_t i = 0; i < g_auth.globals_pre.size(); ++i) {
        if (i) os << ',';
        os << "{\"addr\":\"0x" << std::hex << g_auth.globals_pre[i].first << "\",\"val\":\"0x" << g_auth.globals_pre[i].second << "\"}";
    }
    os << "],\"entity_ram_hex\":\"";
    for (auto b : g_auth.entity_ram) {
        const char* hx = "0123456789ABCDEF";
        os << hx[(b >> 4) & 0xF] << hx[b & 0xF];
    }
    os << std::dec << "\"}";
    g_auth_logs.push_back(os.str());
    g_auth.active = false;
}

void replay_hook(hook_type_t type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    if (!g_watch_mode) return;
    if (type == HOOK_M68K_E) {
        g_cur_insn_pc = address;
        if (address == 0x00E682U) {
            StackFrame sf{};
            sf.frame = g_frame; sf.caller_pc = g_prev_pc; sf.entry_pc = address;
            sf.entry_a7 = g_reg(15); sf.entry_cycles = g_cycles();
            for (int i = 0; i < 4; ++i) sf.stack_before[i] = read_be(sf.entry_a7 + static_cast<unsigned>(i * 4), 4);
            // Count authentic return frames up to stack anchor 0x00FF0BEA (or valid ROM stack longwords)
            unsigned depth = 0;
            for (unsigned sp = sf.entry_a7; sp < 0x00FF0C00U && depth < 16U; sp += 4U) {
                const unsigned lw = read_be(sp, 4) & 0xFFFFFFU;
                if (lw >= 0x000200U && lw < 0x300000U) ++depth;
            }
            sf.call_depth_before = depth;
            sf.instr_count = 1;
            g_e682_stack.push_back(sf);
        } else if (!g_e682_stack.empty()) {
            g_e682_stack.back().instr_count++;
            if (address == 0x00E6A6U) g_e682_stack.back().saw_addq_a7 = true;
            else if (address == 0x00E6A8U) g_e682_stack.back().rts_pc_at = address;
        }
        // Start authentic capture at 0x0059A8 (limit 1 per target/branch diversity + targets 5C1A, 5CC8, 5CF8, 5DC8, 5DE4)
        if (!g_auth.active && address == 0x0059A8U && g_auth_logs.size() < 40U) {
            const unsigned d0_w = static_cast<std::uint16_t>(read_be(g_reg(14) + 4U, 2));
            const bool interesting = (d0_w == 0x18 || d0_w == 0x1A || d0_w == 0x1C || d0_w == 0x1E ||
                                      d0_w == 0x20 || d0_w == 0x22 || d0_w == 0x2E || d0_w == 0x40 ||
                                      d0_w == 0x00 || d0_w == 0x02 || d0_w == 0x04 || d0_w == 0x30 || d0_w == 0x16);
            if (interesting && g_seen_dispatch_pcs.count((d0_w << 8U) | 1U) == 0U) {
                g_seen_dispatch_pcs.insert((d0_w << 8U) | 1U);
                g_auth = AuthCapture{};
                g_auth.active = true; g_auth.frame = g_frame; g_auth.entry_pc = address;
                g_auth.entry_cycles = g_cycles(); g_auth.stop_sp = g_reg(15) + 4U;
                for (int i = 0; i < 8; ++i) { g_auth.d[i] = g_reg(i); g_auth.a[i] = g_reg(8 + i); }
                g_auth.sr = g_reg(17);
                for (int i = 0; i < 4; ++i) g_auth.stack_words[i] = read_be(g_auth.a[7] + static_cast<unsigned>(i * 4), 4);
                for (unsigned k = 0; k < 0xBCU; ++k) g_auth.entity_ram[k] = static_cast<std::uint8_t>(g_peek((g_auth.a[6] + k) & 0xFFFFFFU));
                for (unsigned ga : {0xFF165EU, 0xFF1934U, 0xFF1970U, 0xFF1987U})
                    g_auth.globals_pre.push_back({ga, read_be(ga, 4)});
            }
        }
        if (g_auth.active) {
            if (g_auth.pc_trace.size() < 256) g_auth.pc_trace.push_back(address);
        }
        g_prev_pc = address;
    } else if (type == static_cast<hook_type_t>(kPostHook)) {
        const unsigned next_pc = g_reg(16) & 0xFFFFFFU;
        if (!g_e682_stack.empty()) {
            auto& sf = g_e682_stack.back();
            if (g_cur_insn_pc == 0x00E6A8U && sf.rts_pc_at == 0x00E6A8U) {
                if (sf.saw_addq_a7) dump_e682_proof(sf, next_pc, g_reg(15), g_cycles());
                g_e682_stack.pop_back();
            } else if (g_cur_insn_pc == 0x00E698U) {
                g_e682_stack.pop_back();
            }
        }
        if (g_auth.active) {
            const unsigned sp = g_reg(15);
            if (sp >= g_auth.stop_sp) {
                dump_auth_capture(next_pc, "CompiledNextPc");
            } else if (g_cur_insn_pc == 0x0059B8U) {
                const bool is_compiled_tgt = (next_pc == 0x005CC8U || next_pc == 0x005CF8U || next_pc == 0x005DC8U ||
                                              next_pc == 0x005C1AU || next_pc == 0x00780EU);
                if (!is_compiled_tgt) dump_auth_capture(next_pc, "ValidUncompiledPc");
            } else if (next_pc == 0x0062CCU || next_pc == 0x060004U || next_pc == 0x00B79AU || next_pc == 0x00BCBAU) {
                dump_auth_capture(next_pc, "ValidUncompiledPc");
            } else if (g_auth.pc_trace.size() >= 250) {
                dump_auth_capture(next_pc, "TraceLimit");
            }
        }
    } else if ((type & HOOK_M68K_W) != 0) {
        if (!g_e682_stack.empty() && g_cur_insn_pc >= 0x00E682U && g_cur_insn_pc < 0x00E6C0U) {
            g_e682_stack.back().writes.push_back({g_cur_insn_pc, address, width, value});
        }
        if (g_auth.active && g_auth.writes.size() < 128) g_auth.writes.push_back({g_cur_insn_pc, address, width, value});
    }
}

bool g_probe_done{};
struct ProbeTrace {
    unsigned stop_pc{}, stop_sp{}, max_steps{500};
    std::vector<unsigned> pc_trace;
    std::vector<unsigned> cyc_trace;
    std::vector<WriteRec> writes;
    unsigned start_cyc{}, end_cyc{}, last_insn_cyc{}, exit_pc{};
    std::string exit_kind{"CompiledNextPc"};
} g_probe;

void probe_hook(hook_type_t type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    if (g_probe_done) return;
    if (type == HOOK_M68K_E) {
        g_cur_insn_pc = address;
        if (address == kProbeHandlerAddr) {
            g_probe.exit_pc = kProbeHandlerAddr;
            g_probe.exit_kind = "OriginalCpuException";
            g_probe.end_cyc = g_cycles();
            g_probe_done = true;
            g_stop();
            return;
        }
        g_probe.last_insn_cyc = g_cycles();
        if (g_probe.pc_trace.empty()) g_probe.start_cyc = g_probe.last_insn_cyc;
        if (g_probe.pc_trace.size() < g_probe.max_steps) g_probe.pc_trace.push_back(address);
    } else if (type == static_cast<hook_type_t>(kPostHook)) {
        const unsigned now = g_cycles();
        if (g_probe.cyc_trace.size() < g_probe.pc_trace.size()) g_probe.cyc_trace.push_back((now - g_probe.last_insn_cyc) / 7U);
        const unsigned next_pc = g_reg(16) & 0xFFFFFFU;
        if (next_pc == kProbeHandlerAddr) {
            g_probe.exit_pc = next_pc;
            g_probe.exit_kind = "OriginalCpuException";
            g_probe.end_cyc = now;
            g_probe_done = true;
            g_stop();
        } else if (next_pc == g_probe.stop_pc ||
                   (g_probe.stop_sp && g_reg(15) >= g_probe.stop_sp && next_pc != kProbeHandlerAddr) ||
                   g_probe.pc_trace.size() >= g_probe.max_steps) {
            g_probe.exit_pc = next_pc;
            g_probe.end_cyc = now;
            g_probe_done = true;
            g_stop();
        }
    } else if ((type & HOOK_M68K_W) != 0) {
        g_probe.writes.push_back({g_cur_insn_pc, address, width, value});
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: m15_244_gpgx_oracle <replay-trace|probe-case> <core.so> <rom> <args...>\n";
        return 2;
    }
    const std::string cmd = argv[1];
    oasis::hybrid::ExternalLibrary core(argv[2]);
    const auto rom = oasis::Rom::load(argv[3]);
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
    g_set_reg = core.get<void(*)(unsigned, unsigned)>("retro_9c40_capture_set_register");
    g_peek = core.get<int(*)(unsigned)>("retro_9c40_capture_peek");
    g_poke = core.get<void(*)(unsigned, int, unsigned)>("retro_hybrid_poke");
    g_sched = core.get<int(*)(unsigned)>("retro_hybrid_scheduler_field");
    g_cycles = core.get<unsigned(*)()>("retro_9c40_capture_cycles");
    g_stop = core.get<void(*)()>("retro_9c40_capture_stop");

    retro_set_env(environment); retro_set_vid(video_cb); retro_set_aud(audio_cb);
    retro_set_aub(audio_batch_cb); retro_set_inp(input_poll_cb); retro_set_ins(input_state_cb);
    retro_init();
    retro_game_info gi{}; gi.path = argv[3]; gi.data = rom.bytes().data(); gi.size = rom.bytes().size();
    if (!retro_load_game(&gi)) return 3;

    if (cmd == "replay-trace") {
        // args: <state> <polls> <frames> <out.json>
        if (argc < 8) return 2;
        std::ifstream st_f(argv[4], std::ios::binary);
        std::vector<char> state((std::istreambuf_iterator<char>(st_f)), {});
        if (!retro_unserialize(state.data(), state.size())) return 4;
        auto journal = oasis::hybrid::session::InputJournal::load(argv[5]);
        g_journal = &journal;
        const unsigned frames = static_cast<unsigned>(std::stoul(argv[6]));
        retro_hybrid_install(replay_hook);
        g_watch_mode = true;
        for (unsigned f = 0; f < frames; ++f) {
            g_frame = f; g_poll_idx = 0; g_frame_polls = 0;
            retro_run();
            g_journal->end_frame(f, g_frame_polls);
        }
        std::ofstream out(argv[7]);
        out << "{\"frames\":" << frames << ",\"e682_nonlocal_proofs\":[";
        for (std::size_t i = 0; i < g_e682_proofs.size(); ++i) out << (i ? "," : "") << g_e682_proofs[i];
        out << "],\"authentic_dispatch_captures\":[";
        for (std::size_t i = 0; i < g_auth_logs.size(); ++i) out << (i ? "," : "") << g_auth_logs[i];
        out << "]}\n";
    } else if (cmd == "probe-case") {
        // args: <entry_pc_hex> <stop_pc_hex> <sr_hex> <d0..d7 hex> <a0..a7 hex> <addr=val ...>
        auto fix_prep = core.get<int(*)(unsigned,unsigned,unsigned,int,int,int,unsigned)>("retro_hybrid_fixture_prepare");
        auto patch_rom = core.get<int(*)(unsigned,unsigned)>("retro_hybrid_fixture_patch_rom");
        auto vp_h = core.get<unsigned(*)()>("retro_hybrid_fixture_viewport_height");
        auto fix_run = core.get<void(*)()>("retro_hybrid_fixture_run_frame");
        // Patch vectors 2 (bus error) & 3 (addr error) to kProbeHandlerAddr so we detect hardware exceptions
        for (unsigned vec : {8U, 12U}) {
            for (int b = 0; b < 4; ++b) patch_rom(vec + b, (kProbeHandlerAddr >> (24 - 8 * b)) & 0xFFU);
        }
        g_poke(kProbeHandlerAddr, 2, 0x60FE);
        const unsigned entry_pc = std::stoul(argv[4], nullptr, 16);
        g_probe.stop_pc = std::stoul(argv[5], nullptr, 16);
        const unsigned sr = std::stoul(argv[6], nullptr, 16);
        std::array<unsigned, 8> regs_d{}, regs_a{};
        for (int i = 0; i < 8; ++i) regs_d[i] = std::stoul(argv[7 + i], nullptr, 16);
        for (int i = 0; i < 8; ++i) regs_a[i] = std::stoul(argv[15 + i], nullptr, 16);
        g_probe.stop_sp = regs_a[7] + 4U;
        if (!fix_prep(entry_pc, regs_a[7], sr, 0, 100000, 0, vp_h())) return 5;
        for (int i = 0; i < 8; ++i) g_set_reg(i, regs_d[i]);
        for (int i = 0; i < 8; ++i) g_set_reg(8 + i, regs_a[i]);
        for (int arg = 23; arg < argc; ++arg) {
            std::string kv = argv[arg];
            const auto eq = kv.find('=');
            const auto colon = kv.find(':');
            const unsigned addr = std::stoul(kv.substr(0, colon), nullptr, 16);
            const int w = std::stoi(kv.substr(colon + 1, eq - colon - 1));
            const unsigned val = std::stoul(kv.substr(eq + 1), nullptr, 16);
            g_poke(addr, w, val);
        }
        g_poke(g_probe.stop_pc, 2, 0x60FE);
        g_probe_done = false;
        retro_hybrid_install(probe_hook);
        fix_run();
        std::array<unsigned, 8> ed{}, ea{};
        for (int i = 0; i < 8; ++i) { ed[i] = g_reg(i); ea[i] = g_reg(8 + i); }
        std::cout << "{\"exit_pc\":\"0x" << std::hex << g_probe.exit_pc << "\",\"outcome\":\"" << g_probe.exit_kind
                  << "\",\"cycles\":" << std::dec << ((g_probe.end_cyc - g_probe.start_cyc) / 7U)
                  << ",\"master_cycles\":" << (g_probe.end_cyc - g_probe.start_cyc)
                  << ",\"exit_d\":" << hex_arr(ed.data(), 8) << ",\"exit_a\":" << hex_arr(ea.data(), 8)
                  << ",\"exit_sr\":\"0x" << std::hex << g_reg(17)
                  << "\",\"pc_trace\":" << hex_arr(g_probe.pc_trace.data(), static_cast<int>(g_probe.pc_trace.size()))
                  << ",\"insn_cycles\":[" << std::dec;
        for (std::size_t i = 0; i < g_probe.cyc_trace.size(); ++i) std::cout << (i ? "," : "") << g_probe.cyc_trace[i];
        std::cout << "],\"writes\":[";
        for (std::size_t i = 0; i < g_probe.writes.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << "{\"addr\":\"0x" << std::hex << g_probe.writes[i].addr << "\",\"w\":" << std::dec << g_probe.writes[i].width
                      << ",\"val\":\"0x" << std::hex << g_probe.writes[i].val << "\"}";
        }
        std::cout << "]}\n";
    }
    return 0;
}
