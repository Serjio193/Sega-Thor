#include "tools/hybrid/address_provenance.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
unsigned registers_state[18]{};
unsigned read_register(unsigned index) { return registers_state[index]; }
}

int main() {
    using oasis::hybrid::MemoryClass;
    assert(oasis::hybrid::classify_memory_address(0x000100, 0x300000) == MemoryClass::ROM);
    assert(oasis::hybrid::classify_memory_address(0xFF1234, 0x300000) == MemoryClass::MAIN_RAM);
    assert(oasis::hybrid::classify_memory_address(0xA00003, 0x300000) == MemoryClass::Z80_RAM);
    assert(oasis::hybrid::classify_memory_address(0xA11100, 0x300000) == MemoryClass::Z80_CONTROL);
    assert(oasis::hybrid::classify_memory_address(0xC00011, 0x300000) == MemoryClass::PSG);
    const auto path = std::filesystem::temp_directory_path() / "oasis_address_provenance_test.json";
    {
        oasis::hybrid::AddressProvenanceObserver observer(path, 0x300000, read_register);
        observer.event(1, 0, 0x00026A, 0, 7);
        observer.event(4, 4, 0xFF1234, 0x12345678, 7);
        registers_state[14] = 0xFF1238;
        observer.event(1 << 14, 0, 0x00026E, 0, 7);
        assert(observer.instruction_count() == 1);
        assert(observer.unclosed_count() == 0);
        observer.finish();
    }
    {
        std::ifstream input(path);
        const std::string text((std::istreambuf_iterator<char>(input)), {});
        assert(text.find("oasis.hybrid.address-provenance.v1") != std::string::npos);
        assert(text.find("MAIN_RAM") != std::string::npos);
        assert(text.find("FF1238") != std::string::npos);
    }
    std::filesystem::remove(path);
}
