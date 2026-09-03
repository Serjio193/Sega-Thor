#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

inline constexpr std::uint32_t kReAccelerationEntry = 0x00060004;
inline constexpr std::size_t kReAccelerationByteBudget = 0x1200;

enum class FlowKind {
    none,
    direct_branch,
    direct_call,
    direct_jump,
    indirect_call,
    indirect_jump,
    return_instruction,
    unsupported,
};

enum class MemoryKind { rom, ram, other };
enum class MemoryAccess { read, write, address, unknown };

struct ImmediateConstant {
    std::uint32_t value{};
    std::uint8_t width_bytes{};
};

struct MemoryReference {
    std::uint32_t address{};
    std::uint8_t width_bytes{};
    MemoryKind kind{MemoryKind::other};
    MemoryAccess access{MemoryAccess::unknown};
};

struct UnresolvedMemoryReference {
    std::uint8_t mode{};
    std::uint8_t register_index{};
    std::string reason;
};

struct UnsupportedAddressing {
    std::uint8_t mode{};
    std::uint8_t register_index{};
    std::string reason;
};

struct DecodedInstruction {
    std::uint32_t address{};
    std::uint16_t opcode{};
    std::vector<std::uint8_t> bytes;
    std::string mnemonic;
    bool supported{};
    FlowKind flow{FlowKind::none};
    std::optional<std::uint32_t> direct_target;
    std::optional<std::uint8_t> branch_condition_code;
    std::vector<std::string> addressing_modes;
    std::vector<ImmediateConstant> immediate_constants;
    std::vector<MemoryReference> memory_references;
    std::vector<UnresolvedMemoryReference> unresolved_memory_references;
    std::vector<UnsupportedAddressing> unsupported_addressing;
};

struct BasicBlock {
    std::uint32_t start{};
    std::uint32_t end{};
    std::vector<std::uint32_t> instruction_addresses;
};

struct ControlFlowEdge {
    std::uint32_t source{};
    std::uint32_t target{};
    FlowKind kind{FlowKind::none};
};

struct UnresolvedControlFlow {
    std::uint32_t address{};
    std::uint16_t opcode{};
    FlowKind kind{FlowKind::none};
};

struct DecodedSlice {
    std::uint32_t entry{};
    std::uint32_t range_end{};
    std::vector<DecodedInstruction> instructions;
    std::vector<BasicBlock> basic_blocks;
    std::vector<ControlFlowEdge> control_flow;
    std::vector<UnresolvedControlFlow> unresolved_control_flow;
    std::vector<std::uint32_t> unsupported_instruction_addresses;
};

struct DecodeOptions {
    std::uint32_t entry{kReAccelerationEntry};
    std::size_t byte_budget{kReAccelerationByteBudget};
    std::size_t instruction_budget{4096};
};

[[nodiscard]] DecodedSlice decode_m68k_slice(
    std::span<const std::uint8_t> rom, const DecodeOptions& options = {});

[[nodiscard]] std::string flow_kind_name(FlowKind kind);
[[nodiscard]] std::string memory_kind_name(MemoryKind kind);
[[nodiscard]] std::string memory_access_name(MemoryAccess access);
[[nodiscard]] std::string slice_to_json(const DecodedSlice& slice);
[[nodiscard]] std::string slice_to_text(const DecodedSlice& slice);

} // namespace oasis::tools
