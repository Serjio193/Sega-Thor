#include "game/spirits/spirit_awakening.hpp"

namespace oasis::game::spirits {

std::optional<ShrineEventConfig> dispatch_spirit_awakening(std::uint16_t room_id) noexcept {
    for (const auto& shrine : kShrines) {
        if (shrine.room_id == room_id) {
            return shrine;
        }
    }
    return std::nullopt;
}

} // namespace oasis::game::spirits
