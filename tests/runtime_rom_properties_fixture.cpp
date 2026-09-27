#include "runtime_rom_properties.hpp"

#include <filesystem>
#include <string>

using namespace oasis::thor::runtime_map;

namespace {
Identity identity(std::string run_id, std::string core_hash = std::string(64, 'c')) {
    return {std::string(64, 'a'), 16, std::string(SCHEMA_ID), std::string(CONTRACT_SHA256),
            std::move(core_hash), std::move(run_id), 1, ALL_PROPERTIES, "VALIDATED"};
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::filesystem::path directory(argv[1]);
    std::filesystem::create_directories(directory);
    PropertyMap first(16); first.apply_validated(0, 2, M68K_EXECUTED_ENCODING);
    PropertyMap second(16); second.apply_validated(2, 2, VDP_CRAM_SOURCE);
    PropertyMap incompatible(16); incompatible.apply_validated(4, 1, Z80_DATA_READ);
    save_contribution(directory / "run-a.bin", {identity("run-a"), first.bytes()});
    save_contribution(directory / "run-b.bin", {identity("run-b"), second.bytes()});
    save_contribution(directory / "run-incompatible.bin",
                      {identity("run-bad", std::string(64, 'd')), incompatible.bytes()});
    return 0;
}
