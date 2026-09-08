#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
#include "tools/hybrid/recomp_generator.hpp"
#include "tools/re_assemble.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <initializer_list>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {
using oasis::hybrid::BasicBlockApi;
using oasis::hybrid::generated::add_l_data_to_data;
using oasis::hybrid::generated::add_w_postincrement_to_data_register;
using oasis::hybrid::generated::adda_w_data_to_address;
using oasis::hybrid::generated::branch_condition;
using oasis::hybrid::generated::clear_w_data_register;
using oasis::hybrid::generated::dbcc;
using oasis::hybrid::generated::lea_absolute_long;
using oasis::hybrid::generated::move_b_postincrement_to_data_register;
using oasis::hybrid::generated::move_w_postincrement_to_postincrement;
using oasis::hybrid::generated::movem_l_predecrement;
using oasis::hybrid::generated::test_absolute_long;

struct Access {
    char kind{};
    std::uint32_t address{};
    int width{};
    std::uint32_t value{};
    bool operator==(const Access&) const = default;
};

struct Machine {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<Access> accesses;
    std::vector<std::uint32_t> fetch_words;
    std::size_t fetch_index{};
    int cycles{};
    static Machine* current;

    static unsigned reg(unsigned index) { return current->regs.at(index); }
    static void set_reg(unsigned index, unsigned value) { current->regs.at(index) = value; }
    static int peek(unsigned address) {
        const auto it = current->memory.find(address);
        return it == current->memory.end() ? 0 : it->second;
    }
    static unsigned read(unsigned address, int width) {
        unsigned value = 0;
        for (int i = 0; i < width; ++i)
            value = (value << 8U) | static_cast<unsigned>(peek(address + i));
        current->accesses.push_back({'R', address, width, value});
        return value;
    }
    static void write(unsigned address, int width, unsigned value) {
        for (int i = 0; i < width; ++i)
            current->memory[address + static_cast<unsigned>(i)] =
                static_cast<std::uint8_t>(value >> (8 * (width - i - 1)));
        current->accesses.push_back({'W', address, width, value});
    }
    static unsigned fetch16() {
        const auto value = current->fetch_words.at(current->fetch_index++);
        current->regs[16] += 2U;
        return value;
    }
    static void begin(unsigned) {}
    static void finish(unsigned) {}
    static void add_cycles(int value) { current->cycles += value; }
    static void skip_refresh() {}
};

Machine* Machine::current = nullptr;

BasicBlockApi api_for(Machine& machine) {
    Machine::current = &machine;
    return {Machine::reg, Machine::set_reg, Machine::peek, Machine::fetch16,
            Machine::read, Machine::write, Machine::begin, Machine::finish,
            Machine::add_cycles, Machine::skip_refresh, nullptr, nullptr, nullptr, nullptr};
}

Machine seed() {
    Machine machine;
    for (unsigned i = 0; i < machine.regs.size(); ++i)
        machine.regs[i] = 0x10203040U + i * 0x01010101U;
    machine.regs[17] = 0xA713U;
    return machine;
}

std::uint32_t ref_read(Machine& machine, std::uint32_t address, int width) {
    std::uint32_t value = 0;
    for (int i = 0; i < width; ++i) {
        const auto it = machine.memory.find(address + static_cast<unsigned>(i));
        value = (value << 8U) | (it == machine.memory.end() ? 0U : it->second);
    }
    machine.accesses.push_back({'R', address, width, value});
    return value;
}

void ref_write(Machine& machine, std::uint32_t address, int width, std::uint32_t value) {
    for (int i = 0; i < width; ++i)
        machine.memory[address + static_cast<unsigned>(i)] =
            static_cast<std::uint8_t>(value >> (8 * (width - i - 1)));
    machine.accesses.push_back({'W', address, width, value});
}

