#pragma once

#include "game/camera/camera_controller.hpp"
#include "game/entity/native_entity.hpp"
#include "game/world/town_scene_loader.hpp"

#include <vector>

namespace oasis::game {

void assign_default_town_entities(const world::LoadedTownScene& scene, std::vector<NativeEntity>& out);

camera::CameraConfig make_scene_camera_config(camera::CameraController& cam,
                                             int max_cx, int max_cy,
                                             int cam_x, int cam_y,
                                             int vscroll_a, bool wrap8);

} // namespace oasis::game
