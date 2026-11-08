#include "genesis/vdp_state.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace oasis::genesis {

bool VdpState::is_display_enabled() const noexcept {
    return (registers[1] & 0x40U) != 0U;
}

bool VdpState::is_h40() const noexcept {
    return (registers[12] & 0x01U) != 0U;
}

bool VdpState::is_v30() const noexcept {
    return (registers[1] & 0x08U) != 0U;
}

int VdpState::display_width() const noexcept {
    return is_h40() ? 320 : 256;
}

int VdpState::display_height() const noexcept {
    return is_v30() ? 240 : 224;
}

std::uint32_t VdpState::plane_a_base() const noexcept {
    const std::uint32_t mask = is_h40() ? 0x38U : 0x3EU;
    return static_cast<std::uint32_t>(registers[2] & mask) << 10U;
}

std::uint32_t VdpState::plane_b_base() const noexcept {
    return static_cast<std::uint32_t>(registers[4] & 0x07U) << 13U;
}

std::uint32_t VdpState::window_base() const noexcept {
    const std::uint32_t mask = is_h40() ? 0x3CU : 0x3EU;
    return static_cast<std::uint32_t>(registers[3] & mask) << 10U;
}

std::uint32_t VdpState::sat_base() const noexcept {
    const std::uint32_t mask = is_h40() ? 0x7FU : 0x7EU;
    return static_cast<std::uint32_t>(registers[5] & mask) << 9U;
}

std::uint32_t VdpState::hscroll_base() const noexcept {
    return static_cast<std::uint32_t>(registers[13] & 0x3FU) << 10U;
}

int VdpState::plane_width_cells() const noexcept {
    const unsigned code = registers[16] & 0x03U;
    if (code == 0U) return 32;
    if (code == 1U) return 64;
    return 128;
}

int VdpState::plane_height_cells() const noexcept {
    const unsigned code = (registers[16] >> 4U) & 0x03U;
    if (code == 0U) return 32;
    if (code == 1U) return 64;
    return 128;
}

std::uint8_t VdpState::backdrop_cram_index() const noexcept {
    return registers[7] & 0x3FU;
}

int VdpState::hscroll_mode() const noexcept {
    return registers[11] & 0x03U;
}

int VdpState::vscroll_mode() const noexcept {
    return (registers[11] >> 2U) & 0x01U;
}

namespace {

void skip_ws(std::string_view sv, std::size_t& pos) {
    while (pos < sv.size() && std::isspace(static_cast<unsigned char>(sv[pos]))) {
        ++pos;
    }
}

bool parse_uint(std::string_view sv, std::size_t& pos, unsigned long& out) {
    skip_ws(sv, pos);
    if (pos >= sv.size() || !std::isdigit(static_cast<unsigned char>(sv[pos]))) {
        return false;
    }
    out = 0;
    while (pos < sv.size() && std::isdigit(static_cast<unsigned char>(sv[pos]))) {
        out = out * 10U + static_cast<unsigned long>(sv[pos] - '0');
        ++pos;
    }
    return true;
}

bool find_key(std::string_view sv, std::string_view key, std::size_t& pos) {
    const std::string needle = "\"" + std::string(key) + "\"";
    const std::size_t found = sv.find(needle, pos);
    if (found == std::string_view::npos) return false;
    pos = found + needle.size();
    skip_ws(sv, pos);
    if (pos < sv.size() && sv[pos] == ':') {
        ++pos;
        skip_ws(sv, pos);
        return true;
    }
    return false;
}

template <std::size_t N>
bool parse_byte_array(std::string_view sv, std::string_view key, std::array<std::uint8_t, N>& arr) {
    std::size_t pos = 0;
    if (!find_key(sv, key, pos)) return false;
    if (pos >= sv.size() || sv[pos] != '[') return false;
    ++pos;

    for (std::size_t i = 0; i < N; ++i) {
        unsigned long val = 0;
        if (!parse_uint(sv, pos, val)) return false;
        arr[i] = static_cast<std::uint8_t>(val & 0xFFU);
        skip_ws(sv, pos);
        if (pos < sv.size() && sv[pos] == ',') {
            ++pos;
        }
    }
    return true;
}

} // namespace

bool VdpState::load_from_json(std::string_view json_text) {
    if (!parse_byte_array(json_text, "vram", vram)) return false;
    if (!parse_byte_array(json_text, "cram", cram)) return false;
    if (!parse_byte_array(json_text, "vsram", vsram)) return false;

    std::size_t pos = 0;
    if (find_key(json_text, "registers", pos)) {
        if (pos < json_text.size() && json_text[pos] == '{') {
            ++pos;
            while (pos < json_text.size()) {
                skip_ws(json_text, pos);
                if (pos < json_text.size() && json_text[pos] == '}') break;
                if (pos >= json_text.size() || json_text[pos] != '"') break;
                ++pos;
                unsigned long reg_idx = 0;
                if (!parse_uint(json_text, pos, reg_idx)) break;
                if (pos < json_text.size() && json_text[pos] == '"') ++pos;
                skip_ws(json_text, pos);
                if (pos < json_text.size() && json_text[pos] == ':') ++pos;
                unsigned long reg_val = 0;
                if (!parse_uint(json_text, pos, reg_val)) break;
                if (reg_idx < registers.size()) {
                    registers[reg_idx] = static_cast<std::uint8_t>(reg_val & 0xFFU);
                }
                skip_ws(json_text, pos);
                if (pos < json_text.size() && json_text[pos] == ',') ++pos;
            }
        }
    }
    return true;
}

bool VdpState::load_from_file(const char* filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_json(buffer.str());
}

} // namespace oasis::genesis
