#include "runtime_rom_properties.hpp"

#include "core/rom_identity.hpp"

#include <algorithm>
#include <chrono>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace oasis::thor::runtime_map;

namespace {

std::vector<std::uint8_t> read_bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read " + path.string());
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
    if (file.bad()) throw std::runtime_error("read failed for " + path.string());
    return bytes;
}

void atomic_text(const std::filesystem::path& path, const std::string& text) {
    auto temp = path;
    temp += ".tmp." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file) throw std::runtime_error("cannot create merge manifest temp file");
        file.write(text.data(), static_cast<std::streamsize>(text.size())); file.flush();
        if (!file) { file.close(); std::filesystem::remove(temp); throw std::runtime_error("manifest write failed"); }
    }
#ifdef _WIN32
    HANDLE handle = CreateFileW(temp.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE || !FlushFileBuffers(handle)) {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        std::filesystem::remove(temp); throw std::runtime_error("manifest flush failed");
    }
    CloseHandle(handle);
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temp); throw std::runtime_error("manifest replacement failed");
    }
#else
    const int fd = ::open(temp.c_str(), O_WRONLY);
    if (fd < 0 || ::fsync(fd) != 0) {
        if (fd >= 0) ::close(fd);
        std::filesystem::remove(temp); throw std::runtime_error("manifest flush failed");
    }
    ::close(fd);
    if (::rename(temp.c_str(), path.c_str()) != 0) {
        std::filesystem::remove(temp); throw std::runtime_error("manifest replacement failed");
    }
#endif
}

void export_ranges(const std::filesystem::path& checkpoint, const std::filesystem::path& output) {
    Identity id;
    const auto map = load_checkpoint(checkpoint, id);
    std::ofstream file(output, std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("cannot create range export");
    file << "# schema=thor.rom-property-ranges.v1\n"
         << "# rom_sha256=" << id.rom_sha256 << "\n"
         << "# rom_size=" << id.rom_size << "\n"
         << "# classifier_schema=" << id.schema_id << "\n"
         << "# contract_sha256=" << id.contract_sha256 << "\n"
         << "# core_build_id=" << id.core_build_id << "\n"
         << "# run_id=" << id.run_id << "\n"
         << "# generation=" << id.generation << "\n"
         << "# capabilities=" << id.capabilities << "\n"
         << "# validation_state=" << id.validation_state << "\n"
         << "start\tend_exclusive\tproperty_mask\n" << std::hex;
    for (const auto& range : map.ranges())
        file << range.start << '\t' << range.end << '\t' << range.properties << '\n';
    file.flush();
    if (!file) throw std::runtime_error("range export write failed");
}

std::uint16_t hex_nibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<std::uint16_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<std::uint16_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<std::uint16_t>(c - 'A' + 10);
    throw std::invalid_argument("invalid property hex digit");
}

void import_hex(const std::filesystem::path& hex_path,
                const std::filesystem::path& rom_path,
                const std::filesystem::path& core_manifest,
                const std::filesystem::path& output,
                const std::string& run_id, std::uint64_t generation,
                std::uint64_t capabilities, const std::string& state) {
    auto rom = read_bytes(rom_path);
    const auto hex = read_bytes(hex_path);
    const auto manifest = read_bytes(core_manifest);
    std::string digits;
    digits.reserve(hex.size());
    for (const auto byte : hex) {
        const char c = static_cast<char>(byte);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        digits.push_back(c);
    }
    if (digits.size() != rom.size() * 4)
        throw std::invalid_argument("property dump must contain exactly four hex digits per ROM byte");
    Identity id;
    id.rom_sha256 = oasis::calculate_sha256(rom);
    id.rom_size = rom.size();
    id.schema_id = std::string(SCHEMA_ID);
    id.contract_sha256 = std::string(CONTRACT_SHA256);
    id.core_build_id = oasis::calculate_sha256(manifest);
    id.run_id = run_id;
    id.generation = generation;
    id.capabilities = capabilities;
    id.validation_state = state;
    validate_identity(id);
    PropertyMap map(rom.size());
    for (std::size_t i = 0; i < rom.size(); ++i) {
        const auto value = static_cast<std::uint16_t>((hex_nibble(digits[i * 4]) << 12U) |
            (hex_nibble(digits[i * 4 + 1]) << 8U) |
            (hex_nibble(digits[i * 4 + 2]) << 4U) | hex_nibble(digits[i * 4 + 3]));
        if ((value & ~ALL_PROPERTIES) || (value & ~capabilities))
            throw std::invalid_argument("property dump exceeds byte or declared capabilities");
        if (value) map.apply_validated(i, 1, value);
    }
    save_checkpoint(output, id, map);
}

