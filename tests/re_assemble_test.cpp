#include "tools/re_assemble.hpp"

#include <cassert>
#include <stdexcept>

namespace {
using namespace oasis::tools;
DecodedSlice decode(std::initializer_list<std::uint8_t> bytes) {
    const std::vector<std::uint8_t> data(bytes);
    return decode_m68k_slice(data, {0, data.size(), 64});
}
void operands_and_encoding() {
    // Synthetic register choices and values; no commercial routine fixture.
    const auto slice = decode({0x36,0xC1, 0x32,0x3C,0x12,0x34,
                              0x21,0x41,0x00,0x00, 0x4E,0x75});
    const auto& move = slice.instructions[0];
    assert(move.opcode == 0x36C1 && move.bytes.size() == 2);
    assert(move.exact->width_bytes == 2);
    assert(move.exact->source->kind == OperandKind::data_register);
    assert(move.exact->source->register_index == 1);
    assert(move.exact->destination->kind == OperandKind::postincrement);
    assert(move.exact->destination->register_index == 3);
    assert(exact_instruction_asm(move) == "move.w D1,(A3)+");
    assert(exact_instruction_asm(slice.instructions[1]) == "move.w #$1234,D1");
    assert(slice.instructions[1].exact->source->extension_bytes == 2);
    assert(exact_instruction_asm(slice.instructions[2]) == "move.l D1,0(A0)");
    assert(slice.instructions[2].exact->destination->kind == OperandKind::displacement);
    const std::string expected =
        "; Local ROM-derived code. Do not commit. vasm -m68000 -no-opt -Fbin\n"
        "    org $0\nsub_000000:\n"
        "loc_000000:\n    move.w D1,(A3)+\n"
        "loc_000002:\n    move.w #$1234,D1\n"
        "loc_000006:\n    move.l D1,0(A0)\n"
        "loc_00000A:\n    rts\n";
    assert(slice_asm(slice) == expected);
    const auto json = exact_slice_json(slice);
    assert(json == exact_slice_json(slice));
    assert(json.find("\"kind\":\"postincrement\"") != std::string::npos);
    assert(json.find("\"raw_words\":[14017]") != std::string::npos);
}
void branch_widths() {
    const auto short_branch = decode({0x66,0x02, 0x4E,0x71, 0x4E,0x75});
    assert(short_branch.instructions[0].exact->branch_width_bytes == 1);
    assert(exact_instruction_asm(short_branch.instructions[0]) == "bne.s loc_000004");
    const auto word_branch = decode({0x66,0x00,0x00,0x04, 0x4E,0x71, 0x4E,0x75});
    assert(word_branch.instructions[0].exact->branch_width_bytes == 2);
    assert(exact_instruction_asm(word_branch.instructions[0]) == "bne.w loc_000006");
    assert(slice_asm(word_branch).find("org $0") != std::string::npos);
    const auto dbf = decode({0x51,0xCA,0xFF,0xFE,0x4E,0x75});
    assert(dbf.instructions[0].exact->source->width_bytes == 2);
    assert(dbf.instructions[0].exact->branch_width_bytes == 2);
    assert(exact_instruction_asm(dbf.instructions[0]) == "dbf D2,loc_000000");
}
void movem_and_sizes() {
    const auto slice = decode({0x48,0xA7,0x80,0x00, 0x4C,0x9F,0x00,0x01,
                              0x10,0x3C,0x00,0xAB, 0x4E,0x75});
    assert(exact_instruction_asm(slice.instructions[0]) == "movem.w D0,-(A7)");
    assert(exact_instruction_asm(slice.instructions[1]) == "movem.w (A7)+,D0");
    assert(slice.instructions[0].effective_operands[0].width_bytes == 2);
    assert(slice.instructions[2].exact->width_bytes == 1);
    assert(slice.instructions[2].exact->source->extension_bytes == 2);
    assert(exact_instruction_asm(slice.instructions[2]) == "move.b #$AB,D0");
    const auto address_ops = decode({0x95,0xC3, 0xD5,0xFC,0x12,0x34,0x56,0x78, 0x4E,0x75});
    assert(exact_instruction_asm(address_ops.instructions[0]) == "suba.l D3,A2");
    assert(exact_instruction_asm(address_ops.instructions[1]) == "adda.l #$12345678,A2");
    assert(address_ops.instructions[1].bytes.size() == 6);
}
void differences() {
    const std::vector<std::uint8_t> rom{9,8,0x36,0xC1,0x4E,0x75};
    std::vector<std::uint8_t> rebuilt{0x36,0xC1,0x4E,0x75};
    assert(!first_byte_difference(rom, rebuilt, 2, 6));
    rebuilt[1] = 0xC2;
    const auto diff = first_byte_difference(rom, rebuilt, 2, 6);
    assert(diff->rom_offset == 3 && diff->slice_offset == 1);
    assert(diff->expected == 0xC1 && diff->actual == 0xC2);
    const auto slice = decode_m68k_slice(rom, {2,4,8});
    assert(difference_text(diff, &slice).find("instruction=0x000002 move.w D1,(A3)+") != std::string::npos);
    rebuilt = {0x36,0xC1};
    assert(!first_byte_difference(rom, rebuilt, 2, 6)->actual);
    rebuilt = {0x36,0xC1,0x4E,0x75,0};
    assert(!first_byte_difference(rom, rebuilt, 2, 6)->expected);
    rebuilt.clear();
    assert(first_byte_difference(rom, rebuilt, 2, 6)->slice_offset == 0);
    bool rejected = false;
    try { (void)first_byte_difference(rom, rebuilt, 5, 7); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
}
void rejects_unknown_and_gaps() {
    for (const auto& slice : {decode({0xFF,0xFF}), decode({0x4E,0x75,0x00,0x00}),
                             decode({0x66,0x10,0x4E,0x75})}) {
        bool rejected = false;
        try { (void)slice_asm(slice); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected);
    }
}
} // namespace
int main() {
    operands_and_encoding();
    branch_widths();
    movem_and_sizes();
    differences();
    rejects_unknown_and_gaps();
}
