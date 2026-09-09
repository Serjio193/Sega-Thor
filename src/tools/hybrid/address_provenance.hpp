#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace oasis::hybrid {

enum class MemoryClass {
    ROM, MAIN_RAM, VDP, Z80_RAM, Z80_CONTROL, YM2612, PSG, IO,
    CART_SRAM, OTHER_HARDWARE, UNMAPPED, UNKNOWN,
};

[[nodiscard]] MemoryClass classify_memory_address(std::uint32_t address,
                                                   std::uint32_t rom_size) noexcept;
[[nodiscard]] std::string_view memory_class_name(MemoryClass value) noexcept;

class AddressProvenanceObserver {
public:
    using RegisterReader = unsigned (*)(unsigned);
    AddressProvenanceObserver(std::filesystem::path output, std::uint32_t rom_size,
                              RegisterReader registers);
    ~AddressProvenanceObserver();
    AddressProvenanceObserver(const AddressProvenanceObserver&) = delete;
    AddressProvenanceObserver& operator=(const AddressProvenanceObserver&) = delete;
    void event(int type, int width, std::uint32_t address, std::uint32_t value, unsigned frame);
    void finish();
    [[nodiscard]] std::uint64_t instruction_count() const noexcept { return instructions_; }
    [[nodiscard]] std::uint64_t unclosed_count() const noexcept { return unclosed_; }

private:
    struct State;
    State* state_;
    std::uint64_t instructions_{};
    std::uint64_t unclosed_{};
};

} // namespace oasis::hybrid