void merge_runs(const std::filesystem::path& output, int count, char** paths) {
    if (count < 1) throw std::invalid_argument("merge requires at least one run file");
    std::vector<Contribution> runs;
    std::vector<std::pair<std::string, std::filesystem::path>> sources;
    std::set<std::string> run_ids;
    std::uint64_t generation = 0;
    for (int i = 0; i < count; ++i) {
        auto run = load_contribution(paths[i]);
        if (!run_ids.insert(run.identity.run_id).second)
            throw std::invalid_argument("duplicate run_id in merge inputs");
        sources.emplace_back(run.identity.run_id, std::filesystem::absolute(paths[i]));
        generation = (std::max)(generation, run.identity.generation);
        runs.push_back(std::move(run));
    }
    std::sort(runs.begin(), runs.end(), [](const auto& a, const auto& b) {
        return a.identity.run_id < b.identity.run_id;
    });
    std::sort(sources.begin(), sources.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    auto target = runs.front().identity;
    if (generation == (std::numeric_limits<std::uint64_t>::max)())
        throw std::overflow_error("ROM property generation overflow");
    std::vector<std::string> ordered(run_ids.begin(), run_ids.end());
    std::ostringstream canonical;
    for (const auto& id : ordered) canonical << id << '\n';
    const auto bytes = canonical.str();
    const auto digest = oasis::calculate_sha256(std::span(
        reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()));
    target.run_id = "union-" + digest.substr(0, 24);
    target.generation = generation + 1;
    const auto merged = rebuild_union(runs, target);
    save_checkpoint(output, target, merged);

    const auto union_bytes = read_bytes(output);
    std::ostringstream manifest;
    manifest << "schema=thor.rom-property-union.v1\n"
             << "union_run_id=" << target.run_id << "\n"
             << "union_generation=" << target.generation << "\n"
             << "union_checkpoint_sha256=" << oasis::calculate_sha256(union_bytes) << "\n";
    for (const auto& [run_id, source] : sources) {
        const auto source_bytes = read_bytes(source);
        manifest << "input_run_id=" << std::quoted(run_id) << '\t'
                 << "sha256=" << oasis::calculate_sha256(source_bytes) << '\t'
                 << "path=" << std::quoted(source.string()) << '\n';
    }
    atomic_text(output.string() + ".manifest", manifest.str());
    std::cout << "merged " << runs.size() << " runs into " << output.string() << '\n';
}

void usage() {
    std::cerr << "Usage:\n"
              << "  oasis_rom_property_tool import-hex HEX ROM CORE_MANIFEST OUTPUT RUN_ID GENERATION CAPABILITIES STATE\n"
              << "  oasis_rom_property_tool export CHECKPOINT RANGES.tsv\n"
              << "  oasis_rom_property_tool merge OUTPUT CHECKPOINT...\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 4 && std::string(argv[1]) == "export") {
            export_ranges(argv[2], argv[3]);
            return 0;
        }
        if (argc == 10 && std::string(argv[1]) == "import-hex") {
            auto parse_u64 = [](const char* value) {
                std::uint64_t parsed{};
                const std::string_view input(value);
                const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), parsed, 10);
                if (error != std::errc{} || end != input.data() + input.size())
                    throw std::invalid_argument("invalid numeric import-hex argument");
                return parsed;
            };
            import_hex(argv[2], argv[3], argv[4], argv[5], argv[6],
                       parse_u64(argv[7]), parse_u64(argv[8]), argv[9]);
            return 0;
        }
        if (argc >= 4 && std::string(argv[1]) == "merge") {
            merge_runs(argv[2], argc - 3, argv + 3);
            return 0;
        }
        usage(); return 2;
    } catch (const std::exception& error) {
        std::cerr << "ROM property operation failed: " << error.what() << '\n';
        return 1;
    }
}
