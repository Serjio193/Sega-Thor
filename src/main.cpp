#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/controlled_screen.hpp"
#include "game/resource_diagnostic.hpp"
#include "game/resource_loader.hpp"
#include "game/render/framebuffer.hpp"
#include "genesis/vdp.hpp"
#include "platform/window.hpp"

#include <chrono>
#include <algorithm>
#include <charconv>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace {

int run_resource_diagnostic(const oasis::Rom& rom, std::uint16_t resource_id) {
    const auto resource = oasis::game::load_graphics_resource(rom.bytes(), resource_id);
    std::cout << "ResourceID: " << resource.resource_id << "\n"
              << "Backend: " << resource.backend_status << "\n"
              << "Migration status: " << resource.migration_status << "\n"
              << "Override mode: " << resource.override_mode << "\n"
              << "Compressed SHA-256: " << resource.compressed_sha256 << "\n"
              << "Decoded SHA-256: " << resource.decoded_sha256 << "\n"
              << "ROM payload bytes passed to decoder: "
              << resource.rom_payload_bytes_passed_to_decoder << "\n";
    if (!resource.override_rejection_reason.empty()) {
        std::cout << "Override fallback reason: " << resource.override_rejection_reason << "\n";
    }
    oasis::genesis::Vdp vdp;
    oasis::game::render::ResourceDiagnosticScreen screen;
    screen.transfer_to_vram(vdp, resource.bytes);
    oasis::game::render::SoftwareFramebuffer framebuffer;
    oasis::platform::NativeWindow window(framebuffer.kWidth, framebuffer.kHeight);
    if (!window.backend_available() ||
        !window.open("Graphics resource ID " + std::to_string(resource_id) + " diagnostic")) {
        std::cerr << "error: native window backend is unavailable on this platform\n";
        return 4;
    }

    while (window.is_open()) {
        if (!window.process_events()) break;
        screen.render(vdp, framebuffer.pixels());
        window.present(framebuffer.pixels());
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    std::uint16_t resource_id{};
    const auto id_argument = argc == 4 ? std::string_view(argv[3]) : std::string_view("0");
    const auto parsed_id = std::from_chars(id_argument.data(),
                                           id_argument.data() + id_argument.size(),
                                           resource_id);
    const bool resource_mode = argc == 4 && std::string_view(argv[2]) == "--resource-id" &&
                               !id_argument.empty() && parsed_id.ec == std::errc{} &&
                               parsed_id.ptr == id_argument.data() + id_argument.size() &&
                               resource_id >= 1U &&
                               resource_id <= oasis::game::kGfx3820ResourceCount;
    if (argc != 2 && !resource_mode) {
        std::cerr << "usage: oasis <rom.bin> [--resource-id 1..107]\n";
        return 1;
    }

    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());

        std::cout << "Identity: " << identity.display_name << "\n";
        std::cout << "Status: " << oasis::to_string(identity.status) << "\n";
        std::cout << "Size: " << identity.fingerprint.size << " bytes\n";
        std::cout << "Console: " << identity.header.console_name << "\n";
        std::cout << "Domestic title: " << identity.header.domestic_title << "\n";
        std::cout << "International title: " << identity.header.international_title << "\n";
        std::cout << "Product code: " << identity.header.product_code << "\n";
        std::cout << "Region: " << identity.header.region << "\n";
        std::cout << "Header signature: " << (identity.header.header_signature_valid ? "valid" : "invalid") << "\n";
        std::cout << "Sega checksum: " << (identity.fingerprint.sega_checksum_valid ? "valid" : "invalid") << "\n";
        std::cout << "CRC32: " << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
                  << identity.fingerprint.crc32 << "\n";
        std::cout << "SHA-1: " << identity.fingerprint.sha1 << "\n";
        std::cout << "SHA-256: " << identity.fingerprint.sha256 << "\n";
        if (identity.status != oasis::RomSupportStatus::Supported) {
            std::cerr << "error: controlled native screen requires the canonical supported ROM\n";
            return 3;
        }
        if (resource_mode) return run_resource_diagnostic(rom, resource_id);

        oasis::game::screen::ControlledScreen screen(rom.bytes());
        oasis::game::render::SoftwareFramebuffer framebuffer;
        oasis::platform::NativeWindow window(framebuffer.kWidth, framebuffer.kHeight);
        if (!window.backend_available() || !window.open("Beyond Oasis - Native Controlled Screen")) {
            std::cerr << "error: native window backend is unavailable on this platform\n";
            return 4;
        }

        oasis::core::RuntimeLoop runtime(screen);
        constexpr auto logical_step = std::chrono::milliseconds(1000 / 60);
        auto next_tick = std::chrono::steady_clock::now();
        while (window.is_open()) {
            if (!window.process_events()) break;
            const auto now = std::chrono::steady_clock::now();
            if (now < next_tick) {
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    next_tick - now);
                std::this_thread::sleep_for(std::min(logical_step, remaining));
                continue;
            }
            runtime.step(oasis::core::InputSnapshot{window.poll_controller(), {}});
            screen.render(framebuffer.pixels());
            window.present(framebuffer.pixels());
            next_tick += logical_step;
            if (now - next_tick > std::chrono::milliseconds(250)) next_tick = now;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 2;
    }
}
