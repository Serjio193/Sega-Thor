#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/gameplay_session.hpp"
#include "core/rom.hpp"
#include "cpuhook.h"
#include <libretro.h>
#include <array>
#include <cstdint>
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
unsigned (*g_cycles)(){};
void (*g_stop)(){};

oasis::hybrid::session::InputJournal* g_journal{};
unsigned g_frame{}, g_poll_idx{}, g_frame_polls{}, g_cur_mask{};
std::vector<std::uint8_t> g_rom_bytes;
std::uint32_t g_vbl_vec{}, g_hbl_vec{};
int g_irq_depth{};
unsigned g_prev_pc{}, g_cur_insn_pc{};

unsigned read_be(unsigned addr, int bytes) {
    unsigned v = 0;
    for (int i = 0; i < bytes; ++i) {
        const int b = g_peek((addr + static_cast<unsigned>(i)) & 0xFFFFFFU);
        v = (v << 8U) | (b >= 0 ? static_cast<unsigned>(b) & 0xFFU : 0U);
    }
    return v;
}

bool is_compiled_m244_or_m245_pc(unsigned pc) {
    pc &= 0xFFFFFFU;
    if (pc == 0x005EE8U || pc == 0x006E86U || pc == 0x007F82U || pc == 0x007FB6U || pc == 0x00822CU) return false;
    return (pc >= 0x0059A8U && pc < 0x0059BAU) || (pc >= 0x005C1AU && pc < 0x005DE4U) || (pc >= 0x00780EU && pc < 0x007852U) ||
           (pc >= 0x005E78U && pc < 0x005EE8U) || (pc >= 0x0061F6U && pc < 0x0062CCU) || (pc >= 0x006338U && pc < 0x00638CU) ||
           (pc >= 0x006464U && pc < 0x0064C4U) || (pc >= 0x0066AAU && pc < 0x006738U) || (pc >= 0x006738U && pc < 0x0067DEU) ||
           (pc >= 0x006E28U && pc < 0x006E2AU) || (pc >= 0x006E4AU && pc < 0x006E86U) || (pc >= 0x006EDAU && pc < 0x006F3CU) ||
           (pc >= 0x007112U && pc < 0x007198U) || (pc >= 0x0071DAU && pc < 0x007262U) || (pc >= 0x007656U && pc < 0x00780EU) ||
           (pc >= 0x007852U && pc < 0x0078F8U) || (pc >= 0x0078F8U && pc < 0x00792AU) || (pc >= 0x007DC8U && pc < 0x007EC4U);
}

bool environment(unsigned cmd, void* data) {
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) { *static_cast<retro_pixel_format*>(data) = RETRO_PIXEL_FORMAT_RGB565; return true; }
    if (cmd == RETRO_ENVIRONMENT_GET_CAN_DUPE) { *static_cast<bool*>(data) = true; return true; }
    if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) { *static_cast<const char**>(data) = "/tmp"; return true; }
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
struct AuthCapture {
    bool active{};
    unsigned frame{}, entry_pc{}, entry_cycles{}, stop_sp{};
    std::array<unsigned, 8> d{}, a{};
    unsigned sr{};
    std::array<unsigned, 4> stack_before{}, stack_after{};
    std::vector<std::pair<unsigned, std::uint8_t>> ram_pre;
    std::vector<unsigned> pc_trace;
    std::vector<WriteRec> writes;
} g_auth;

std::vector<std::string> g_auth_logs;
std::set<unsigned> g_captured_roots;
bool g_watch_mode{};

std::string hex_arr(const unsigned* p, int n) {
    std::ostringstream os; os << '[';
    for (int i = 0; i < n; ++i) { if (i) os << ','; os << "\"0x" << std::hex << p[i] << std::dec << '"'; }
    os << ']'; return os.str();
}

