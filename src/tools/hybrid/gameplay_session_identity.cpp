#include "tools/hybrid/gameplay_session_identity.hpp"
#include "core/rom_identity.hpp"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iterator>
#include <unistd.h>
#endif

namespace oasis::hybrid::session {

std::string file_sha256(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read identity file: " + path.string());
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    return oasis::calculate_sha256(bytes);
}

std::string executable_path() {
#ifdef _WIN32
    std::array<char, 32768> path{};
    const auto length = GetModuleFileNameA(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length >= path.size()) throw std::runtime_error("cannot resolve runner executable path");
    return std::filesystem::absolute(std::string(path.data(), length)).string();
#else
    std::array<char, 4096> path{};
    const auto length = readlink("/proc/self/exe", path.data(), path.size() - 1);
    if (length <= 0) throw std::runtime_error("cannot resolve runner executable path");
    return std::filesystem::absolute(std::string(path.data(), static_cast<std::size_t>(length))).string();
#endif
}

Manifest read_manifest(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("capture manifest missing");
    std::string line;
    if (!std::getline(in, line) || line != "OASIS_GAMEPLAY_SESSION 1")
        throw std::runtime_error("unsupported session manifest");
    Manifest values;
    while (std::getline(in, line)) {
        const auto split = line.find('=');
        if (split == std::string::npos || !values.emplace(line.substr(0, split), line.substr(split + 1)).second)
            throw std::runtime_error("malformed session manifest");
    }
    return values;
}

void write_manifest(const std::filesystem::path& path, const Manifest& values) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw std::runtime_error("cannot write session manifest");
    out << "OASIS_GAMEPLAY_SESSION 1\n";
    for (const auto& [key, value] : values) out << key << '=' << value << '\n';
    if (!out) throw std::runtime_error("manifest write failed");
}

std::string config_identity(unsigned frames, const std::string& input_mode) {
    std::ostringstream out;
    out << "cold_retro_load_game;frames=" << frames << ";input=" << input_mode
        << ";poll=retro_input_poll+ordered_retro_input_state;landmark=60;"
        << "watch=E376,9C40,E37A,E396,94A2,8EF8,B730,16A46;"
        << "call=E376:9C40:E37A:E396;rts=9C7A;video=discard;audio=discard;sram=isolated-per-run";
    return out.str();
}

} // namespace oasis::hybrid::session
