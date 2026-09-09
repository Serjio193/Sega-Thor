#include "tools/hybrid/caller_attribution.hpp"

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

namespace {
struct Fake {
    std::array<unsigned, 18> registers{};
    std::unordered_map<unsigned, unsigned> memory;
    int cycles{1234};
    int refresh{5678};
    static Fake* current;

    static unsigned reg(unsigned index) { return current->registers[index]; }
    static int peek(unsigned address) {
        const auto found = current->memory.find(address & 0xFFFFFFU);
        return found == current->memory.end() ? -1 : static_cast<int>(found->second);
    }
    static int cycle_value() { return current->cycles; }
    static int refresh_value() { return current->refresh; }
    void byte(unsigned address, unsigned value) { memory[address & 0xFFFFFFU] = value & 0xFFU; }
    void word(unsigned address, unsigned value) { byte(address, value >> 8U); byte(address + 1U, value); }
    void longword(unsigned address, unsigned value) {
        word(address, value >> 16U); word(address + 2U, value);
    }
};
Fake* Fake::current = nullptr;
}

int main() {
    Fake fake;
    Fake::current = &fake;
    const oasis::hybrid::CallerAttributionApi api{
        Fake::reg, Fake::peek, Fake::cycle_value, Fake::refresh_value};
    const auto path = std::filesystem::temp_directory_path() / "oasis_caller_attribution_test.json";
    fake.registers[15] = 0x00FFFF00;
    fake.word(0x0604F6, 0x6100); fake.word(0x0604F8, 0xFFC4);
    fake.word(0x060BCC, 0x6100); fake.word(0x060BCE, 0xF8EE);
    oasis::hybrid::CallerAttributionObserver observer(path, api);
    fake.longword(0x00FFFF00, 0x0604FA);
    observer.entry(0x0604BC, 0x0604F6, 10);
    fake.longword(0x00FFFF00, 0x060BD0);
    observer.entry(0x0604BC, 0x060BCC, 20);
    assert(observer.records().size() == 2);
    assert(observer.records()[0].classification == oasis::hybrid::CallerClassification::Caller604F6);
    assert(observer.records()[0].caller_pc == 0x0604F0 && observer.records()[0].return_pc == 0x0604FA);
    assert(observer.records()[1].classification == oasis::hybrid::CallerClassification::Caller60BCC);
    assert(observer.records()[1].caller_pc == 0x060BC4 && observer.records()[1].return_pc == 0x060BD0);
    assert(observer.unknown_count() == 0);
    const auto first = oasis::hybrid::caller_attribution_to_json(observer.records(), observer.unknown_count());
    observer.finish();
    std::ifstream input(path);
    const std::string persisted((std::istreambuf_iterator<char>(input)), {});
    input.close();
    assert(first == persisted);
    assert(persisted.find("0x000604F6") != std::string::npos);
    assert(persisted.find("0x00060BCC") != std::string::npos);
    std::filesystem::remove(path);
}
