#include "tools/hybrid/address_provenance.hpp"

#include <array>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>

namespace oasis::hybrid {
namespace {
constexpr int kExecute = 1;
constexpr int kRead = 2;
constexpr int kWrite = 4;
constexpr int kPost = 1 << 14;
constexpr unsigned kRegisterCount = 18;

struct Access {
    std::uint32_t address{};
    unsigned width{};
    char direction{};
    bool operator<(const Access& other) const noexcept {
        return std::tie(address, width, direction) <
               std::tie(other.address, other.width, other.direction);
    }
};
struct Record {
    std::uint64_t count{};
    std::map<unsigned, std::uint64_t> frame_counts;
    std::map<Access, std::uint64_t> accesses;
    std::map<std::string, std::uint64_t> sequences;
    std::map<std::string, std::uint64_t> transitions;
    unsigned first_frame{};
    unsigned last_frame{};
};
std::string hex(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}
std::string registers_text(const std::array<unsigned, kRegisterCount>& values,
                           unsigned first, unsigned count) {
    std::ostringstream out;
    for (unsigned i = 0; i < count; ++i) {
        if (i) out << ',';
        out << hex(values[first + i]);
    }
    return out.str();
}
std::string access_text(const Access& access) {
    std::ostringstream out;
    out << access.direction << access.width << ':' << hex(access.address);
    return out.str();
}
} // namespace

MemoryClass classify_memory_address(std::uint32_t address, std::uint32_t rom_size) noexcept {
    address &= 0xFFFFFFU;
    if (address < rom_size) return MemoryClass::ROM;
    if (address >= 0xFF0000U) return MemoryClass::MAIN_RAM;
    if (address == 0xC00011U) return MemoryClass::PSG;
    if (address >= 0xC00000U && address <= 0xC0001FU) return MemoryClass::VDP;
    if (address >= 0xA04000U && address <= 0xA04003U) return MemoryClass::YM2612;
    if (address == 0xA11100U || address == 0xA11200U) return MemoryClass::Z80_CONTROL;
    if (address >= 0xA00000U && address <= 0xA0FFFFU) return MemoryClass::Z80_RAM;
    if (address >= 0xA10000U && address <= 0xA1001FU) return MemoryClass::IO;
    // Cartridge SRAM is mapper-dependent; address alone is not sufficient proof.
    if (address >= 0x200000U && address < 0x400000U && address >= rom_size)
        return MemoryClass::UNKNOWN;
    if (address >= 0x400000U && address < 0xA00000U) return MemoryClass::UNMAPPED;
    if (address >= 0xA10020U && address < 0xE00000U) return MemoryClass::OTHER_HARDWARE;
    return MemoryClass::UNKNOWN;
}

std::string_view memory_class_name(MemoryClass value) noexcept {
    switch (value) {
    case MemoryClass::ROM: return "ROM";
    case MemoryClass::MAIN_RAM: return "MAIN_RAM";
    case MemoryClass::VDP: return "VDP";
    case MemoryClass::Z80_RAM: return "Z80_RAM";
    case MemoryClass::Z80_CONTROL: return "Z80_CONTROL";
    case MemoryClass::YM2612: return "YM2612";
    case MemoryClass::PSG: return "PSG";
    case MemoryClass::IO: return "IO";
    case MemoryClass::CART_SRAM: return "CART_SRAM";
    case MemoryClass::OTHER_HARDWARE: return "OTHER_HARDWARE";
    case MemoryClass::UNMAPPED: return "UNMAPPED";
    case MemoryClass::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

struct AddressProvenanceObserver::State {
    std::filesystem::path output;
    std::uint32_t rom_size{};
    RegisterReader registers{};
    std::map<std::uint32_t, Record> records;
    std::uint32_t current_pc{};
    std::array<unsigned, kRegisterCount> before{};
    std::string sequence;
    bool active{};
};

AddressProvenanceObserver::AddressProvenanceObserver(std::filesystem::path output,
                                                     std::uint32_t rom_size,
                                                     RegisterReader registers)
    : state_(new State{std::move(output), rom_size, registers}) {
    if (!registers) throw std::invalid_argument("address observer requires register reader");
}
AddressProvenanceObserver::~AddressProvenanceObserver() {
    try { finish(); } catch (...) { }
    delete state_;
}
void AddressProvenanceObserver::event(int type, int width, std::uint32_t address,
                                      std::uint32_t, unsigned frame) {
    address &= 0xFFFFFFU;
    if (type == kExecute) {
        if (state_->active) ++unclosed_;
        state_->active = true;
        state_->current_pc = address;
        state_->sequence.clear();
        for (unsigned i = 0; i < kRegisterCount; ++i) state_->before[i] = state_->registers(i);
        auto& record = state_->records[address];
        ++record.count;
        ++record.frame_counts[frame];
        if (!record.first_frame) record.first_frame = frame;
        record.last_frame = frame;
        ++instructions_;
        return;
    }
    if (!state_->active) return;
    if (type == kRead || type == kWrite) {
        const Access access{address, static_cast<unsigned>(width), type == kRead ? 'R' : 'W'};
        auto& record = state_->records[state_->current_pc];
        ++record.accesses[access];
        if (!state_->sequence.empty()) state_->sequence += ';';
        state_->sequence += access_text(access);
        return;
    }
    if (type == kPost) {
        auto& record = state_->records[state_->current_pc];
        ++record.sequences[state_->sequence.empty() ? "NO_DATA_BUS_ACCESS" : state_->sequence];
        std::array<unsigned, kRegisterCount> after{};
        for (unsigned i = 0; i < kRegisterCount; ++i) after[i] = state_->registers(i);
        ++record.transitions[registers_text(state_->before, 8, 8) + "->" +
                             registers_text(after, 8, 8)];
        state_->active = false;
    }
}
void AddressProvenanceObserver::finish() {
    if (!state_) return;
    if (state_->active) { ++unclosed_; state_->active = false; }
    if (state_->output.empty()) return;
    std::ofstream output(state_->output);
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output << "{\n\"schema\":\"oasis.hybrid.address-provenance.v1\",\n"
           << "\"rom_size\":" << state_->rom_size << ",\n\"instructions\":"
           << instructions_ << ",\n\"unclosed\":" << unclosed_ << ",\n\"pcs\":[";
    bool first_pc = true;
    for (const auto& [pc, record] : state_->records) {
        if (!first_pc) output << ',';
        first_pc = false;
        output << "{\"pc\":\"" << hex(pc) << "\",\"count\":" << record.count
               << ",\"first_frame\":" << record.first_frame << ",\"last_frame\":"
               << record.last_frame << ",\"frame_counts\":[";
        bool first_frame = true;
        for (const auto& [frame, count] : record.frame_counts) {
            if (!first_frame) output << ',';
            first_frame = false;
            output << "{\"frame\":" << frame << ",\"count\":" << count << '}';
        }
        output << "],\"provenance\":\"unresolved exact evidence\",\"accesses\":[";
        bool first_access = true;
        for (const auto& [access, count] : record.accesses) {
            if (!first_access) output << ',';
            first_access = false;
            output << "{\"address\":\"" << hex(access.address) << "\",\"width\":"
                   << access.width << ",\"direction\":\"" << access.direction
                   << "\",\"count\":" << count << ",\"memory_class\":\""
                   << memory_class_name(classify_memory_address(access.address, state_->rom_size))
                   << "\"}";
        }
        output << "],\"ordered_sequences\":[";
        bool first_sequence = true;
        for (const auto& [sequence, count] : record.sequences) {
            if (!first_sequence) output << ',';
            first_sequence = false;
            output << "{\"sequence\":\"" << sequence << "\",\"count\":" << count << '}';
        }
        output << "],\"register_transitions\":[";
        bool first_transition = true;
        for (const auto& [transition, count] : record.transitions) {
            if (!first_transition) output << ',';
            first_transition = false;
            output << "{\"transition\":\"" << transition << "\",\"count\":" << count << '}';
        }
        output << "]}";
    }
    output << "]\n}\n";
}
} // namespace oasis::hybrid
