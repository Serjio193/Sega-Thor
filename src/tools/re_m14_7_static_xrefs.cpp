#include "core/rom_identity.hpp"
#include "tools/re_canonical_rom_bytes.hpp"
#include "tools/re_static_xref_scan.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::uint32_t number(const std::string& text) {
    std::size_t used = 0;
    const auto value = std::stoull(text, &used, 0);
    if (used != text.size() || value > 0xFFFFFFFFULL)
        throw std::invalid_argument("invalid ROM offset");
    return static_cast<std::uint32_t>(value);
}

std::vector<oasis::tools::StaticXrefSpan> read_spans(const char* path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read span input");
    std::vector<oasis::tools::StaticXrefSpan> spans;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        std::string start, end;
        if (!std::getline(fields, start, '\t') || !std::getline(fields, end, '\t'))
            throw std::runtime_error("invalid span row");
        spans.push_back({number(start), number(end)});
    }
    return spans;
}

void write_candidates(const char* path,
    const std::vector<oasis::tools::StaticXrefCandidate>& candidates) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write xref candidates");
    out << "kind\tcaller_pc\tcaller_end\topcode\tmnemonic\tflow\tcondition"
           "\ttarget_pc\tsource_start\tsource_end\tcomponent_start\tcomponent_end"
           "\ttarget_is_entry\n";
    for (const auto& item : candidates) {
        out << oasis::tools::static_xref_kind(item) << '\t' << item.caller_pc << '\t'
            << item.caller_end << '\t' << item.opcode << '\t' << item.mnemonic << '\t'
            << oasis::tools::flow_kind_name(item.flow) << '\t'
            << static_cast<unsigned>(item.condition_code) << '\t' << item.target_pc << '\t'
            << item.source_span.start << '\t' << item.source_span.end << '\t'
            << item.target_component.start << '\t' << item.target_component.end << '\t'
            << (item.target_is_component_entry ? "true" : "false") << '\n';
    }
    if (!out) throw std::runtime_error("cannot finish xref candidates");
}

void write_summary(const char* path, const oasis::tools::StaticXrefScanResult& scan,
                   const std::string& rom_sha, std::size_t rom_size) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write xref summary");
    out << "{\"schema\":\"oasis.m14.7.static-xref-scan.v1\",\"rom_sha256\":\""
        << rom_sha << "\",\"rom_size\":" << rom_size
        << ",\"decoded_instruction_count\":" << scan.decoded_instruction_count
        << ",\"decoded_byte_count\":" << scan.decoded_byte_count
        << ",\"candidate_count\":" << scan.candidates.size() << ",\"ranges\":[";
    for (std::size_t i = 0; i < scan.ranges.size(); ++i) {
        if (i) out << ',';
        const auto& range = scan.ranges[i];
        out << "{\"start\":" << range.span.start << ",\"end\":" << range.span.end
            << ",\"decoded_end\":" << range.decoded_end
            << ",\"instruction_count\":" << range.instruction_count
            << ",\"stop_reason\":\"" << range.stop_reason << "\"}";
    }
    out << "]}\n";
    if (!out) throw std::runtime_error("cannot finish xref summary");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: oasis_re_m14_7_static_xrefs <rom> <verified-asm.tsv> "
                     "<components.tsv> <candidates.tsv> <summary.json>\n";
        return 2;
    }
    try {
        const auto rom = oasis::tools::CanonicalRomBytes::load(argv[1]);
        const auto sources = read_spans(argv[2]);
        const auto targets = read_spans(argv[3]);
        const auto scan = oasis::tools::scan_static_xrefs(
            rom.read(0, rom.size()), sources, targets);
        write_candidates(argv[4], scan.candidates);
        write_summary(argv[5], scan, rom.sha256(), rom.size());
        std::cout << "DECODED_INSTRUCTIONS=" << scan.decoded_instruction_count
                  << " DECODED_BYTES=" << scan.decoded_byte_count
                  << " CANDIDATES=" << scan.candidates.size() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
