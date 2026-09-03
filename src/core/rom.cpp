#include "core/rom.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace oasis {
namespace {
std::string read_header_string(const std::vector<std::uint8_t>& data, std::size_t offset, std::size_t length) {
    if (data.size() < offset + length) return {};
    std::string s(reinterpret_cast<const char*>(data.data() + offset), length);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
    return s;
}
}

Rom Rom::load(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Unable to open ROM: " + path.string());

    std::vector<std::uint8_t> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};

    if (data.size() < 0x200) throw std::runtime_error("File is too small to be a Mega Drive ROM");
    return Rom(std::move(data));
}

std::string Rom::domestic_title() const {
    return read_header_string(data_, 0x120, 48);
}

std::string Rom::international_title() const {
    return read_header_string(data_, 0x150, 48);
}

} // namespace oasis
