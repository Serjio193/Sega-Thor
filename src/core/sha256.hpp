#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace oasis::core {

[[nodiscard]] std::string compute_sha256(std::span<const std::uint8_t> data);

} // namespace oasis::core
