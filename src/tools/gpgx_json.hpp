#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::tools {

struct JsonValue {
    enum class Kind { null_value, boolean, number, string, array, object };
    Kind kind = Kind::null_value;
    std::string scalar;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue> object;

    [[nodiscard]] const JsonValue* find(std::string_view key) const;
    [[nodiscard]] std::string string_value(std::string_view context) const;
    [[nodiscard]] std::uint64_t integer_value(std::string_view context) const;
};

[[nodiscard]] JsonValue parse_json(std::string_view text);

} // namespace oasis::tools
