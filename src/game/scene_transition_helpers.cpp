#include "game/scene_transition_helpers.hpp"

namespace oasis::game {

void assign_default_town_entities(const world::LoadedTownScene& scene, std::vector<NativeEntity>& out) {
    if (scene.entities.empty()) return;
    out = scene.entities;
    NativeEntity slot19{};
    slot19.slot_index = 19U;
    out.push_back(slot19);
}

void assign_scene_entities(world::ScreenId id, const world::LoadedTownScene& scene,
                           bool chest_opened, std::vector<NativeEntity>& out) {
    if (id.group == 1U && id.index == 3U) {
        assign_default_town_entities(scene, out);
    } else if (id.group == 1U && id.index == 2U) {
        out.resize(2U);
        out[0] = NativeEntity{};
        out[0].type_id = 0x0060U;
        out[0].active = true;
        out[0].world_x = 256;
        out[0].world_y = 88;
        out[0].tile_base = 0x04A8U;
        if (chest_opened) out[0].state = 0x6AU;
        out[1] = NativeEntity{};
        out[1].type_id = 0x0098U;
        out[1].active = true;
        out[1].world_x = 259;
        out[1].world_y = 101;
        out[1].tile_base = 0x069DU;
        out[1].hp = 4;
    } else if (id.group == 1U && id.index == 8U) {
        out.resize(1U);
        out[0] = NativeEntity{};
        out[0].slot_index = 4U;
        out[0].type_id = 0x003CU; // Village Elder NPC
        out[0].world_x = 160;
        out[0].world_y = 160;
        out[0].tile_base = 0x0490U;
        out[0].hp = 100;
        out[0].active = true;
        out[0].message_id = 5U; // Village Elder message ID
    } else if (id.group == 1U && id.index == 0U) {
        // Area 1 Room 0 (South Dock / Pier) - Authentic entity 0x003C (Pier Villager)
        // Spawned by opcode 0x15 in room script at 0x046356, pos (160, 216), tile 0x0490, message ID 4
        out.resize(1U);
        out[0] = NativeEntity{};
        out[0].slot_index = 4U;
        out[0].type_id = 0x003CU;
        out[0].world_x = 160;
        out[0].world_y = 216;
        out[0].tile_base = 0x0490U;
        out[0].hp = 100;
        out[0].active = true;
        out[0].message_id = 4U; // Pier Villager: "Oh, Ali.  Nice armlet.  Where'd you find it?"
    } else if (id.group == 13U && id.index == 0U) {
        // Area 0x0D Room 0 (Spirit Sanctuary Entrance)
        // Spawn script 0x04382E executes opcode 0x00 (no persistent entities at entrance)
        out.clear();
    } else {
        out.clear();
    }
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
