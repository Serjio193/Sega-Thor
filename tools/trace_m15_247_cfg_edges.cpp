#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/gameplay_session.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_router.hpp"
#include "core/rom.hpp"
#include "cpuhook.h"
#include <libretro.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace {
unsigned (*g_cycles)(){};
oasis::hybrid::session::InputJournal* g_j{};
unsigned g_f{}, g_pi{}, g_fp{}, g_mask{}, g_prev_pc{};
bool g_prev_in{}, g_in_irq{};
std::uint32_t g_vbl_vec{}, g_hbl_vec{};
std::vector<std::uint8_t> g_rom;
std::set<std::pair<unsigned, unsigned>> g_edges_hit;
std::set<unsigned> g_pcs_hit;

bool env(unsigned cmd, void* data) {
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) { *static_cast<retro_pixel_format*>(data) = RETRO_PIXEL_FORMAT_RGB565; return true; }
    if (cmd == RETRO_ENVIRONMENT_GET_CAN_DUPE) { *static_cast<bool*>(data) = true; return true; }
    if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) { *static_cast<const char**>(data) = "/tmp"; return true; }
    return cmd == RETRO_ENVIRONMENT_SET_VARIABLES || cmd == RETRO_ENVIRONMENT_SET_MEMORY_MAPS ||
           cmd == RETRO_ENVIRONMENT_SET_GEOMETRY || cmd == RETRO_ENVIRONMENT_SET_CONTROLLER_INFO ||
           cmd == RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS || cmd == RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE;
}
void vid(const void*, unsigned, unsigned, std::size_t) {}
void aud(std::int16_t, std::int16_t) {}
std::size_t aub(const std::int16_t*, std::size_t c) { return c; }
void inp() {
    if (g_pi && g_fp == g_pi) g_j->end_poll();
    auto src = g_f < 1800 ? oasis::hybrid::session::InputSource::FallbackSchedule
                          : oasis::hybrid::session::InputSource::InjectedScripted;
    g_mask = g_j->begin_poll(g_f, g_pi, 0, src);
    ++g_pi; ++g_fp;
}
std::int16_t ins(unsigned p, unsigned d, unsigned ix, unsigned id) {
    return g_j->record_query({(uint8_t)p,(uint8_t)d,(uint16_t)ix,(uint16_t)id}, (p==0&&d==1&&id<16&&(g_mask&(1U<<id)))?1:0);
}
void hook(hook_type_t t, int, unsigned a, unsigned) {
    if (t != HOOK_M68K_E) return;
    const unsigned pc = a & 0xFFFFFFU;
    if ((pc == g_vbl_vec && g_vbl_vec) || (pc == g_hbl_vec && g_hbl_vec)) { g_in_irq = true; g_prev_in = false; return; }
    if (g_in_irq) {
        if (pc + 1U < g_rom.size() && g_rom[pc] == 0x4EU && g_rom[pc + 1U] == 0x73U) g_in_irq = false;
        return;
    }
    const bool in246 = oasis::recompiler::ir::is_compiled_exp246_dispatch_pc(pc);
    if (g_prev_in) g_edges_hit.insert({g_prev_pc, pc});
    if (in246) g_pcs_hit.insert(pc);
    g_prev_in = in246;
    g_prev_pc = pc;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 7) return 2;
    oasis::hybrid::ExternalLibrary core(argv[1]);
    auto rom = oasis::Rom::load(argv[2]);
    g_rom.assign(rom.bytes().begin(), rom.bytes().end());
    auto rd32 = [&](std::size_t o) { return (uint32_t(g_rom[o])<<24)|(uint32_t(g_rom[o+1])<<16)|(uint32_t(g_rom[o+2])<<8)|uint32_t(g_rom[o+3]); };
    g_hbl_vec = rd32(0x70) & 0xFFFFFFU; g_vbl_vec = rd32(0x78) & 0xFFFFFFU;
    core.get<void(*)(retro_environment_t)>("retro_set_environment")(env);
    core.get<void(*)(retro_video_refresh_t)>("retro_set_video_refresh")(vid);
    core.get<void(*)(retro_audio_sample_t)>("retro_set_audio_sample")(aud);
    core.get<void(*)(retro_audio_sample_batch_t)>("retro_set_audio_sample_batch")(aub);
    core.get<void(*)(retro_input_poll_t)>("retro_set_input_poll")(inp);
    core.get<void(*)(retro_input_state_t)>("retro_set_input_state")(ins);
    core.get<void(*)()>("retro_init")();
    retro_game_info gi{argv[2], rom.bytes().data(), rom.bytes().size()};
    if (!core.get<bool(*)(const retro_game_info*)>("retro_load_game")(&gi)) return 3;
    std::ifstream sf(argv[3], std::ios::binary);
    std::vector<char> st((std::istreambuf_iterator<char>(sf)), {});
    if (!core.get<bool(*)(const void*, std::size_t)>("retro_unserialize")(st.data(), st.size())) return 4;
    auto j = oasis::hybrid::session::InputJournal::load(argv[4]);
    g_j = &j;
    const unsigned frames = static_cast<unsigned>(std::stoul(argv[5]));
    core.get<void(*)(void(*)(hook_type_t,int,unsigned,unsigned))>("retro_hybrid_install")(hook);
    auto run = core.get<void(*)()>("retro_run");
    for (g_f = 0; g_f < frames; ++g_f) { g_pi = 0; g_fp = 0; g_prev_in = false; run(); j.end_frame(g_f, g_fp); }
    std::ofstream out(argv[6]);
    out << "{\"pcs\":[";
    bool first = true;
    for (unsigned p : g_pcs_hit) { out << (first ? "" : ",") << "\"0x" << std::hex << p << "\""; first = false; }
    out << "],\"edges\":[";
    first = true;
    for (auto [u, v] : g_edges_hit) { out << (first ? "" : ",") << "[\"0x" << std::hex << u << "\",\"0x" << v << "\"]"; first = false; }
    out << "]}\n";
    return 0;
}
