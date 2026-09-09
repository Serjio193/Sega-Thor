#include "tools/hybrid/dispatch.hpp"
#include "tools/hybrid/candidate_2d66.hpp"
#include "tools/hybrid/candidate_604bc.hpp"
#include "tools/hybrid/candidate_61032.hpp"
#include "tools/hybrid/replacement.hpp"
#include "tools/hybrid/basic_block.hpp"
#include "tools/hybrid/generated_blocks.hpp"
#include "tools/hybrid/interpreter_profile.hpp"
#include "tools/hybrid/address_provenance.hpp"
#include "tools/hybrid/mechanical_primitive.hpp"
#include "tools/hybrid/runner_report.hpp"
#include "tools/hybrid/checkpoint_evidence.hpp"
#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include <libretro.h>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string_view>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {
class Library {
public:
    explicit Library(const char* path) {
#ifdef _WIN32
        handle_ = LoadLibraryA(path);
#else
        handle_ = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
        if (!handle_) throw std::runtime_error("cannot load external GPGX library");
    }
    template<class T> T get(const char* name) {
#ifdef _WIN32
        const auto symbol = GetProcAddress(handle_, name);
#else
        const auto symbol = dlsym(handle_, name);
#endif
        if (!symbol) throw std::runtime_error(std::string("missing GPGX symbol ") + name);
        return reinterpret_cast<T>(symbol);
    }
private:
#ifdef _WIN32
    HMODULE handle_{};
#else
    void* handle_{};
#endif
};
oasis::hybrid::Dispatch* dispatch{}; // One explicitly owned frontend session.
oasis::hybrid::Registry* registry_dispatch{};
oasis::hybrid::BasicBlockRegistry* block_registry{};
oasis::hybrid::MechanicalPrimitiveRegistry* primitive_registry{};
int (*cpu_cycles)(){};
unsigned (*gpgx_boundary_reason)(){};
unsigned bytes_per_pixel = 2;
unsigned video_frames{};
std::string video_hashes;
std::string directory;
bool discovery_mode{};
bool plain_emulated_mode{};
std::map<unsigned, unsigned> discovered_pcs;
std::map<unsigned, unsigned> observed_pcs;
std::unique_ptr<oasis::hybrid::AddressProvenanceObserver> address_observer;
unsigned current_frame{};
unsigned interpreter_instruction_executions{};
void hook(int type, int width, unsigned address, unsigned value) {
    address &= 0xFFFFFFU;
    if (type == 1) {
        ++observed_pcs[address];
        ++interpreter_instruction_executions;
    }
    if (address_observer) address_observer->event(type, width, address, value, current_frame);
    if (discovery_mode) {
        if (type == 1) ++discovered_pcs[address];
        return;
    }
    if (plain_emulated_mode) return;
    if (primitive_registry) primitive_registry->event(type, width, address, value);
    if (block_registry) block_registry->event(type, width, address, value);
    else if (registry_dispatch) registry_dispatch->hook(type, width, address, value);
    else dispatch->hook(type, width, address, value);
}
oasis::hybrid::BlockExitReason boundary_reason() noexcept {
    if (!gpgx_boundary_reason) return oasis::hybrid::BlockExitReason::FALLBACK;
    const auto value = gpgx_boundary_reason();
    return value <= static_cast<unsigned>(oasis::hybrid::BlockExitReason::FALLBACK) ?
        static_cast<oasis::hybrid::BlockExitReason>(value) :
        oasis::hybrid::BlockExitReason::FALLBACK;
}
int block_hook(unsigned address) {
    if (primitive_registry && primitive_registry->handles(address)) {
        const auto result = primitive_registry->dispatch(address);
        if (result) return result;
    }
    return block_registry ? block_registry->dispatch(address) : 0;
}
unsigned parse_target(std::string_view text) {
    std::size_t consumed = 0;
    const auto value = std::stoul(std::string(text), &consumed, 0);
    if (consumed != text.size() || value > 0xFFFFFF) throw std::runtime_error("invalid target");
    return value;
}
bool environment(unsigned command, void* data) {
    switch (command) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *static_cast<const char**>(data) = directory.c_str(); return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        bytes_per_pixel = *static_cast<retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2;
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *static_cast<bool*>(data) = true; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *static_cast<bool*>(data) = false; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE:
        static_cast<retro_variable*>(data)->value = nullptr; return false;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        return true;
    default: return false;
    }
}
void video(const void* data, unsigned width, unsigned height, std::size_t pitch) {
    ++video_frames;
    if (!data) { video_hashes += "DUP;"; return; }
    const auto* pixels = static_cast<const std::uint8_t*>(data);
    std::vector<std::uint8_t> packed;
    for (unsigned row = 0; row < height; ++row)
        packed.insert(packed.end(), pixels + row * pitch, pixels + row * pitch + width * bytes_per_pixel);
    video_hashes += std::to_string(width) + "x" + std::to_string(height) + ":" + oasis::calculate_sha256(packed);
}
void audio(std::int16_t, std::int16_t) {}
std::size_t audio_batch(const std::int16_t*, std::size_t frames) { return frames; }
void poll() {}
std::int16_t input(unsigned, unsigned, unsigned, unsigned) { return 0; }
std::string hash_text(const std::string& value) {
    return oasis::calculate_sha256({reinterpret_cast<const std::uint8_t*>(value.data()), value.size()});
}
bool checkpoint_evidence_enabled() {
    return std::getenv("OASIS_CHECKPOINT_EVIDENCE") != nullptr;
}
}

