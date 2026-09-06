#include "tools/re_assemble.hpp"

#include <array>
#include <sstream>

namespace oasis::tools {
namespace {
void operand_json(std::ostream& out, const std::optional<DecodedOperand>& operand) {
    if (!operand) { out << "null"; return; }
    constexpr std::array names{"data_register", "address_register", "indirect", "postincrement",
        "predecrement", "displacement", "indexed", "absolute_word", "absolute_long",
        "pc_displacement", "pc_indexed", "immediate", "register_list"};
    const auto& p = *operand;
    out << "{\"kind\":\"" << names[static_cast<unsigned>(p.kind)]
        << "\",\"register\":" << unsigned(p.register_index)
        << ",\"width_bytes\":" << unsigned(p.width_bytes)
        << ",\"extension_bytes\":" << unsigned(p.extension_bytes)
        << ",\"value\":" << p.value << ",\"displacement\":" << p.displacement
        << ",\"extension_address\":" << p.extension_address << '}';
}
} // namespace
std::string exact_slice_json(const DecodedSlice& slice) {
    std::ostringstream out;
    out << "{\"source_decoder\":\"re_slice_decoder\",\"start\":" << slice.entry
        << ",\"end\":" << slice.range_end << ",\"instructions\":[";
    bool first = true;
    for (const auto& instruction : slice.instructions) {
        if (!first) out << ',';
        first = false;
        const auto& exact = instruction.exact.value();
        out << "{\"address\":" << instruction.address << ",\"opcode\":" << instruction.opcode
            << ",\"raw_words\":[";
        for (std::size_t i = 0; i < instruction.bytes.size(); i += 2) {
            if (i) out << ',';
            out << ((unsigned(instruction.bytes[i]) << 8U) | instruction.bytes.at(i + 1));
        }
        out << "],\"operation\":\"" << exact.operation << "\",\"width_bytes\":"
            << unsigned(exact.width_bytes) << ",\"branch_width_bytes\":"
            << unsigned(exact.branch_width_bytes) << ",\"branch_target\":";
        if (instruction.direct_target) out << *instruction.direct_target;
        else out << "null";
        out << ",\"source\":";
        operand_json(out, exact.source);
        out << ",\"destination\":";
        operand_json(out, exact.destination);
        out << '}';
    }
    return out.str() + "]}\n";
}
} // namespace oasis::tools
