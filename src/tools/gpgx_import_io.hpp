#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::tools::gpgx {

struct Range {
    std::uint32_t start{};
    std::uint32_t end{};
    std::string classification;
    bool data{};
};

struct Capture {
    std::string id;
    std::string bitmap_kind;
    std::string bitmap_sha256;
    std::size_t address_count{};
    std::uint64_t frames{};
    std::uint64_t new_pcs{};
    std::string build_id;
};

struct Store {
    std::set<std::uint32_t> addresses;
    std::map<std::uint32_t, std::set<std::string>> facts;
    std::vector<Capture> captures;
    std::set<std::string> build_ids;
    bool legacy_provenance = false;
};

[[nodiscard]] std::vector<Range> load_ranges(const std::filesystem::path& path);
[[nodiscard]] Store load_store(const std::filesystem::path& path,
                               std::string_view canonical_sha);
[[nodiscard]] std::string field_string(std::string_view text, std::string_view key);
[[nodiscard]] std::uint64_t field_number(std::string_view text, std::string_view key);
[[nodiscard]] bool allowed_gpgx_build(std::string_view build_id);

} // namespace oasis::tools::gpgx
