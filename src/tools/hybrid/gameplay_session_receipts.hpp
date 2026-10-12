#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace oasis::hybrid::session {

void write_replay_result(const std::filesystem::path& path, unsigned frames,
                         std::size_t polls, const std::string& input_sha256,
                         const std::string& landmarks_sha256,
                         const std::string& calls_sha256,
                         const std::string& census_sha256,
                         const std::string& full_calls_sha256);

} // namespace oasis::hybrid::session
