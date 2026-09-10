#include "tools/hybrid/a5_lifetime_observer.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kStart = 0x060182U;
constexpr unsigned kKill = 0x06027EU;

unsigned offset_for(unsigned pc) {
    if (pc == 0x06019EU || pc == 0x0601A6U) return 7U;
    if (pc == 0x06193CU) return 0U;
    if (pc == 0x061946U || pc == 0x061998U) return 4U;
    return 0xFFFFFFFFU;
}

bool is_branch(unsigned pc) { return pc == 0x06018EU || pc == 0x0601A2U; }
unsigned branch_target(unsigned pc) { return pc == 0x06018EU ? 0x0601D4U : 0x0601CEU; }
unsigned branch_fallthrough(unsigned pc) { return pc == 0x06018EU ? 0x060192U : 0x0601A6U; }

bool is_call(unsigned pc) {
    return pc == 0x0601E2U || pc == 0x0601EEU || pc == 0x0601FAU ||
           pc == 0x060206U || pc == 0x060212U || pc == 0x06021EU ||
           pc == 0x060234U || pc == 0x060242U || pc == 0x060250U ||
           pc == 0x060260U || pc == 0x060276U || pc == 0x06027AU;
}

unsigned call_target(unsigned pc) {
    switch (pc) {
    case 0x0601E2U: return 0x062AE0U;
    case 0x0601EEU: case 0x0601FAU: case 0x060206U: case 0x060212U:
    case 0x06021EU: case 0x060260U: return 0x061934U;
    case 0x060234U: case 0x060242U: case 0x060250U: case 0x060276U: return 0x0623ACU;
    case 0x06027AU: return 0x060286U;
    default: return 0U;
    }
}

} // namespace

A5LifetimeObserver::A5LifetimeObserver(CallerAttributionApi api) : api_(api) {
    if (!api_.reg || !api_.peek || !api_.cycles || !api_.refresh_cycles)
        throw std::invalid_argument("A5 lifetime observer requires complete API");
    jsonl_ = "{\"schema\":\"oasis.hybrid.a5-lifetime.v1\",\"generations\":0,\"closed_generations\":0}\n";
}

unsigned A5LifetimeObserver::a5() const { return api_.reg(13U) & 0xFFFFFFU; }

unsigned A5LifetimeObserver::peek_byte(unsigned address) const {
    const auto value = api_.peek(address & 0xFFFFFFU);
    if (value < 0 || value > 0xFF) throw std::runtime_error("A5 lifetime peek failed");
    return static_cast<unsigned>(value);
}

void A5LifetimeObserver::append(const std::string& text) { jsonl_ += text + '\n'; }

void A5LifetimeObserver::execute(unsigned pc, unsigned frame) {
    if (branch_pending_) {
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << branch_pc_
            << ",\"event\":\"branch\",\"outcome\":\""
            << (pc == branch_target_ ? "taken" : pc == branch_fallthrough_ ? "fallthrough" : "unknown")
            << "\"}";
        append(out.str());
        branch_pending_ = false;
    }
    if (!active_) {
        if (pc != kStart) return;
        active_ = true;
        ++generations_;
        current_generation_ = generations_;
        base_a5_ = a5();
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << pc
            << ",\"a5\":" << base_a5_ << ",\"event\":\"start\"}";
        append(out.str());
    }
    if (pc == kKill) {
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << pc
            << ",\"a5_at_kill\":" << a5() << ",\"event\":\"kill\"}";
        append(out.str());
        active_ = false;
        ++closed_;
    }
    if (!active_) return;
    const auto offset = offset_for(pc);
    if (offset != 0xFFFFFFFFU) {
        write_pending_ = pc == 0x0601A6U || pc == 0x061946U;
        write_pc_ = pc;
        write_address_ = (a5() + offset) & 0xFFFFFFU;
        write_old_ = peek_byte(write_address_);
    }
    if (is_branch(pc)) {
        branch_pending_ = true;
        branch_pc_ = pc;
        branch_target_ = branch_target(pc);
        branch_fallthrough_ = branch_fallthrough(pc);
    }
    if (is_call(pc)) {
        call_pending_ = true;
        call_site_ = pc;
        call_target_ = call_target(pc);
        call_return_ = pc + 4U;
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << pc
            << ",\"event\":\"call_entry\",\"target\":" << call_target_ << "}";
        append(out.str());
    } else if (call_pending_ && pc == call_target_) {
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << pc
            << ",\"event\":\"callee_entry\",\"call_site\":" << call_site_ << "}";
        append(out.str());
        call_pending_ = false;
    } else if (pc == call_return_) {
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << pc
            << ",\"event\":\"call_return\"}";
        append(out.str());
        call_pending_ = false;
    }
    previous_pc_ = pc;
}

void A5LifetimeObserver::memory(int type, int width, unsigned address, unsigned value, unsigned frame) {
    if (type >= 0 && type < static_cast<int>(memory_type_counts_.size())) ++memory_type_counts_[type];
    if (!active_) return;
    address &= 0xFFFFFFU;
    if (write_pending_ && type == 4 && address == write_address_) {
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << write_pc_
            << ",\"a5\":" << a5() << ",\"effective_address\":" << address
            << ",\"offset\":" << offset_for(write_pc_)
            << ",\"width\":" << width << ",\"direction\":\"write\",\"old\":"
            << write_old_ << ",\"new\":" << value << "}";
        append(out.str());
        write_pending_ = false;
    } else if (type == 2 && offset_for(previous_pc_) != 0xFFFFFFFFU &&
               address == ((base_a5_ + offset_for(previous_pc_)) & 0xFFFFFFU)) {
        std::ostringstream out;
        out << "{\"generation_id\":" << current_generation_ << ",\"frame\":" << frame
            << ",\"cycles\":" << api_.cycles() << ",\"pc\":" << previous_pc_
            << ",\"a5\":" << a5() << ",\"effective_address\":" << address
            << ",\"offset\":" << offset_for(previous_pc_)
            << ",\"width\":" << width << ",\"direction\":\"read\",\"old\":"
            << value << ",\"new\":" << value << "}";
        append(out.str());
    }
}

void A5LifetimeObserver::event(int type, int width, unsigned address, unsigned value,
                               unsigned frame, unsigned previous_execute_pc) {
    previous_pc_ = previous_execute_pc & 0xFFFFFFU;
    if (type == 1) execute(address & 0xFFFFFFU, frame);
    else memory(type, width, address, value, frame);
}

void A5LifetimeObserver::finish(const std::filesystem::path& output) {
    std::ostringstream header;
    header << "{\"schema\":\"oasis.hybrid.a5-lifetime.v1\",\"generations\":"
           << generations_ << ",\"closed_generations\":" << closed_ << ",\"memory_type_counts\":[";
    for (std::size_t index = 0; index < memory_type_counts_.size(); ++index) {
        if (index) header << ',';
        header << memory_type_counts_[index];
    }
    header << "]}";
    const auto events = jsonl_.find('\n');
    jsonl_ = header.str() + '\n' + (events == std::string::npos ? std::string{} : jsonl_.substr(events + 1));
    if (output.empty()) return;
    std::ofstream file(output);
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file << jsonl_;
}

} // namespace oasis::hybrid