void dump_auth(unsigned exit_pc, const char* cont_type) {
    std::array<unsigned, 8> ed{}, ea{};
    for (int i = 0; i < 8; ++i) { ed[i] = g_reg(i); ea[i] = g_reg(8 + i); }
    for (int i = 0; i < 4; ++i) g_auth.stack_after[i] = read_be(ea[7] + static_cast<unsigned>(i * 4), 4);
    std::ostringstream os;
    os << "{\"classification\":\"AUTHENTIC_GAMEPLAY_ENTRY\",\"frame\":" << g_auth.frame
       << ",\"entry_pc\":\"0x" << std::hex << g_auth.entry_pc << "\",\"exit_pc\":\"0x" << exit_pc << '"'
       << ",\"continuation_type\":\"" << cont_type << "\",\"cycles\":" << std::dec << ((g_cycles() - g_auth.entry_cycles) / 7U)
       << ",\"entry_d\":" << hex_arr(g_auth.d.data(), 8) << ",\"entry_a\":" << hex_arr(g_auth.a.data(), 8)
       << ",\"entry_sr\":\"0x" << std::hex << g_auth.sr << "\",\"entry_stack\":" << hex_arr(g_auth.stack_before.data(), 4)
       << ",\"exit_d\":" << hex_arr(ed.data(), 8) << ",\"exit_a\":" << hex_arr(ea.data(), 8)
       << ",\"exit_sr\":\"0x" << std::hex << g_reg(17) << "\",\"exit_stack\":" << hex_arr(g_auth.stack_after.data(), 4)
       << ",\"pc_trace\":" << hex_arr(g_auth.pc_trace.data(), static_cast<int>(g_auth.pc_trace.size()))
       << ",\"writes\":[";
    for (std::size_t i = 0; i < g_auth.writes.size(); ++i) {
        if (i) os << ',';
        os << "{\"pc\":\"0x" << std::hex << g_auth.writes[i].pc << "\",\"addr\":\"0x" << g_auth.writes[i].addr
           << "\",\"w\":" << std::dec << g_auth.writes[i].width << ",\"val\":\"0x" << std::hex << g_auth.writes[i].val << "\"}";
    }
    os << "],\"ram_pre\":[";
    for (std::size_t i = 0; i < g_auth.ram_pre.size(); ++i) {
        if (i) os << ',';
        os << "[\"0x" << std::hex << g_auth.ram_pre[i].first << "\"," << std::dec << static_cast<int>(g_auth.ram_pre[i].second) << "]";
    }
    os << "]}";
    g_auth_logs.push_back(os.str());
    g_auth.active = false;
}

bool want_entry(unsigned pc) {
    return pc == 0x0061F6U || pc == 0x0061FEU || pc == 0x006338U || pc == 0x006464U ||
           pc == 0x0066AAU || pc == 0x006738U || pc == 0x006EDAU || pc == 0x007112U ||
           pc == 0x007124U || pc == 0x0071DAU || pc == 0x007656U || pc == 0x007852U ||
           pc == 0x0078F8U || pc == 0x007DC8U || pc == 0x006E4AU || pc == 0x005E78U;
}

void snap_range(std::vector<std::pair<unsigned, std::uint8_t>>& dst, std::set<unsigned>& seen_a, unsigned s, unsigned len) {
    for (unsigned i = 0; i < len; ++i) {
        const unsigned addr = (s + i) & 0xFFFFFFU;
        if (addr < 0xFF0000U || seen_a.count(addr)) continue;
        const int v = g_peek(addr);
        if (v > 0) { dst.push_back({addr, static_cast<std::uint8_t>(v & 0xFF)}); seen_a.insert(addr); }
    }
}