int main(int argc, char** argv) {
    if (argc != 6 && argc != 7) {
        std::cerr << "usage: oasis_hybrid_poc <GPGX library> <canonical ROM> "
                     "<mode> <frames:1..600> <output directory> [target]\n"
                     "modes: EMULATED, SHADOW_NATIVE, NATIVE_OVERRIDE, "
                     "BASIC_BLOCK_SHADOW, BASIC_BLOCK_NATIVE, BASIC_BLOCK_ADDRESS_PROVENANCE, "
                     "MECHANICAL_PRIMITIVE_SHADOW, MECHANICAL_PRIMITIVE_NATIVE, DISCOVER_BLOCKS\n";
        return 2;
    }
    try {
        const std::string_view mode_text(argv[3]);
        using oasis::hybrid::Mode;
        const bool discover_mode = mode_text == "DISCOVER_BLOCKS";
        const bool address_mode = mode_text == "BASIC_BLOCK_ADDRESS_PROVENANCE";
        const bool primitive_mode = mode_text == "MECHANICAL_PRIMITIVE_SHADOW" ||
                                    mode_text == "MECHANICAL_PRIMITIVE_NATIVE";
        const bool block_mode = mode_text == "BASIC_BLOCK_SHADOW" || mode_text == "BASIC_BLOCK_NATIVE" ||
                                address_mode || primitive_mode;
        const auto mode = discover_mode ? Mode::EMULATED : mode_text == "EMULATED" ? Mode::EMULATED :
            mode_text == "SHADOW_NATIVE" ? Mode::SHADOW_NATIVE :
            mode_text == "NATIVE_OVERRIDE" ? Mode::NATIVE_OVERRIDE :
            block_mode ? Mode::SHADOW_NATIVE : throw std::runtime_error("invalid mode");
        const bool plain_emulated = mode_text == "EMULATED";
        plain_emulated_mode = plain_emulated;
        const auto target_text = discover_mode || plain_emulated ? std::string_view{} : argc == 7 ?
            std::string_view(argv[6]) : block_mode ? std::string_view{} : std::string_view("0x3820");
        std::vector<unsigned> selected_targets;
        std::size_t begin = 0;
        while (!target_text.empty() && begin < target_text.size()) {
            const auto comma = target_text.find(',', begin);
            const auto token = target_text.substr(begin, comma == std::string_view::npos ?
                                                       target_text.size() - begin : comma - begin);
            if (token.empty()) throw std::runtime_error("invalid target list");
            selected_targets.push_back(parse_target(token));
            if (comma == std::string_view::npos) break;
            begin = comma + 1;
        }
        const auto selected_target = selected_targets.empty() ? 0U : selected_targets.front();
        if (discover_mode) {
            discovery_mode = true;
            discovered_pcs.clear();
        } else if (!plain_emulated && !block_mode && selected_targets.empty())
            throw std::runtime_error("empty target list");
        if (!block_mode && !discover_mode) {
            if (selected_targets.size() == 1 && selected_target == 0x3820) oasis::hybrid::require_mode(mode);
            for (const auto target : selected_targets)
                if (target == 0x3820) throw std::runtime_error("0x3820 cannot be combined with registry targets");
        }
#ifdef _WIN32
        _putenv_s("GPGX_HYBRID_ONLY", "1");
#else
        setenv("GPGX_HYBRID_ONLY", "1", 1);
#endif
        const auto frames = std::stoul(argv[4]);
        if (!frames || frames > 600) throw std::runtime_error("scenario frame budget must be 1..600");
        const auto rom = oasis::Rom::load(argv[2]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("canonical USA ROM required");
        directory = std::filesystem::absolute(argv[5]).string();
        std::filesystem::create_directories(directory);
        const auto library_hash = oasis::calculate_sha256(oasis::Rom::load(argv[1]).bytes());
        Library library(argv[1]);
        if (library.get<unsigned(*)()>("retro_hybrid_abi")() != 3)
            throw std::runtime_error("GPGX hybrid bridge ABI mismatch");
        std::ofstream calls(std::filesystem::path(directory) / "calls.jsonl");
        calls.exceptions(std::ios::failbit | std::ios::badbit);
        const auto reg = library.get<unsigned(*)(unsigned)>("retro_hybrid_register");
        const auto peek = library.get<int(*)(unsigned)>("retro_hybrid_peek");
        cpu_cycles = library.get<int(*)()>("retro_hybrid_cycles");
        std::unique_ptr<oasis::hybrid::Dispatch> session;
        std::vector<std::unique_ptr<oasis::hybrid::Replacement>> candidate_storage;
        std::vector<oasis::hybrid::Replacement*> candidate_targets;
        std::unique_ptr<oasis::hybrid::Registry> registry;
        std::unique_ptr<oasis::hybrid::BasicBlockRegistry> blocks;
        std::unique_ptr<oasis::hybrid::MechanicalPrimitiveRegistry> primitives;
        observed_pcs.clear();
        interpreter_instruction_executions = 0;
        if (discover_mode) {
            // Discovery deliberately installs only the execution observer. It does not
            // register a candidate, so every natural instruction remains authoritative.
        } else if (block_mode) {
            const auto set_reg = library.get<void(*)(unsigned, unsigned)>("retro_hybrid_set_register");
            const auto fetch16 = library.get<unsigned(*)()>("retro_hybrid_fetch16");
            const auto read = library.get<unsigned(*)(unsigned, int)>("retro_hybrid_read");
            const auto write = library.get<void(*)(unsigned, int, unsigned)>("retro_hybrid_write");
            const auto begin_instruction = library.get<void(*)(unsigned)>("retro_hybrid_begin_instruction");
            const auto finish_instruction = library.get<void(*)(unsigned)>("retro_hybrid_finish_instruction");
            const auto instruction_cycles = library.get<unsigned(*)(unsigned)>("retro_hybrid_instruction_cycles");
            const auto cpu_field = library.get<unsigned(*)(unsigned)>("retro_hybrid_cpu_field");
            const auto refresh_period = library.get<unsigned(*)()>("retro_hybrid_refresh_period");
            const auto refresh_penalty = library.get<unsigned(*)()>("retro_hybrid_refresh_penalty");
            gpgx_boundary_reason = library.get<unsigned(*)()>("retro_hybrid_boundary_reason");
            const auto add_block_cycles = library.get<void(*)(int)>("retro_hybrid_add_cycles");
            const auto skip_block_refresh = library.get<void(*)()>("retro_hybrid_skip_bus_refresh");
            const oasis::hybrid::BasicBlockApi api{
                reg, set_reg, peek, fetch16, read, write, begin_instruction,
                finish_instruction, add_block_cycles, skip_block_refresh,
                instruction_cycles, cpu_field, refresh_period, refresh_penalty,
                boundary_reason};
            blocks = std::make_unique<oasis::hybrid::BasicBlockRegistry>(
                api, (mode_text == "BASIC_BLOCK_SHADOW" || mode_text == "MECHANICAL_PRIMITIVE_SHADOW") ?
                    oasis::hybrid::BasicBlockMode::SHADOW_NATIVE :
                    oasis::hybrid::BasicBlockMode::NATIVE_OVERRIDE, calls);
            block_registry = blocks.get();
            if (primitive_mode) {
                primitives = std::make_unique<oasis::hybrid::MechanicalPrimitiveRegistry>(
                    api, mode_text == "MECHANICAL_PRIMITIVE_SHADOW" ?
                        oasis::hybrid::BasicBlockMode::SHADOW_NATIVE :
                        oasis::hybrid::BasicBlockMode::NATIVE_OVERRIDE, calls);
                primitive_registry = primitives.get();
            }
        } else if (plain_emulated) {
            // Plain EMULATED is the authoritative before-promotion baseline.
        } else if (selected_targets.size() == 1 && selected_target == 0x3820) {
            session = std::make_unique<oasis::hybrid::Dispatch>(oasis::hybrid::Api{reg, peek}, mode,
                                                                 rom.bytes(), calls);
            dispatch = session.get();
        } else {
            const auto set_reg = library.get<void(*)(unsigned, unsigned)>("retro_hybrid_set_register");
            const auto poke = library.get<void(*)(unsigned, int, unsigned)>("retro_hybrid_poke");
            const auto set_return_state = library.get<void(*)(unsigned, unsigned, unsigned)>("retro_hybrid_set_return_state");
            const auto cycles = library.get<int(*)()>("retro_hybrid_cycles");
            const auto add_cycles = library.get<void(*)(int)>("retro_hybrid_add_cycles");
            const auto refresh_cycles = library.get<int(*)()>("retro_hybrid_refresh_cycles");
            const auto skip_bus_refresh = library.get<void(*)()>("retro_hybrid_skip_bus_refresh");
            const oasis::hybrid::CandidateApi api{reg, set_reg, peek, poke, set_return_state, cycles, add_cycles,
                                                  refresh_cycles, skip_bus_refresh};
            for (const auto target : selected_targets) {
                if (target == 0x2D66)
                    candidate_storage.push_back(std::make_unique<oasis::hybrid::Candidate2D66>(api, mode, rom.bytes(), calls));
                else if (target == 0x604BC)
                    candidate_storage.push_back(std::make_unique<oasis::hybrid::Candidate604BC>(api, mode, calls));
                else if (target == 0x61032)
                    candidate_storage.push_back(std::make_unique<oasis::hybrid::Candidate61032>(api, mode, calls));
                else throw std::runtime_error("unsupported registry target");
            }
            for (const auto& candidate : candidate_storage) candidate_targets.push_back(candidate.get());
            registry = std::make_unique<oasis::hybrid::Registry>(candidate_targets);
            registry_dispatch = registry.get();
        }
        library.get<decltype(&retro_set_environment)>("retro_set_environment")(environment);
        library.get<decltype(&retro_set_video_refresh)>("retro_set_video_refresh")(video);
        library.get<decltype(&retro_set_audio_sample)>("retro_set_audio_sample")(audio);
        library.get<decltype(&retro_set_audio_sample_batch)>("retro_set_audio_sample_batch")(audio_batch);
        library.get<decltype(&retro_set_input_poll)>("retro_set_input_poll")(poll);
        library.get<decltype(&retro_set_input_state)>("retro_set_input_state")(input);
        library.get<decltype(&retro_init)>("retro_init")();
        const retro_game_info game{argv[2], rom.bytes().data(), rom.size(), nullptr};
        if (!library.get<decltype(&retro_load_game)>("retro_load_game")(&game))
            throw std::runtime_error("GPGX rejected ROM");
        if (address_mode) address_observer = std::make_unique<oasis::hybrid::AddressProvenanceObserver>(std::filesystem::path(directory) / "address_provenance.json", rom.size(), reg);
        library.get<void(*)(decltype(&hook))>("retro_hybrid_install")(hook);
        if (block_mode)
            library.get<void(*)(int(*)(unsigned))>("retro_hybrid_install_block")(block_hook);
        const auto run = library.get<decltype(&retro_run)>("retro_run");
        const auto size = library.get<decltype(&retro_serialize_size)>("retro_serialize_size");
        const auto serialize = library.get<decltype(&retro_serialize)>("retro_serialize");
        std::string state_hashes;
        std::ofstream checkpoints(std::filesystem::path(directory) / "checkpoints.jsonl");
        checkpoints.exceptions(std::ios::failbit | std::ios::badbit);
        std::unique_ptr<oasis::hybrid::CheckpointEvidence> evidence;
        if (checkpoint_evidence_enabled()) {
            evidence = std::make_unique<oasis::hybrid::CheckpointEvidence>(directory);
        }
        for (unsigned frame = 0; frame < frames; ++frame) {
            current_frame = frame + 1;
            if (session) session->frame(frame);
            run();
            if ((session && !session->error().empty()) || (registry && !registry->error().empty()) ||
                (blocks && !blocks->error().empty())) break;
            if ((frame + 1) % 60 == 0 || frame + 1 == frames) {
                std::vector<std::uint8_t> state(size());
                if (!serialize(state.data(), state.size())) throw std::runtime_error("serialization failed");
                const auto raw_hash = oasis::calculate_sha256(state);
                const auto hash = oasis::hybrid::checkpoint_identity_hash(state);
                state_hashes += hash;
                if (evidence) evidence->record(frame + 1, state, cpu_cycles(), raw_hash, hash);
                checkpoints << "{\"frame\":" << frame + 1 << ",\"state_sha256\":\"" << hash
                            << "\",\"raw_state_sha256\":\"" << raw_hash
                            << "\",\"cpu_cycles\":" << cpu_cycles() << "}\n";
            }
        }
        if (discover_mode) {
            discovery_mode = false;
            library.get<void(*)(decltype(&hook))>("retro_hybrid_install")(nullptr);
            library.get<decltype(&retro_unload_game)>("retro_unload_game")();
            library.get<decltype(&retro_deinit)>("retro_deinit")();
            std::ofstream discovery(std::filesystem::path(directory) / "discovered_pcs.json");
            discovery.exceptions(std::ios::failbit | std::ios::badbit);
            discovery << "{\"schema\":\"oasis.hybrid.discovery.v1\",\"frames\":" << frames
                      << ",\"unique_pcs\":" << discovered_pcs.size() << ",\"pcs\":[";
            bool first = true;
            for (const auto& [pc, count] : discovered_pcs) {
                if (!first) discovery << ',';
                first = false;
                discovery << "{\"pc\":\"0x" << std::hex << pc << std::dec
                          << "\",\"count\":" << count << '}';
            }
            discovery << "]}\n";
            std::cout << "mode=DISCOVER_BLOCKS frames=" << frames
                      << " unique_pcs=" << discovered_pcs.size() << '\n';
            return discovered_pcs.empty() ? 1 : 0;
        }
        if (block_mode)
            library.get<void(*)(int(*)(unsigned))>("retro_hybrid_install_block")(nullptr);
        library.get<void(*)(decltype(&hook))>("retro_hybrid_install")(nullptr);
        if (address_observer) { address_observer->finish(); address_observer.reset(); }
        library.get<decltype(&retro_unload_game)>("retro_unload_game")();
        library.get<decltype(&retro_deinit)>("retro_deinit")();
        unsigned natural_calls{}, comparisons{}, divergences{}, body_instructions{}, interrupts{}, override_calls{};
        unsigned translated_blocks{}, translated_entries{}, translated_instructions{}, original_inside{}, fallback_entries{}, hardware_accesses{};
        unsigned boundary_yields{}, event_boundary_yields{}, interrupt_boundary_yields{}, trace_boundary_yields{};
        unsigned translated_multi_instruction_entries{}, interrupted_resumptions{};
        unsigned interpreter_instructions{}, total_guest_instructions{}, observed_interpreter_pcs{};
        unsigned primitive_guest_instructions{};
        oasis::hybrid::ReplacementMetrics routine_metrics{};
        std::size_t registered_block_count{};
        bool complete{};
        std::string error_text;
        if (blocks) {
            const auto metrics = blocks->metrics();
            natural_calls = metrics.natural_entries;
            comparisons = metrics.shadow_comparisons;
            divergences = metrics.divergences;
            translated_blocks = metrics.translated_blocks;
            translated_entries = metrics.translated_entries;
            translated_instructions = metrics.translated_instructions;
            override_calls = metrics.translated_entries;
            original_inside = metrics.original_starts_inside_translated;
            fallback_entries = metrics.fallback_entries;
            interrupts = metrics.interrupts;
            hardware_accesses = metrics.hardware_accesses;
            boundary_yields = metrics.boundary_yields;
            event_boundary_yields = metrics.event_boundary_yields;
            interrupt_boundary_yields = metrics.interrupt_boundary_yields;
            trace_boundary_yields = metrics.trace_boundary_yields;
            translated_multi_instruction_entries = metrics.translated_multi_instruction_entries;
            interrupted_resumptions = metrics.interrupted_resumptions;
            complete = blocks->complete();
            error_text = blocks->error();
            registered_block_count = metrics.per_block.size();
            if (primitives) {
                const auto primitive_metrics = primitives->metrics();
                primitive_guest_instructions = primitive_metrics.iterations * 2U;
                natural_calls += primitive_metrics.dispatches;
                comparisons += primitive_metrics.shadow_comparisons;
                divergences += primitive_metrics.divergences;
                override_calls += primitive_metrics.dispatches;
                complete = complete && primitives->complete();
                if (error_text.empty()) error_text = primitives->error();
                if (mode_text == "MECHANICAL_PRIMITIVE_NATIVE") {
                    boundary_yields += primitive_metrics.boundary_yields;
                    event_boundary_yields += primitive_metrics.boundary_yields;
                }
            }
        } else {
            const auto totals = registry ? registry->totals() : session ? oasis::hybrid::ReplacementMetrics{
                session->calls, session->comparisons, session->divergences, session->body_instructions, 0,
                session->interrupts} : oasis::hybrid::ReplacementMetrics{};
            routine_metrics = totals;
            natural_calls = totals.calls;
            comparisons = totals.comparisons;
            divergences = totals.divergences;
            body_instructions = totals.body_instructions;
            interrupts = totals.interrupts;
            override_calls = totals.override_calls;
            complete = registry ? registry->complete() : session ? session->complete() : true;
            error_text = registry ? registry->error() : session ? session->error() : std::string{};
        }
        if (plain_emulated) {
            natural_calls = interpreter_instruction_executions;
            complete = true;
        }
        interpreter_instructions = interpreter_instruction_executions;
        total_guest_instructions = oasis::hybrid::guest_instruction_total(interpreter_instructions, translated_instructions, primitive_guest_instructions, routine_metrics.native_routine_instructions);
        observed_interpreter_pcs = static_cast<unsigned>(observed_pcs.size());
        oasis::hybrid::write_interpreter_profile_report(std::filesystem::path(directory) / "interpreter_profile.json", mode_text, observed_pcs, interpreter_instructions);
        const auto body_skipped = oasis::hybrid::body_was_skipped(block_mode, override_calls, original_inside, registry.get(), body_instructions);
        const bool completed = complete && video_frames == frames && natural_calls > 0;
        const bool full_cpu_equivalent = oasis::hybrid::full_cpu_identity(completed, divergences, block_mode, mode);
        std::ofstream report(std::filesystem::path(directory) / "summary.json");
        report.exceptions(std::ios::failbit | std::ios::badbit);
        report << "{\n\"schema\":\"oasis.hybrid-poc.v2\",\n\"target\":" << selected_target
               << ",\n\"target_hex\":\"0x" << std::hex << selected_target << std::dec
               << "\",\n\"registered_target_count\":" << (block_mode ? registered_block_count : selected_targets.size())
               << ",\n\"registered_targets\":\"" << target_text
               << "\",\n\"rom_size\":" << rom.size()
               << ",\n\"rom_sha256\":\"" << identity.fingerprint.sha256
               << "\",\n\"gpgx_binary_sha256\":\"" << library_hash
               << "\",\n\"scenario\":\"cold-reset-neutral-input\",\n\"mode\":\"" << mode_text
               << "\",\n\"requested_frames\":" << frames << ",\n\"video_frames\":" << video_frames
               << ",\n\"natural_calls\":" << natural_calls << ",\n\"shadow_comparisons\":" << comparisons
               << ",\n\"divergence_count\":" << divergences
               << ",\n\"body_instructions_observed\":" << body_instructions
               << ",\n\"original_target_body_instruction_starts\":" << body_instructions
               << ",\n\"external_interrupts_during_calls\":" << interrupts
               << ",\n\"native_override_calls\":" << override_calls << ",\n\"original_body_skipped\":"
               << (body_skipped ? "true" : "false") << ",\n\"scenario_completed\":"
               << (completed ? "true" : "false") << ",\n\"state_checkpoints_sha256\":\"" << hash_text(state_hashes)
               << "\",\n\"video_sequence_sha256\":\"" << hash_text(video_hashes)
               << "\",\n\"fallback_emulated_calls\":" << (natural_calls - override_calls)
               << ",\n\"native_call_share\":" << (natural_calls ?
                   static_cast<double>(override_calls) / natural_calls : 0.0)
               << ",\n\"translated_blocks\":" << translated_blocks
               << ",\n\"translated_entries\":" << translated_entries
               << ",\n\"translated_guest_instruction_executions\":" << translated_instructions
               << ",\n\"native_mechanical_guest_instruction_executions\":" << primitive_guest_instructions
               << ",\n\"native_mechanical_replacement_share\":" << (total_guest_instructions ?
                   static_cast<double>(primitive_guest_instructions) / total_guest_instructions : 0.0)
               << ",\n\"interpreter_instruction_executions\":" << interpreter_instructions
               << ",\n\"total_guest_instruction_executions\":" << total_guest_instructions
               << ",\n\"translated_instruction_share\":" << (total_guest_instructions ?
                   static_cast<double>(translated_instructions) / total_guest_instructions : 0.0)
               << ",\n\"unique_observed_interpreter_pcs\":" << observed_interpreter_pcs
               << ",\n\"original_starts_inside_translated\":" << original_inside
               << ",\n\"interpreter_fallback_entries\":" << fallback_entries
               << ",\n\"hardware_visible_accesses\":" << hardware_accesses
               << ",\n\"instruction_boundary_yields\":" << boundary_yields
               << ",\n\"event_boundary_yields\":" << event_boundary_yields
               << ",\n\"interrupt_boundary_yields\":" << interrupt_boundary_yields
               << ",\n\"trace_boundary_yields\":" << trace_boundary_yields
               << ",\n\"translated_multi_instruction_entries\":" << translated_multi_instruction_entries
               << ",\n\"interrupted_resumptions\":" << interrupted_resumptions;
        oasis::hybrid::write_runner_report_details(report, registry.get(), blocks.get(), primitives.get());
        oasis::hybrid::write_native_routine_accounting(report, routine_metrics);
        report
               << ",\n\"full_cpu_equivalence\":" << (full_cpu_equivalent ? "true" : "false")
               << ",\n\"sr_comparison_mask\":65519,\n\"override_blocker\":\""
               << (selected_target == 0x3820 ? oasis::hybrid::override_blocker : "") << "\"\n}\n";
        calls.close();
        checkpoints.close();
        if (evidence) evidence->close();
        report.close();
        if (!completed) {
            std::ofstream failure(std::filesystem::path(directory) / "FIRST_DIVERGENCE.txt");
            failure << (error_text.empty() ? "incomplete scenario or no natural calls" : error_text);
            throw std::runtime_error(error_text.empty() ? "scenario incomplete" : error_text);
        }
        std::cout << "mode=" << mode_text << " target=" << target_text << " natural_calls=" << natural_calls
                  << " comparisons=" << comparisons << " divergences=" << divergences
                  << " override_calls=" << override_calls << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
