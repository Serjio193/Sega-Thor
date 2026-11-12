#include "game/scene_transition_helpers.hpp"

namespace oasis::game {

void assign_default_town_entities(const world::LoadedTownScene& scene, std::vector<NativeEntity>& out) {
    if (scene.entities.empty()) return;
    out = scene.entities;
    NativeEntity slot19{};
    slot19.slot_index = 19U;
    out.push_back(slot19);
}

camera::CameraConfig make_scene_camera_config(camera::CameraController& cam,
                                             int max_cx, int max_cy,
                                             int cam_x, int cam_y,
                                             int vscroll_a, bool wrap8) {
    camera::CameraConfig cfg = cam.config();
    cfg.min_camera_x = 0; cfg.max_camera_x = max_cx;
    cfg.min_camera_y = 0; cfg.max_camera_y = max_cy;
    cfg.initial_camera_x = cam_x; cfg.initial_camera_y = cam_y;
    cfg.initial_vscroll_a = vscroll_a; cfg.vscroll_wrap_8bit = wrap8;
    return cfg;
}

} // namespace oasis::game
