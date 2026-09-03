#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace oasis {

class Rom {
public:
    static Rom load(const std::filesystem::path& path);

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept { return data_; }
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    [[nodiscard]] std::string domestic_title() const;
    [[nodiscard]] std::string international_title() const;

private:
    explicit Rom(std::vector<std::uint8_t> data) : data_(std::move(data)) {}
    std::vector<std::uint8_t> data_;
};

} // namespace oasis