void ref_move_flags(Machine& machine, std::uint32_t value, unsigned width) {
    const auto sign = width == 1 ? 0x80U : width == 2 ? 0x8000U : 0x80000000U;
    const auto mask = width == 1 ? 0xFFU : width == 2 ? 0xFFFFU : 0xFFFFFFFFU;
    machine.regs[17] = (machine.regs[17] & ~0x0FU) | (value & mask ? 0U : 4U) |
                       (value & sign ? 8U : 0U);
}

void ref_movem(Machine& machine, std::uint32_t mask) {
    auto stack = machine.regs[15];
    for (int bit = 15; bit >= 0; --bit) {
        if (!(mask & (1U << static_cast<unsigned>(bit)))) continue;
        const auto value = machine.regs[bit];
        stack -= 2U;
        ref_write(machine, stack, 2, value & 0xFFFFU);
        stack -= 2U;
        ref_write(machine, stack, 2, value >> 16U);
    }
    machine.regs[15] = stack;
}

void ref_clear(Machine& machine) {
    machine.regs[3] &= 0xFFFF0000U;
    ref_move_flags(machine, 0, 2);
}

void ref_move_b(Machine& machine) {
    const auto address = machine.regs[14];
    const auto value = ref_read(machine, address, 1);
    machine.regs[14] = address + 1U;
    machine.regs[7] = (machine.regs[7] & 0xFFFFFF00U) | value;
    ref_move_flags(machine, value, 1);
}

void ref_lea(Machine& machine) { machine.regs[11] = 0xFF134CU; }

void ref_adda(Machine& machine) {
    const auto value = static_cast<std::int16_t>(machine.regs[7] & 0xFFFFU);
    machine.regs[11] += static_cast<std::uint32_t>(static_cast<std::int32_t>(value));
}

void ref_move_w(Machine& machine) {
    const auto source = machine.regs[14];
    const auto value = ref_read(machine, source, 2);
    machine.regs[14] = source + 2U;
    const auto destination = machine.regs[11];
    ref_write(machine, destination, 2, value);
    machine.regs[11] = destination + 2U;
    ref_move_flags(machine, value, 2);
}

void ref_add(Machine& machine) {
    const auto lhs = machine.regs[2];
    const auto rhs = machine.regs[1];
    const auto result = lhs + rhs;
    const auto carry = static_cast<std::uint64_t>(lhs) + rhs > 0xFFFFFFFFULL;
    const auto overflow = ((~(lhs ^ rhs) & (lhs ^ result)) & 0x80000000U) != 0;
    machine.regs[2] = result;
    machine.regs[17] = (machine.regs[17] & ~0x1FU) | (carry ? 0x11U : 0U) |
                       (result & 0x80000000U ? 8U : 0U) | (result ? 0U : 4U) |
                       (overflow ? 2U : 0U);
}

void ref_add_w_postincrement(Machine& machine) {
    const auto address = machine.regs[8];
    const auto rhs = ref_read(machine, address, 2) & 0xFFFFU;
    machine.regs[8] = address + 2U;
    const auto lhs = machine.regs[0] & 0xFFFFU;
    const auto result = (lhs + rhs) & 0xFFFFU;
    const auto carry = lhs + rhs > 0xFFFFU;
    const auto overflow = ((~(lhs ^ rhs) & (lhs ^ result)) & 0x8000U) != 0;
    machine.regs[0] = (machine.regs[0] & 0xFFFF0000U) | result;
    machine.regs[17] = (machine.regs[17] & ~0x1FU) | (carry ? 0x11U : 0U) |
                       (result & 0x8000U ? 8U : 0U) | (result ? 0U : 4U) |
                       (overflow ? 2U : 0U);
}

void ref_tst(Machine& machine, std::uint32_t address, unsigned width) {
    ref_move_flags(machine, ref_read(machine, address, static_cast<int>(width)), width);
}

using Action = std::function<void(BasicBlockApi&)>;
using Reference = std::function<void(Machine&)>;

std::string state(const Machine& machine) {
    std::ostringstream out;
    out << "regs=";
    for (unsigned i = 0; i < machine.regs.size(); ++i)
        out << (i ? "," : "[") << "0x" << std::hex << machine.regs[i];
    out << "] memory=";
    for (const auto& [address, value] : machine.memory)
        out << "(0x" << std::hex << address << ":0x" << unsigned(value) << ")";
    out << " cycles=" << std::dec << machine.cycles;
    return out.str();
}

