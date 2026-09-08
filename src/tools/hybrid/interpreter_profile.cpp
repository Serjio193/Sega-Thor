#include "tools/hybrid/interpreter_profile.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace oasis::hybrid {
namespace {

struct Entry {
    unsigned pc{};
    unsigned count{};
};

} // namespace

void write_interpreter_profile(const std::filesystem::path& path,
                               std::string_view mode,
                               const std::map<unsigned, unsigned>& pcs,
                               unsigned total_executions) {
    std::vector<Entry> ranked;
    ranked.reserve(pcs.size());
    for (const auto& [pc, count] : pcs) ranked.push_back({pc, count});
    std::sort(ranked.begin(), ranked.end(), [](const Entry& left, const Entry& right) {
        return left.count != right.count ? left.count > right.count : left.pc < right.pc;
    });

    std::ofstream output(path);
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output << "{\n\"schema\":\"oasis.hybrid.interpreter-profile.v1\",\n"
           << "\"mode\":\"" << mode << "\",\n"
           << "\"total_executions\":" << total_executions
           << ",\n\"unique_pcs\":" << ranked.size() << ",\n\"pcs\":[";
    for (std::size_t i = 0; i < ranked.size(); ++i) {
        if (i) output << ',';
        output << "{\"pc\":\"0x" << std::hex << ranked[i].pc << std::dec
               << "\",\"count\":" << ranked[i].count << '}';
    }
    output << "]\n}\n";
}

} // namespace oasis::hybrid
