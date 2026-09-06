#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_assemble.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace {
using namespace oasis::tools;
struct Selection {
    std::uint32_t start, end;
    const char* structural;
    const char* runtime;
    const char* semantics;
};
constexpr std::array selections{
    Selection{0x07C4, 0x07E2, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x08A2, 0x08B6, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x0D5E, 0x0D82, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x0E80, 0x0EC2, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x0F32, 0x0F7E, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x1108, 0x1112, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x12E8, 0x1300, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x2B6E, 0x2B8A, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x2B8A, 0x2BA0, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x2D66, 0x2D84, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x3820, 0x3B3E, "VERIFIED_BOUNDED_CODE", "M3 vectors; M11.8 13 hits", "graphics decompression"},
    Selection{0x4A92, 0x4AD0, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x62CC, 0x62E4, "STRONG_STATIC", "M11.6.1/M11.8 not reached", "UNKNOWN"},
    Selection{0x64C4, 0x6516, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x8504, 0x8530, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x85C4, 0x85E2, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x8CAC, 0x8CD0, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x94A2, 0x94D2, "STRONG_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x99B8, 0x99D6, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0x9BF2, 0x9C40, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0xA8DA, 0xA8F0, "MODERATE_STATIC; multiple_entry_overlap", "M11.6.1 not reached", "UNKNOWN"},
    Selection{0xB730, 0xB79A, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0xC90E, 0xC92C, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0xCECC, 0xCEEA, "MODERATE_STATIC", "UNKNOWN", "UNKNOWN"},
    Selection{0xD3B2, 0xD406, "STRONG_STATIC", "M11 prior bounded evidence", "UNKNOWN"}};
std::string label(std::uint32_t address) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << address;
    return out.str();
}
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!(out << text)) throw std::runtime_error("cannot write output");
}
std::uint32_t number(const char* text) {
    std::size_t used = 0;
    const auto value = std::stoull(text, &used, 0);
    if (text[used] || value > 0xFFFFFFFFULL) throw std::invalid_argument("invalid offset");
    return static_cast<std::uint32_t>(value);
}
void emit(const oasis::Rom& rom, const oasis::RomIdentity& identity,
          const std::filesystem::path& directory) {
    if (std::filesystem::exists(directory))
        throw std::invalid_argument("output directory must be new to avoid stale artifacts");
    // Validate every slice before creating any output.
    std::vector<DecodedSlice> slices;
    for (const auto& selected : selections) {
        auto slice = decode_m68k_slice(rom.bytes(), {selected.start, selected.end - selected.start, 512});
        try {
            (void)slice_asm(slice);
        } catch (const std::exception& error) {
            throw std::runtime_error("selection 0x" + label(selected.start) + ": " + error.what());
        }
        slices.push_back(std::move(slice));
    }
    std::filesystem::create_directories(directory / "code");
    std::filesystem::create_directories(directory / "blobs");
    std::ostringstream manifest, main;
    manifest << "{\"schema\":\"oasis.reassemblable-poc.v1\",\"rom_sha256\":\""
             << identity.fingerprint.sha256 << "\",\"start\":" << selections.front().start
             << ",\"end\":" << selections.back().end << ",\"routines\":[";
    main << "; Local bounded reconstructed layout; unknown gaps are exact local ROM blobs.\n"
         << "    org $" << label(selections.front().start) << "\n";
    for (std::size_t i = 0; i < selections.size(); ++i) {
        const auto& selected = selections[i];
        const auto stem = "sub_" + label(selected.start);
        write(directory / "code" / (stem + ".asm"), slice_asm(slices[i]));
        write(directory / "code" / (stem + ".json"), exact_slice_json(slices[i]));
        if (i) manifest << ',';
        manifest << "{\"start\":" << selected.start << ",\"end\":" << selected.end
            << ",\"instruction_count\":" << slices[i].instructions.size()
            << ",\"source_decoder\":\"re_slice_decoder\",\"structural_status\":\""
            << selected.structural << "\",\"confidence\":\"CONFIRMED_ROM_ENCODING; semantics separately qualified\""
            << ",\"runtime_evidence\":\"" << selected.runtime << "\",\"semantic_meaning\":\""
            << selected.semantics << "\",\"asm\":\"code/" << stem << ".asm\"}";
        std::istringstream body(slice_asm(slices[i]));
        std::string line;
        while (std::getline(body, line)) {
            if (line.rfind("    org ", 0) == 0 || line.rfind("sub_", 0) == 0) continue;
            main << line << '\n';
        }
        if (i + 1 < selections.size())
            main << "data_" << label(selected.end) << ":\n    incbin \"blobs/"
                 << label(selected.end) << ".bin\"\n";
    }
    manifest << "],\"unknown_ranges\":[";
    for (std::size_t i = 0; i + 1 < selections.size(); ++i) {
        if (i) manifest << ',';
        manifest << "{\"start\":" << selections[i].end << ",\"end\":" << selections[i + 1].start
                 << ",\"kind\":\"UNKNOWN_BLOB\",\"file\":\"blobs/"
                 << label(selections[i].end) << ".bin\"}";
    }
    write(directory / "main.asm", main.str());
    write(directory / "manifest.json", manifest.str() + "]}\n");
    std::cout << "EMITTED " << selections.size()
              << " routines; extract unknown ranges only from local canonical ROM\n";
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) throw std::invalid_argument("missing command");
        const std::string command = argv[1];
        if ((command == "emit" && argc != 4) || (command == "verify" && argc != 6) ||
            (command != "emit" && command != "verify"))
            throw std::invalid_argument("usage: oasis_re_assemble emit <rom> <new-directory> | verify <rom> <binary> <start> <end-exclusive>");
        const auto rom = oasis::Rom::load(argv[2]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported)
            throw std::invalid_argument("canonical supported USA ROM required");
        if (command == "emit") { emit(rom, identity, argv[3]); return 0; }
        std::ifstream binary(argv[3], std::ios::binary);
        if (!binary) throw std::runtime_error("cannot read assembled slice");
        const std::vector<std::uint8_t> rebuilt{std::istreambuf_iterator<char>(binary), {}};
        if (binary.bad()) throw std::runtime_error("failed reading assembled slice");
        const auto start = number(argv[4]), end = number(argv[5]);
        const auto difference = first_byte_difference(rom.bytes(), rebuilt, start, end);
        std::optional<DecodedSlice> slice;
        for (const auto& selected : selections) {
            if (selected.start == start && selected.end == end)
                slice = decode_m68k_slice(rom.bytes(), {start, end - start, 512});
        }
        std::cout << difference_text(difference, slice ? &*slice : nullptr);
        return difference ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