void replay_hook(hook_type_t type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    if (!g_watch_mode) return;
    if (type == HOOK_M68K_E) {
        g_cur_insn_pc = address;
        if ((address == g_vbl_vec && g_vbl_vec) || (address == g_hbl_vec && g_hbl_vec)) {
            ++g_irq_depth;
            if (g_auth.active) g_auth.active = false;
            return;
        }
        if (g_irq_depth > 0) return;

        if (!g_auth.active && g_auth_logs.size() < 48U && want_entry(address) && g_captured_roots.count(address) == 0U) {
            g_captured_roots.insert(address);
            g_auth = AuthCapture{};
            g_auth.active = true; g_auth.frame = g_frame; g_auth.entry_pc = address;
            g_auth.entry_cycles = g_cycles(); g_auth.stop_sp = g_reg(15) + 4U;
            for (int i = 0; i < 8; ++i) { g_auth.d[i] = g_reg(i); g_auth.a[i] = g_reg(8 + i); }
            g_auth.sr = g_reg(17);
            for (int i = 0; i < 4; ++i) g_auth.stack_before[i] = read_be(g_auth.a[7] + static_cast<unsigned>(i * 4), 4);
            std::set<unsigned> seen_a;
            snap_range(g_auth.ram_pre, seen_a, g_auth.a[6], 0xC0U);
            snap_range(g_auth.ram_pre, seen_a, g_auth.a[7], 0x20U);
            snap_range(g_auth.ram_pre, seen_a, 0xFF0D80U, 0x0E0U);
            snap_range(g_auth.ram_pre, seen_a, 0xFF1650U, 0x0B0U);
            snap_range(g_auth.ram_pre, seen_a, 0xFF17B0U, 0x020U);
            snap_range(g_auth.ram_pre, seen_a, 0xFF1930U, 0x180U);
        }
        if (g_auth.active && g_auth.pc_trace.size() < 256) g_auth.pc_trace.push_back(address);
        g_prev_pc = address;
    } else if (type == static_cast<hook_type_t>(kPostHook)) {
        if (g_irq_depth > 0) {
            if (g_cur_insn_pc + 1U < g_rom_bytes.size() && g_rom_bytes[g_cur_insn_pc] == 0x4EU && g_rom_bytes[g_cur_insn_pc + 1U] == 0x73U)
                --g_irq_depth;
            return;
        }
        if (g_auth.active) {
            const unsigned next_pc = g_reg(16) & 0xFFFFFFU;
            const unsigned sp = g_reg(15);
            if (sp >= g_auth.stop_sp) dump_auth(next_pc, "CompiledNextPc");
            else if (!is_compiled_m244_or_m245_pc(next_pc)) dump_auth(next_pc, "ValidUncompiledPc");
            else if (g_auth.pc_trace.size() >= 250) dump_auth(next_pc, "TraceLimit");
        }
    } else if ((type & HOOK_M68K_W) != 0 && g_irq_depth == 0 && g_auth.active) {
        if (g_auth.writes.size() < 128) g_auth.writes.push_back({g_cur_insn_pc, address, width, value});
    }
}

