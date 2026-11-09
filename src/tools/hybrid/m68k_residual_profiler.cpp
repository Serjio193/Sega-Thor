#include "tools/hybrid/m68k_residual_profiler.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace oasis::hybrid {
namespace {

std::string hex_str(std::uint32_t val, unsigned width = 6) {
    std::ostringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(static_cast<int>(width)) << val;
    return ss.str();
}

std::string calculate_file_sha256(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::vector<std::uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
    // Simple SHA256 using standard C++ or sha256 helper
    // We can run external or use a small inline digest
    return ""; // filled by python or post-processor if empty
}

void atexit_handler() {
    M68kResidualProfiler::instance().finish();
}

} // namespace

M68kResidualProfiler& M68kResidualProfiler::instance() noexcept {
    static M68kResidualProfiler inst;
    return inst;
}

bool M68kResidualProfiler::is_idle_pc(unsigned pc) noexcept {
    return (pc >= 0x008ACAU && pc <= 0x008AD0U) || (pc >= 0x008B72U && pc <= 0x008B7AU);
}

bool M68kResidualProfiler::is_experimental_native_pc(unsigned pc) noexcept {
    if (pc >= 0x02575CU && pc <= 0x0259DEU) return true; // Slot 222
    if (pc >= 0x00AD76U && pc <= 0x00B020U) return true; // Slot 223
    if (pc >= 0x00A9A0U && pc <= 0x00AD56U) return true; // Slot 224
    if (pc >= 0x002FDAU && pc <= 0x00302EU) return true; // Slot 226
    if (pc >= 0x002512U && pc <= 0x002632U) return true; // Slot 227
    if (pc >= 0x0023CCU && pc <= 0x002512U) return true; // Slot 228
    if (pc >= 0x00D168U && pc <= 0x00D264U) return true; // Slot 229
    if (pc >= 0x00D266U && pc <= 0x00D2A0U) return true; // Slot 230
    if (pc >= 0x006E7AU && pc <= 0x006E86U) return true; // Slot 234
    if (pc >= 0x00D2B4U && pc <= 0x00D344U) return true; // Slot 237
    if (pc >= 0x0059A8U && pc <= 0x0059BCU) return true; // Slot 238
    return false;
}

void M68kResidualProfiler::init(const std::filesystem::path& output_dir, const std::string& scenario) {
    if (enabled_) return;
    output_dir_ = output_dir;
    scenario_ = scenario;
    enabled_ = true;
    finished_ = false;
    summary_ = ResidualProfileSummary{};
    records_.clear();
    std::atexit(atexit_handler);
}

void M68kResidualProfiler::observe(unsigned pc, unsigned opcode, unsigned frame, unsigned previous_pc,
                                   RegisterRead read_register, SchedulerRead, bool native_step) {
    if (!enabled_ || finished_) return;
    pc &= 0xFFFFFFU;
    if ((pc & 1U) != 0) return;

    ++summary_.total_guest_steps;

    if (native_step) {
        if (is_experimental_native_pc(pc)) {
            ++summary_.experimental_native_steps;
        } else {
            ++summary_.production_native_steps;
        }
    } else {
        if (is_idle_pc(pc)) {
            ++summary_.idle_interpreter_steps;
        } else {
            ++summary_.non_idle_interpreter_steps;
            auto& rec = records_[pc];
            rec.pc = pc;
            rec.opcode = static_cast<std::uint16_t>(opcode);
            ++rec.count;
            if (rec.first_frame == 0) rec.first_frame = frame;
            rec.last_frame = frame;
            if (previous_pc != 0) {
                ++rec.callers[previous_pc & 0xFFFFFFU];
            }
            if (read_register) {
                const unsigned sr = read_register(17U);
                const unsigned irq = (sr >> 8U) & 7U;
                ++rec.irq_masks[irq];
            }
        }
    }
}