std::string hex_value(std::uint32_t value, unsigned width) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0') << std::setw(width) << value;
    return out.str();
}

[[noreturn]] void mismatch(const std::string& name, std::uint16_t opcode,
                           const std::string& decoded, const Machine& before,
                           const Machine& expected, const Machine& actual,
                           const std::string& detail) {
    std::ostringstream out;
    out << "FIRST_MISMATCH case=" << name << " opcode=0x" << std::hex << opcode
        << " decoded=\"" << decoded << "\" detail=" << detail
        << " pre=" << state(before) << " expected=" << state(expected)
        << " actual=" << state(actual);
    throw std::runtime_error(out.str());
}

void run_case(const std::string& name, std::uint16_t opcode, const std::string& decoded,
              const Machine& before, Action action, Reference reference) {
    Machine expected = before;
    reference(expected);
    Machine actual = before;
    try {
        auto api = api_for(actual);
        action(api);
    } catch (...) {
        Machine::current = nullptr;
        throw;
    }
    Machine::current = nullptr;
    if (actual.regs != expected.regs)
        mismatch(name, opcode, decoded, before, expected, actual, "registers");
    if (actual.memory != expected.memory)
        mismatch(name, opcode, decoded, before, expected, actual, "memory");
    if (actual.accesses != expected.accesses)
        mismatch(name, opcode, decoded, before, expected, actual, "accesses");
    if (actual.cycles != expected.cycles)
        mismatch(name, opcode, decoded, before, expected, actual, "cycles");
}

void put(std::vector<std::uint8_t>& rom, std::uint32_t address,
         std::initializer_list<std::uint8_t> bytes) {
    std::copy(bytes.begin(), bytes.end(), rom.begin() + address);
}