bool g_probe_done{};
struct ProbeTrace {
    unsigned stop_pc{}, stop_sp{}, max_steps{500};
    std::vector<unsigned> pc_trace, cyc_trace;
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
            g_probe.exit_pc = kProbeHandlerAddr; g_probe.exit_kind = "OriginalCpuException";
            g_probe.end_cyc = g_cycles(); g_probe_done = true; g_stop(); return;
        }
        g_probe.last_insn_cyc = g_cycles();
        if (g_probe.pc_trace.empty()) g_probe.start_cyc = g_probe.last_insn_cyc;
        if (g_probe.pc_trace.size() < g_probe.max_steps) g_probe.pc_trace.push_back(address);
    } else if (type == static_cast<hook_type_t>(kPostHook)) {
        const unsigned now = g_cycles();
        if (g_probe.cyc_trace.size() < g_probe.pc_trace.size()) g_probe.cyc_trace.push_back((now - g_probe.last_insn_cyc) / 7U);
        const unsigned next_pc = g_reg(16) & 0xFFFFFFU;
        if (next_pc == kProbeHandlerAddr) {
            g_probe.exit_pc = next_pc; g_probe.exit_kind = "OriginalCpuException";
            g_probe.end_cyc = now; g_probe_done = true; g_stop();
        } else if (next_pc == g_probe.stop_pc ||
                   (g_probe.stop_sp && g_reg(15) >= g_probe.stop_sp) ||
                   g_probe.pc_trace.size() >= g_probe.max_steps) {
            g_probe.exit_pc = next_pc; g_probe.end_cyc = now; g_probe_done = true; g_stop();
        }
    } else if ((type & HOOK_M68K_W) != 0) {
        g_probe.writes.push_back({g_cur_insn_pc, address, width, value});
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 5) return 2;
    const std::string cmd = argv[1];
    oasis::hybrid::ExternalLibrary core(argv[2]);
    const auto rom = oasis::Rom::load(argv[3]);
    g_rom_bytes.assign(rom.bytes().begin(), rom.bytes().end());
    auto rd32 = [&](std::size_t off) {
        return (std::uint32_t(g_rom_bytes[off]) << 24) | (std::uint32_t(g_rom_bytes[off + 1]) << 16) |
               (std::uint32_t(g_rom_bytes[off + 2]) << 8) | std::uint32_t(g_rom_bytes[off + 3]);
    };
    g_hbl_vec = rd32(0x70) & 0xFFFFFFU; g_vbl_vec = rd32(0x78) & 0xFFFFFFU;

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
    g_cycles = core.get<unsigned(*)()>("retro_9c40_capture_cycles");
    g_stop = core.get<void(*)()>("retro_9c40_capture_stop");

    retro_set_env(environment); retro_set_vid(video_cb); retro_set_aud(audio_cb);
    retro_set_aub(audio_batch_cb); retro_set_inp(input_poll_cb); retro_set_ins(input_state_cb);
    retro_init();
    retro_game_info gi{}; gi.path = argv[3]; gi.data = rom.bytes().data(); gi.size = rom.bytes().size();
    if (!retro_load_game(&gi)) return 3;

    if (cmd == "replay-trace") {
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
        out << "{\"frames\":" << frames << ",\"authentic_captures\":[";
        for (std::size_t i = 0; i < g_auth_logs.size(); ++i) out << (i ? "," : "") << g_auth_logs[i];
        out << "]}\n";
    } else if (cmd == "probe-case") {
        auto fix_prep = core.get<int(*)(unsigned,unsigned,unsigned,int,int,int,unsigned)>("retro_hybrid_fixture_prepare");
        auto patch_rom = core.get<int(*)(unsigned,unsigned)>("retro_hybrid_fixture_patch_rom");
        auto vp_h = core.get<unsigned(*)()>("retro_hybrid_fixture_viewport_height");
        auto fix_run = core.get<void(*)()>("retro_hybrid_fixture_run_frame");
        for (unsigned vec : {8U, 12U}) for (int b = 0; b < 4; ++b) patch_rom(vec + b, (kProbeHandlerAddr >> (24 - 8 * b)) & 0xFFU);
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
            const auto eq = kv.find('='), colon = kv.find(':');
            g_poke(std::stoul(kv.substr(0, colon), nullptr, 16), std::stoi(kv.substr(colon + 1, eq - colon - 1)), std::stoul(kv.substr(eq + 1), nullptr, 16));
        }
        g_poke(g_probe.stop_pc, 2, 0x60FE);
        g_probe_done = false;
        retro_hybrid_install(probe_hook);
        fix_run();
        std::array<unsigned, 8> ed{}, ea{};
        for (int i = 0; i < 8; ++i) { ed[i] = g_reg(i); ea[i] = g_reg(8 + i); }
        std::cout << "{\"exit_pc\":\"0x" << std::hex << g_probe.exit_pc << "\",\"outcome\":\"" << g_probe.exit_kind
                  << "\",\"cycles\":" << std::dec << ((g_probe.end_cyc - g_probe.start_cyc) / 7U)
                  << ",\"exit_d\":" << hex_arr(ed.data(), 8) << ",\"exit_a\":" << hex_arr(ea.data(), 8)
                  << ",\"exit_sr\":\"0x" << std::hex << g_reg(17)
                  << "\",\"pc_trace\":" << hex_arr(g_probe.pc_trace.data(), static_cast<int>(g_probe.pc_trace.size()))
                  << ",\"insn_cycles\":" << '[' << std::dec;
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
