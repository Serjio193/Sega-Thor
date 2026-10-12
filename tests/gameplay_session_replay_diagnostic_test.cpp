#include "tools/hybrid/gameplay_session_replay_diagnostic.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

int main() {
    const auto base = std::filesystem::temp_directory_path() / "oasis-replay-diagnostic-test";
    std::filesystem::create_directories(base);
    const auto expected_path = base / "expected.jsonl";
    const auto observed_path = base / "observed.jsonl";
    const auto receipt_path = base / "receipt.json";
    { std::ofstream out(expected_path); out << "{\"frame\":1}\n{\"frame\":2}\n"; }
    {
        std::ifstream expected(expected_path);
        oasis::hybrid::session::ReplayLandmarkDiagnostic diagnostic(observed_path.string());
        if (!diagnostic.observe(expected, "{\"frame\":1}", 1)) throw std::runtime_error("matching frame rejected");
        if (diagnostic.observe(expected, "{\"frame\":2,\"changed\":true}", 2))
            throw std::runtime_error("changed frame accepted");
        if (!diagnostic.has_divergence()) throw std::runtime_error("divergence not retained");
        diagnostic.write_receipt(receipt_path.string());
    }
    {
        std::ifstream receipt(receipt_path);
        const std::string text((std::istreambuf_iterator<char>(receipt)), {});
        if (text.find("\"first_different_frame\":2") == std::string::npos ||
            text.find("changed") == std::string::npos)
            throw std::runtime_error("machine receipt missed first differing frame");
    }
    std::filesystem::remove_all(base);
}