void test_decode_and_provenance() {
    std::vector<std::uint8_t> rom(0x200, 0);
    put(rom, 0x20, {0x48, 0xE7, 0x01, 0x10, 0x42, 0x47, 0x1E, 0x1E,
                    0x47, 0xF9, 0x00, 0xFF, 0x13, 0x4C, 0xD6, 0xC7,
                    0x1E, 0x1E, 0x36, 0xDE});
    put(rom, 0x80, {0x4D, 0xF9, 0x00, 0xFF, 0x06, 0x28});
    put(rom, 0xA0, {0xD4, 0x81});
    put(rom, 0xC0, {0x66, 0x08});
    put(rom, 0xD0, {0x51, 0xC8, 0xFF, 0xFC});
    put(rom, 0xE0, {0x51, 0xC8, 0xFF, 0xFC});
    const std::array blocks{
        std::pair{0x20U, 0x34U}, std::pair{0x80U, 0x86U}, std::pair{0xA0U, 0xA2U}};
    struct Expectation {
        std::uint16_t opcode;
        std::size_t bytes;
        const char* operation;
        std::uint8_t width;
        std::optional<oasis::tools::OperandKind> source_kind;
        std::optional<oasis::tools::OperandKind> destination_kind;
        std::uint32_t source_value;
        std::uint8_t source_register;
        std::uint8_t destination_register;
    };
    using Kind = oasis::tools::OperandKind;
    const std::array expected{
        std::vector<Expectation>{
            {0x48E7, 4, "movem", 4, Kind::register_list, Kind::predecrement, 0x0880, 0, 7},
            {0x4247, 2, "clr", 2, std::nullopt, Kind::data_register, 0, 0, 7},
            {0x1E1E, 2, "move", 1, Kind::postincrement, Kind::data_register, 0, 6, 7},
            {0x47F9, 6, "lea", 4, Kind::absolute_long, Kind::address_register, 0xFF134C, 0, 3},
            {0xD6C7, 2, "adda", 2, Kind::data_register, Kind::address_register, 0, 7, 3},
            {0x1E1E, 2, "move", 1, Kind::postincrement, Kind::data_register, 0, 6, 7},
            {0x36DE, 2, "move", 2, Kind::postincrement, Kind::postincrement, 0, 6, 3}},
        std::vector<Expectation>{
            {0x4DF9, 6, "lea", 4, Kind::absolute_long, Kind::address_register, 0xFF0628, 0, 6}},
        std::vector<Expectation>{
            {0xD481, 2, "add", 4, Kind::data_register, Kind::data_register, 0, 1, 2}}};
    for (std::size_t block_index = 0; block_index < blocks.size(); ++block_index) {
        const auto [start, end] = blocks[block_index];
        const auto block = oasis::hybrid::generate_block(rom, start, end);
        if (block.instructions.size() != expected[block_index].size())
            throw std::runtime_error("decode instruction count mismatch");
        for (std::size_t i = 0; i < block.instructions.size(); ++i) {
            const auto& instruction = block.instructions[i];
            const auto& item = expected[block_index][i];
            if (instruction.opcode != item.opcode || instruction.bytes.size() != item.bytes ||
                !instruction.supported || !instruction.exact ||
                instruction.exact->operation != item.operation ||
                instruction.exact->width_bytes != item.width ||
                (instruction.address + instruction.bytes.size() !=
                 (i + 1U == block.instructions.size() ? end : block.instructions[i + 1U].address)))
                throw std::runtime_error("decode length/exact metadata mismatch");
            for (const auto& [operand, kind, value, reg] :
                 {std::tuple{instruction.exact->source, item.source_kind, item.source_value,
                             item.source_register},
                  std::tuple{instruction.exact->destination, item.destination_kind, 0U,
                             item.destination_register}}) {
                const auto register_kind = kind &&
                    (*kind == Kind::data_register || *kind == Kind::address_register ||
                     *kind == Kind::postincrement || *kind == Kind::predecrement);
                if (kind.has_value() != operand.has_value() ||
                    (kind && (operand->kind != *kind || (register_kind && operand->register_index != reg) ||
                              (kind == Kind::absolute_long || kind == Kind::register_list) &&
                                  operand->value != value)))
                    throw std::runtime_error("decode operand metadata mismatch at block " +
                                             std::to_string(block_index) + " instruction " +
                                             std::to_string(i) + " actual_kind=" +
                                             (operand ? std::to_string(static_cast<int>(operand->kind)) : "none") +
                                             " actual_reg=" + (operand ? std::to_string(operand->register_index) : "none") +
                                             " actual_value=" + (operand ? hex_value(operand->value, 8) : "none"));
            }
        }
        const auto emitted = oasis::hybrid::emit_translation_unit({block});
        for (const auto& instruction : block.instructions) {
            std::ostringstream provenance;
            provenance << "guest 0x" << hex_value(instruction.address, 6) << " opcode 0x"
                       << hex_value(instruction.opcode, 4);
            if (emitted.find(provenance.str()) == std::string::npos)
                throw std::runtime_error("generated provenance chain is incomplete");
        }
    }
    for (const auto& [start, end, opcode, operation, width, condition, target] : {
             std::tuple{0xC0U, 0xC2U, 0x6608U, "bne", 1U, 6U, 0xCAU},
             std::tuple{0xD0U, 0xD4U, 0x51C8U, "dbf", 2U, 1U, 0xCEU},
             std::tuple{0xE0U, 0xE4U, 0x51C8U, "dbf", 2U, 1U, 0xDEU}}) {
        const auto block = oasis::hybrid::generate_block(rom, start, end);
        if (block.instructions.size() != 1U || block.instructions[0].opcode != opcode ||
            !block.instructions[0].exact || block.instructions[0].exact->operation != operation ||
            block.instructions[0].exact->branch_width_bytes != width ||
            !block.instructions[0].branch_condition_code ||
            *block.instructions[0].branch_condition_code != condition ||
            !block.instructions[0].direct_target || *block.instructions[0].direct_target != target)
            throw std::runtime_error("branch decode/provenance metadata mismatch");
        const auto emitted = oasis::hybrid::emit_translation_unit({block});
        if (emitted.find("guest 0x" + hex_value(start, 6)) == std::string::npos)
            throw std::runtime_error("branch generated provenance is incomplete");
    }
}