void M68kResidualProfiler::finish() {
    if (!enabled_ || finished_) return;
    finished_ = true;

    summary_.unique_interpreted_pcs = records_.size();

    if (!output_dir_.empty()) {
        std::filesystem::create_directories(output_dir_);

        // Write detailed residual interpreter profile
        const auto profile_path = output_dir_ / "scenario_c_residual_interpreter_profile.json";
        std::ofstream out(profile_path, std::ios::trunc);
        out << "{\n"
            << "  \"schema\": \"oasis.residual-interpreter-profile.v1\",\n"
            << "  \"scenario\": \"" << scenario_ << "\",\n"
            << "  \"total_guest_steps\": " << summary_.total_guest_steps << ",\n"
            << "  \"idle_interpreter_steps\": " << summary_.idle_interpreter_steps << ",\n"
            << "  \"production_native_steps\": " << summary_.production_native_steps << ",\n"
            << "  \"experimental_native_steps\": " << summary_.experimental_native_steps << ",\n"
            << "  \"non_idle_interpreter_steps\": " << summary_.non_idle_interpreter_steps << ",\n"
            << "  \"unique_interpreted_pcs\": " << summary_.unique_interpreted_pcs << ",\n"
            << "  \"instructions\": [\n";

        std::vector<std::uint32_t> sorted_pcs;
        sorted_pcs.reserve(records_.size());
        for (const auto& [pc, _] : records_) sorted_pcs.push_back(pc);
        std::sort(sorted_pcs.begin(), sorted_pcs.end(), [this](std::uint32_t a, std::uint32_t b) {
            if (records_[a].count != records_[b].count) return records_[a].count > records_[b].count;
            return a < b;
        });

        for (std::size_t i = 0; i < sorted_pcs.size(); ++i) {
            const auto& rec = records_[sorted_pcs[i]];
            out << "    {\n"
                << "      \"pc\": \"" << hex_str(rec.pc) << "\",\n"
                << "      \"opcode\": \"" << hex_str(rec.opcode, 4) << "\",\n"
                << "      \"execution_count\": " << rec.count << ",\n"
                << "      \"first_frame\": " << rec.first_frame << ",\n"
                << "      \"last_frame\": " << rec.last_frame << ",\n"
                << "      \"distinct_callers\": " << rec.callers.size() << ",\n"
                << "      \"irq_masks\": [";
            for (std::size_t j = 0; j < 8; ++j) {
                out << (j ? ", " : "") << rec.irq_masks[j];
            }
            out << "]\n    }" << (i + 1 < sorted_pcs.size() ? "," : "") << "\n";
        }
        out << "  ]\n}\n";
        out.close();

        // Write disjoint profile summary
        const auto summary_path = output_dir_ / "scenario_c_disjoint_profile_summary.json";
        std::ofstream sum_out(summary_path, std::ios::trunc);
        sum_out << "{\n"
                << "  \"schema\": \"oasis.disjoint-profile-summary.v1\",\n"
                << "  \"scenario\": \"" << scenario_ << "\",\n"
                << "  \"disjoint_categories\": {\n"
                << "    \"TOTAL_GUEST_STEPS\": " << summary_.total_guest_steps << ",\n"
                << "    \"IDLE_INTERPRETER\": " << summary_.idle_interpreter_steps << ",\n"
                << "    \"PRODUCTION_NATIVE\": " << summary_.production_native_steps << ",\n"
                << "    \"EXPERIMENTAL_NATIVE\": " << summary_.experimental_native_steps << ",\n"
                << "    \"NON_IDLE_INTERPRETER\": " << summary_.non_idle_interpreter_steps << "\n"
                << "  },\n"
                << "  \"unique_interpreted_pcs\": " << summary_.unique_interpreted_pcs << ",\n"
                << "  \"accounting_check\": "
                << ((summary_.idle_interpreter_steps + summary_.production_native_steps +
                     summary_.experimental_native_steps + summary_.non_idle_interpreter_steps ==
                     summary_.total_guest_steps) ? "\"PASS\"" : "\"FAIL\"") << "\n"
                << "}\n";
        sum_out.close();
    }
}

} // namespace oasis::hybrid
