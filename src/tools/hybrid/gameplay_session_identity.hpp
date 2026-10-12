#pragma once

#include <filesystem>
#include <map>
#include <string>

namespace oasis::hybrid::session {

using Manifest = std::map<std::string, std::string>;

[[nodiscard]] std::string file_sha256(const std::filesystem::path& path);
[[nodiscard]] std::string executable_path();
[[nodiscard]] Manifest read_manifest(const std::filesystem::path& path);
void write_manifest(const std::filesystem::path& path, const Manifest& values);
[[nodiscard]] std::string config_identity(unsigned frames, const std::string& input_mode);

} // namespace oasis::hybrid::session