bool ref_condition(std::uint32_t sr, unsigned condition) {
    const bool c = (sr & 1U) != 0, v = (sr & 2U) != 0;
    const bool z = (sr & 4U) != 0, n = (sr & 8U) != 0;
    switch (condition & 0x0FU) {
    case 0: return true; case 1: return false; case 2: return !c && !z;
    case 3: return c || z; case 4: return !c; case 5: return c;
    case 6: return !z; case 7: return z; case 8: return !v;
    case 9: return v; case 10: return !n; case 11: return n;
    case 12: return n == v; case 13: return n != v; case 14: return !z && n == v;
    default: return z || n != v;
    }
}

void ref_bcc(Machine& machine, unsigned condition, unsigned target) {
    if (ref_condition(machine.regs[17], condition)) machine.regs[16] = target;
    else machine.cycles -= 14;
}

void ref_dbcc(Machine& machine, unsigned condition, unsigned data_register,
              unsigned target) {
    if (ref_condition(machine.regs[17], condition)) return;
    const auto value = machine.regs[data_register];
    const auto result = (value - 1U) & 0xFFFFU;
    machine.regs[data_register] = (value & 0xFFFF0000U) | result;
    if (result != 0xFFFFU) { machine.regs[16] = target; machine.cycles -= 14; }
    else machine.cycles += 14;
}

