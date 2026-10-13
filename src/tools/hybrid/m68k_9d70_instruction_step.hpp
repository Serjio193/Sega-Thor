#pragma once

#include <cstdint>

namespace oasis::hybrid { class ExternalLibrary; }

namespace oasis::hybrid {

struct M68k9D70StepApi {
    void* context{};
    std::uint32_t (*read_register)(void*, unsigned){};
    void (*write_register)(void*, unsigned, std::uint32_t){};
    int (*peek)(void*, unsigned){};
    std::uint32_t (*read)(void*, unsigned, int){};
    void (*write)(void*, unsigned, int, std::uint32_t){};
    std::uint32_t (*fetch16)(void*){};
    void (*set_return_state)(void*, unsigned, unsigned, unsigned){};
    void (*begin_instruction)(void*, unsigned){};
    void (*finish_instruction)(void*, unsigned){};
    void (*add_cycles)(void*, int){};
    int (*word_not_taken_adjustment)(void*){};
    int (*byte_not_taken_adjustment)(void*){};
    void (*observe_instruction)(void*, unsigned, unsigned){};
    void (*skip_bus_refresh)(void*){};
    int (*read_scheduler_field)(void*, unsigned){};
    unsigned (*read_transfer_source_pc)(void*){};
};

enum class M68k9D70StepResult { NotHandled, InstructionStepped, ContractFault };

class M68k9D70InstructionStepper {
public:
    [[nodiscard]] M68k9D70StepResult step(unsigned pc,
                                           const M68k9D70StepApi& api) noexcept;
    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    bool active_{};
    bool consumed_{};
};

void install_m68k_9d70_instruction_step(
    ExternalLibrary& core, void (*observe)(unsigned, unsigned),
    void (*fault)(unsigned));

} // namespace oasis::hybrid
