#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace oasis::hybrid {

enum class CheckpointRepresentationKind {
    HostPointer,
    FunctionPointer,
    AbiPadding,
};

struct CheckpointRepresentationSpan {
    std::size_t offset;
    std::size_t size;
    CheckpointRepresentationKind kind;
    std::string_view name;
};

struct GpgxCheckpointLayout {
    std::size_t state_size;
    std::size_t ym2612_offset;
    std::size_t ym2612_size;
    std::size_t z80_offset;
    std::size_t z80_size;
    std::string_view state_version;
    std::span<const CheckpointRepresentationSpan> representation_spans;
};

[[nodiscard]] const GpgxCheckpointLayout& gpgx_checkpoint_layout();

} // namespace oasis::hybrid