void test_semantics() {
    for (const auto sr : {0xA713U, 0xA717U}) {
        auto before = seed();
        before.regs[16] = 0x3A9B2U; before.regs[17] = sr;
        run_case("BNE.S condition and PC", 0x6608, "bne.s loc_03A9BC", before,
                 [](BasicBlockApi& api) { branch_condition(api, 6, 0x3A9BCU, -14); },
                 [](Machine& machine) { ref_bcc(machine, 6, 0x3A9BCU); });
    }
    for (const auto value : {0x00000000U, 0x00001234U, 0xFFFFFFFFU}) {
        auto before = seed();
        before.regs[0] = value; before.regs[16] = 0x60312U; before.regs[17] = 0;
        run_case("DBF.W counter and PC", 0x51C8, "dbf D0,loc_060310", before,
                 [](BasicBlockApi& api) { dbcc(api, 1, 0, 0x60310U); },
                 [](Machine& machine) { ref_dbcc(machine, 1, 0, 0x60310U); });
    }
    {
        auto before = seed(); before.regs[0] = 0x12345678U; before.regs[17] = 4;
        run_case("DBNE.W condition true", 0x56C8, "dbne D0,loc", before,
                 [](BasicBlockApi& api) { dbcc(api, 6, 0, 0x100U); },
                 [](Machine& machine) { ref_dbcc(machine, 6, 0, 0x100U); });
    }
    for (const auto mask : {0x0880U, 0x8001U, 0xFFFFU}) {
        auto before = seed();
        before.regs[15] = 0x00000200U;
        run_case("MOVEM.L predecrement mask", 0x48E7, "movem.l <list>,-(A7)", before,
                 [mask](BasicBlockApi& api) { movem_l_predecrement(api, mask); },
                 [mask](Machine& machine) { ref_movem(machine, mask); });
    }
    for (const auto value : {0x00000000U, 0x00008000U, 0x12347FFFU}) {
        auto before = seed();
        before.regs[3] = value;
        run_case("CLR.W D3", 0x4243, "clr.w D3", before,
                 [](BasicBlockApi& api) { clear_w_data_register(api, 3); }, ref_clear);
    }
    for (const auto [width, value] : {std::pair{1U, 0x00U}, std::pair{1U, 0x80U},
                                      std::pair{2U, 0x7FFFU}, std::pair{2U, 0x8000U}}) {
        auto before = seed(); before.regs[17] = 0xA713U;
        before.memory[0x180] = static_cast<std::uint8_t>(value >> 8U);
        before.memory[0x181] = static_cast<std::uint8_t>(value);
        const auto address = width == 1U ? 0x181U : 0x180U;
        run_case("TST absolute memory", 0x4A79, "tst.[b/w] (abs.l)", before,
                 [address, width](BasicBlockApi& api) { test_absolute_long(api, address, width); },
                 [address, width](Machine& machine) { ref_tst(machine, address, width); });
    }
    for (const auto value : {0x00U, 0x80U, 0x7FU}) {
        auto before = seed();
        before.regs[14] = 0xFFFFFFFFU;
        before.memory[0xFFFFFFFFU] = static_cast<std::uint8_t>(value);
        run_case("MOVE.B postincrement", 0x1E1E, "move.b (A6)+,D7", before,
                 [](BasicBlockApi& api) { move_b_postincrement_to_data_register(api, 6, 7); },
                 ref_move_b);
    }
    for (const auto address : {0x00000000U, 0xFFFFFFFFU}) {
        auto before = seed();
        before.regs[11] = address;
        run_case("LEA absolute long", 0x47F9, "lea.l (abs.l),A3", before,
                 [](BasicBlockApi& api) { lea_absolute_long(api, 0xFF134CU, 3); }, ref_lea);
    }
    for (const auto value : {0x0000U, 0x0001U, 0xFFFFU, 0x8000U}) {
        auto before = seed();
        before.regs[7] = 0xCAFE0000U | value;
        before.regs[11] = value == 0x8000U ? 0x00000001U : 0xFFFFFFFFU;
        run_case("ADDA.W D7,A3", 0xD6C7, "adda.w D7,A3", before,
                 [](BasicBlockApi& api) { adda_w_data_to_address(api, 7, 3); }, ref_adda);
    }
    for (const auto value : {0x0000U, 0x8000U, 0x7FFFU}) {
        auto before = seed();
        before.regs[14] = 0x00000100U;
        before.regs[11] = 0x00000100U;
        before.memory[0x100] = static_cast<std::uint8_t>(value >> 8U);
        before.memory[0x101] = static_cast<std::uint8_t>(value);
        run_case("MOVE.W postincrement overlap", 0x36DE, "move.w (A6)+,(A3)+", before,
                 [](BasicBlockApi& api) { move_w_postincrement_to_postincrement(api, 6, 3); },
                 ref_move_w);
    }
    for (const auto pair : {std::pair{0x00000000U, 0x00000000U},
                            std::pair{0xFFFFFFFFU, 0x00000001U},
                            std::pair{0x7FFFFFFFU, 0x00000001U},
                            std::pair{0x80000000U, 0x80000000U},
                            std::pair{0x80000000U, 0x00000001U}}) {
        auto before = seed();
        before.regs[2] = pair.first;
        before.regs[1] = pair.second;
        run_case("ADD.L D1,D2", 0xD481, "add.l D1,D2", before,
                 [](BasicBlockApi& api) { add_l_data_to_data(api, 1, 2); }, ref_add);
    }
    for (const auto pair : {std::pair{0x0000U, 0x0000U},
                            std::pair{0x7FFFU, 0x0001U},
                            std::pair{0x8000U, 0x8000U},
                            std::pair{0xFFFFU, 0x0001U}}) {
        auto before = seed();
        before.regs[0] = 0xCAFE0000U | pair.first;
        before.regs[8] = 0x00000100U;
        before.memory[0x100] = static_cast<std::uint8_t>(pair.second >> 8U);
        before.memory[0x101] = static_cast<std::uint8_t>(pair.second);
        run_case("ADD.W postincrement to data", 0xD058, "add.w (A0)+,D0", before,
                 [](BasicBlockApi& api) {
                     add_w_postincrement_to_data_register(api, 0, 0);
                 }, ref_add_w_postincrement);
    }
}

} // namespace

int main() {
    test_decode_and_provenance();
    test_semantics();
    return 0;
}
