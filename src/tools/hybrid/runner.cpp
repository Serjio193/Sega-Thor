#include "tools/hybrid/dispatch.hpp"
#include "tools/hybrid/candidate_2d66.hpp"
#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include <libretro.h>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iostream>
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
oasis::hybrid::Candidate2D66* candidate_dispatch{};
unsigned bytes_per_pixel = 2;
unsigned video_frames{};
std::string video_hashes;
std::string directory;
void hook(int type, int width, unsigned address, unsigned value) {
    if (candidate_dispatch) candidate_dispatch->hook(type, width, address, value);
    else dispatch->hook(type, width, address, value);
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
}

int main(int argc, char** argv) {
    if (argc != 6 && argc != 7) {
        std::cerr << "usage: oasis_hybrid_poc <GPGX library> <canonical ROM> "
                     "<EMULATED|SHADOW_NATIVE|NATIVE_OVERRIDE> <frames:1..600> <output directory> [target]\n";
        return 2;
    }
    try {
        const std::string_view mode_text(argv[3]);
        using oasis::hybrid::Mode;
        const auto mode = mode_text == "EMULATED" ? Mode::EMULATED :
            mode_text == "SHADOW_NATIVE" ? Mode::SHADOW_NATIVE :
            mode_text == "NATIVE_OVERRIDE" ? Mode::NATIVE_OVERRIDE : throw std::runtime_error("invalid mode");
        const auto target_text = argc == 7 ? std::string_view(argv[6]) : std::string_view("0x3820");
        const auto selected_target = target_text == "0x2D66" ? 0x2D66U :
            target_text == "0x3820" ? 0x3820U : throw std::runtime_error("invalid target");
        if (selected_target == 0x3820) oasis::hybrid::require_mode(mode);
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
        if (library.get<unsigned(*)()>("retro_hybrid_abi")() != 1)
            throw std::runtime_error("GPGX hybrid bridge ABI mismatch");
        std::ofstream calls(std::filesystem::path(directory) / "calls.jsonl");
        calls.exceptions(std::ios::failbit | std::ios::badbit);
        const auto reg = library.get<unsigned(*)(unsigned)>("retro_hybrid_register");
        const auto peek = library.get<int(*)(unsigned)>("retro_hybrid_peek");
        std::unique_ptr<oasis::hybrid::Dispatch> session;
        std::unique_ptr<oasis::hybrid::Candidate2D66> candidate;
        if (selected_target == 0x3820) {
            session = std::make_unique<oasis::hybrid::Dispatch>(oasis::hybrid::Api{reg, peek}, mode,
                                                                 rom.bytes(), calls);
            dispatch = session.get();
        } else {
            const auto set_reg = library.get<void(*)(unsigned, unsigned)>("retro_hybrid_set_register");
            const auto poke = library.get<void(*)(unsigned, int, unsigned)>("retro_hybrid_poke");
            candidate = std::make_unique<oasis::hybrid::Candidate2D66>(
                oasis::hybrid::CandidateApi{reg, set_reg, peek, poke}, mode, rom.bytes(), calls);
            candidate_dispatch = candidate.get();
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
        library.get<void(*)(decltype(&hook))>("retro_hybrid_install")(hook);
        const auto run = library.get<decltype(&retro_run)>("retro_run");
        const auto size = library.get<decltype(&retro_serialize_size)>("retro_serialize_size");
        const auto serialize = library.get<decltype(&retro_serialize)>("retro_serialize");
        std::string state_hashes;
        std::ofstream checkpoints(std::filesystem::path(directory) / "checkpoints.jsonl");
        checkpoints.exceptions(std::ios::failbit | std::ios::badbit);
        for (unsigned frame = 0; frame < frames; ++frame) {
            if (session) session->frame(frame);
            run();
            if ((session && !session->error().empty()) || (candidate && !candidate->error().empty())) break;
            if ((frame + 1) % 60 == 0 || frame + 1 == frames) {
                std::vector<std::uint8_t> state(size());
                if (!serialize(state.data(), state.size())) throw std::runtime_error("serialization failed");
                const auto hash = oasis::calculate_sha256(state);
                state_hashes += hash;
                checkpoints << "{\"frame\":" << frame + 1 << ",\"state_sha256\":\"" << hash << "\"}\n";
            }
        }
        library.get<void(*)(decltype(&hook))>("retro_hybrid_install")(nullptr);
        library.get<decltype(&retro_unload_game)>("retro_unload_game")();
        library.get<decltype(&retro_deinit)>("retro_deinit")();
        const auto natural_calls = candidate ? candidate->calls : session->calls;
        const auto comparisons = candidate ? candidate->comparisons : session->comparisons;
        const auto divergences = candidate ? candidate->divergences : session->divergences;
        const auto body_instructions = candidate ? candidate->body_instructions : session->body_instructions;
        const auto interrupts = candidate ? candidate->interrupt_count : session->interrupts;
        const auto override_calls = candidate ? candidate->override_calls : 0U;
        const auto body_skipped = candidate ? (override_calls > 0 && body_instructions == 0) : false;
        const auto complete = candidate ? candidate->complete() : session->complete();
        const auto& error_text = candidate ? candidate->error() : session->error();
        const bool completed = complete && video_frames == frames && natural_calls > 0;
        std::ofstream report(std::filesystem::path(directory) / "summary.json");
        report.exceptions(std::ios::failbit | std::ios::badbit);
        report << "{\n\"schema\":\"oasis.hybrid-poc.v1\",\n\"target\":" << selected_target
               << ",\n\"target_hex\":\"0x" << std::hex << selected_target << std::dec
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
               << "\",\n\"full_cpu_equivalence\":" << ((completed && divergences == 0) ? "true" : "false")
               << ",\n\"sr_comparison_mask\":65519,\n\"override_blocker\":\""
               << (selected_target == 0x3820 ? oasis::hybrid::override_blocker : "") << "\"\n}\n";
        calls.close();
        checkpoints.close();
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
