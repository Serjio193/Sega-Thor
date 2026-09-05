#include "tools/re_ant_queue.hpp"

#include <sstream>

namespace oasis::tools {
namespace {

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (const auto character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += 'n'; else if (character == '\r') result += 'r';
        else if (character == '\t') result += 't'; else result += character;
    }
    return result + '"';
}

} // namespace

std::string ant_queue_to_json(const AntQueue& queue) {
    std::ostringstream out;
    out << "{\"schema\":" << quote(queue.schema) << ",\"queue_id\":" << quote(queue.queue_id)
        << ",\"rom_sha256\":" << quote(queue.rom_sha256) << ",\"rom_size\":" << queue.rom_size
        << ",\"backend\":" << quote(queue.backend) << ",\"backend_version\":" << quote(queue.backend_version)
        << ",\"created_from_explorer_hash\":" << quote(queue.created_from_explorer_hash)
        << ",\"generation_policy\":" << quote(queue.generation_policy)
        << ",\"queue_state\":" << quote(queue.queue_state) << ",\"version\":" << queue.version
        << ",\"duplicate_jobs_avoided\":" << queue.duplicate_jobs_avoided << ",\"jobs\":[";
    for (std::size_t index = 0; index < queue.jobs.size(); ++index) {
        if (index) out << ',';
        const auto& item = queue.jobs[index];
        out << "{\"state\":" << quote(ant_queue_lifecycle_name(item.lifecycle))
            << ",\"attempts\":" << item.attempts << ",\"last_result_hash\":" << quote(item.last_result_hash)
            << ",\"failure_reason\":" << quote(item.failure_reason) << ",\"job\":" << ant_job_to_json(item.job) << '}';
    }
    out << "]}";
    return out.str();
}

std::string ant_queue_to_text(const AntQueue& queue) {
    std::ostringstream out;
    out << queue.schema << '\n' << "queue_id=" << queue.queue_id << " rom_sha256=" << queue.rom_sha256
        << " backend=" << queue.backend << " version=" << queue.backend_version << '\n'
        << "created_from_explorer_hash=" << queue.created_from_explorer_hash
        << " generation_policy=" << queue.generation_policy << " queue_state=" << queue.queue_state << '\n'
        << "jobs=" << queue.jobs.size() << " duplicate_jobs_avoided=" << queue.duplicate_jobs_avoided << '\n';
    for (std::size_t index = 0; index < queue.jobs.size(); ++index) {
        const auto& item = queue.jobs[index];
        out << index << " state=" << ant_queue_lifecycle_name(item.lifecycle)
            << " attempts=" << item.attempts << " job_id=" << item.job.job_id
            << " frontier_id=" << item.job.frontier_id << " result_hash=" << item.last_result_hash
            << " failure=" << item.failure_reason << '\n';
    }
    return out.str();
}

} // namespace oasis::tools
