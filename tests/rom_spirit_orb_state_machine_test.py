#!/usr/bin/env python3
"""
Unit tests for ROM Spirit Orb State 0 Scan, Catalyst Transmutation,
Trail Particle Emitter & Tilemap Sampler (0x01171C..0x011A4C).
Milestone M15.484.
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_spirit_orb_state_machine_m15_484.json"
)

SPIRIT_ORB_TRAIL_TILE_TABLE = [16, 8, 16, 9, 16, 10, 16, 11, 12, 13, 12, 11, 14, 15, 14, 17, 18, 19]

ITEM_DIRECT_CATALYSTS = {0: 0x12, 1: 0x13, 2: 0x14, 3: 0x15}
ITEM_TRANSMUTATIONS = {
    0x1A: (0, 0x21, None),
    0x1B: (1, 0x05, 0x14),
    0x1C: (2, 0x11, None),
    0x1D: (3, 0x2A, None),
    0x27: (3, 0x26, None),
}


def s8(val):
    val &= 0xFF
    return val - 256 if val >= 128 else val


def u8(val):
    return val & 0xFF


def asr_w_3(val):
    val &= 0xFFFF
    if val >= 0x8000:
        val -= 0x10000
    return val >> 3


def sample_low_nibble_attribute(tile_bytes, stride_shift, x, y):
    row = asr_w_3(y) << stride_shift
    col = asr_w_3(x)
    idx = (row + col) & 0xFFFF
    if idx >= 0x8000:
        idx -= 0x10000
    return tile_bytes[idx] & 0x0F


def emit_trail_particle_vertex(slot_val, facing):
    lat = 1
    age = slot_val & 0xFF
    if age & 0x80:
        age &= 0x7F
        lat = -2
    if age >= 18:
        return None, 0xFF
    tile_id = SPIRIT_ORB_TRAIL_TILE_TABLE[age]
    d1 = u8(age * 3 + 12)
    d2 = u8(lat)
    if facing == 0:
        d1, d2 = d2, d1
    elif facing == 1:
        d1 = u8(-d1)
    elif facing == 3:
        pass
    else:
        d1, d2 = d2, d1
        d2 = u8(-d2)
    return (tile_id, s8(d1), s8(u8(d2 - 12))), slot_val


def commit_state_transition(orb, target_state, sfx_log):
    orb["vel_x"] = 0
    orb["vel_y"] = 0
    orb["state_04"] = target_state
    orb["field_2a"] = target_state * 2
    orb["z_10_word"] = (orb["z_10_word"] + 12) & 0xFFFF
    orb["z_10_fixed"] = (orb["z_10_word"] << 16) | (orb.get("z_10_fixed", 0) & 0xFFFF)
    orb["timer_02"] = 0
    orb["flags_36"] |= 0x08
    sfx_log.append(0x008004D2)
    return "STATE_TRANSITION"


def check_unlock_and_transition(orb, ram, sfx_log):
    spirit_id = ram["ff19a6"] & 0xFF
    unlocked = bool((ram["ff0d7a"] >> spirit_id) & 1)
    inhibited = bool(ram["ff16f1"] & 0x01)
    next_state = 1 if (unlocked and not inhibited) else 2
    return commit_state_transition(orb, next_state, sfx_log)


def aabb_overlaps_2d(orb, target, rx, ry):
    oy = orb["y"] + orb["z_14"]
    ty = target["y"] + target["z_14"]
    return (
        orb["x"] - rx <= target["x"] + target["half_w"]
        and orb["x"] + rx >= target["x"] - target["half_w"]
        and oy - ry <= ty + target["half_h"]
        and oy + ry >= ty - target["half_h"]
    )


def run_item_catalyst_scan(orb, ram, fx_pool, sfx_log):
    hit = None
    for slot in fx_pool:
        if slot.get("type_00", 0) > 0 and aabb_overlaps_2d(orb, slot, 4, 4):
            if slot["z_lo"] <= orb["z_total"] <= slot["z_hi"]:
                hit = slot
                break
    if hit is None or hit["type_00"] != 0x0008:
        return "INTEGRATE_AND_TRAIL"
    if ram["ff16f1"] & 0x01:
        return commit_state_transition(orb, 2, sfx_log)

    subtype = hit["subtype_32"] & 0xFF
    for spirit_id, cat_subtype in ITEM_DIRECT_CATALYSTS.items():
        if ((ram["ff0d7a"] >> spirit_id) & 1) and subtype == cat_subtype:
            ram["ff19a6"] = spirit_id
            return check_unlock_and_transition(orb, ram, sfx_log)

    if subtype in ITEM_TRANSMUTATIONS:
        spirit_id, new_subtype, extra_52 = ITEM_TRANSMUTATIONS[subtype]
        if (ram["ff0d7a"] >> spirit_id) & 1:
            if extra_52 is not None:
                hit["field_52"] = extra_52
            hit["subtype_32"] = new_subtype
            hit["timer_22"] = 0x1E
            hit["flags_36"] &= ~0x02
            ram["ff19a6"] = spirit_id
            return check_unlock_and_transition(orb, ram, sfx_log)

    return commit_state_transition(orb, 2, sfx_log)


def run_state0_step(orb, ram, entities, fx_pool, sfx_log):
    if ram["ff1990"] != 0:
        ram["ff1990"] = 0
        return check_unlock_and_transition(orb, ram, sfx_log)

    probe_medium = True
    if orb["z_10_fixed"] != 0:
        decayed = max(0, orb["z_10_fixed"] - 0x18000)
        orb["z_10_fixed"] = decayed
        orb["z_10_word"] = (decayed >> 16) & 0xFFFF
        contact = (orb["flags_40"] | orb["flags_41"]) & 0xFF
        if (contact & 0x40) == 0:
            probe_medium = False
        elif contact & 0x08:
            orb["type_00"] = 0
            return "DESPAWNED"

    if probe_medium:
        if orb["flags_39"] & 0x02:
            ram["ff19a6"] = 0
            return check_unlock_and_transition(orb, ram, sfx_log)
        if ram["ff17c2"] <= 3 and (orb["flags_6f"] & 0x20):
            ram["ff19a6"] = ram["ff17c2"]
            return check_unlock_and_transition(orb, ram, sfx_log)
        if ram["ff17c3"] <= 3 and (orb["flags_6f"] & 0x40):
            ram["ff19a6"] = ram["ff17c3"]
            return check_unlock_and_transition(orb, ram, sfx_log)
        return commit_state_transition(orb, 2, sfx_log)

    hit = None
    for idx, ent in enumerate(entities):
        if idx >= 2 and ent.get("type_00", 0) > 0 and aabb_overlaps_2d(orb, ent, 12, 10):
            if orb["z_total"] <= ent["z_hi"] and orb["z_total"] + 24 >= ent["z_lo"]:
                hit = ent
                break
    if hit is None:
        return run_item_catalyst_scan(orb, ram, fx_pool, sfx_log)

    if hit["type_00"] == 0x0064:
        hit["state_02"] = 1
    if hit["type_00"] == 0x00B4:
        hit["field_a6"] = 0xFF
    if hit["flags_39"] & 0x08:
        ram["ff19a6"] = 1
        return check_unlock_and_transition(orb, ram, sfx_log)
    for bit in range(4):
        if (hit["flags_3c"] >> bit) & 1:
            ram["ff19a6"] = bit
            return check_unlock_and_transition(orb, ram, sfx_log)
    return commit_state_transition(orb, 2, sfx_log)


def run_integrate_and_update_trail(orb, cam_x, cam_y):
    orb["pos_x_72"] += orb["vel_x"]
    orb["pos_y_76"] += orb["vel_y"]
    screen_y = orb["y"] - orb["z_10_word"]
    if (
        orb["x"] < cam_x - 0x10
        or screen_y < cam_y - 0x10
        or orb["x"] > cam_x + 0x140 + 0x10
        or screen_y > cam_y + 0xE0 + 0x10
    ):
        orb["type_00"] = 0
        return None

    orb["trail_timer_ac"] = s8(orb["trail_timer_ac"] - 1)
    if orb["trail_timer_ac"] < 0:
        for i in range(6):
            if orb["trail_slots"][i] == 0:
                orb["trail_slots"][i] = 1
                orb["trail_timer_ac"] = 2
                orb["trail_sign_ad"] ^= 1
                if orb["trail_sign_ad"] != 0:
                    orb["trail_slots"][i] |= 0x80
                break

    vertices, count = [], 0
    for i in range(6):
        val = orb["trail_slots"][i]
        if val != 0:
            vert, new_val = emit_trail_particle_vertex(val, orb["facing_66"])
            orb["trail_slots"][i] = new_val
            if vert is not None:
                vertices.append(vert)
                count += 1
            orb["trail_slots"][i] = u8(orb["trail_slots"][i] + 1)

    if count == 0:
        orb["anim_06"] = 0
        return {"header": None, "vertices": []}
    orb["anim_06"] = 0x16
    orb["vbuf_ptr_62"] = 0x00FF19A8
    return {"header": 0x80 | ((count - 1) & 0xFF), "vertices": vertices}


class TestROMSpiritOrbStateMachine(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            self.evidence = json.load(f)

    def _make_orb(self, **overrides):
        orb = {
            "type_00": 0x0030, "timer_02": 5, "state_04": 0, "anim_06": 0,
            "x": 160, "y": 120, "z_10_fixed": 0, "z_10_word": 0, "z_14": 0, "z_total": 0,
            "field_2a": 0, "flags_36": 0, "flags_39": 0, "flags_40": 0, "flags_41": 0,
            "vel_x": 0x10000, "vel_y": -0x08000, "vbuf_ptr_62": 0, "facing_66": 0, "flags_6f": 0,
            "pos_x_72": 0, "pos_y_76": 0, "trail_slots": [0] * 6, "trail_timer_ac": 0, "trail_sign_ad": 0,
        }
        orb.update(overrides)
        return orb

    def _make_ram(self, **overrides):
        ram = {"ff0d7a": 0x0F, "ff16f1": 0x00, "ff17c2": 0xFF, "ff17c3": 0xFF, "ff1990": 0, "ff19a6": 0}
        ram.update(overrides)
        return ram

    def _make_entity(self, **overrides):
        ent = {
            "type_00": 0x0064, "x": 160, "y": 120, "z_14": 0, "half_w": 8, "half_h": 8,
            "z_lo": 0, "z_hi": 20, "flags_39": 0, "flags_3c": 0, "state_02": 0, "field_a6": 0,
        }
        ent.update(overrides)
        return ent

    def _make_item(self, subtype, type_00=0x0008):
        return {
            "type_00": type_00, "x": 160, "y": 120, "z_14": 0, "half_w": 4, "half_h": 4,
            "z_lo": 0, "z_hi": 10, "subtype_32": subtype, "timer_22": 0, "flags_36": 0x0F, "field_52": 0,
        }

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.484")
        self.assertEqual(self.evidence["range"]["start"], "0x01171C")
        self.assertEqual(self.evidence["range"]["end"], "0x011A4C")
        self.assertEqual(self.evidence["range"]["size_bytes"], 816)

        table = self.evidence["table"]
        self.assertEqual(table["address"], "0x011A3A")
        self.assertEqual(table["end"], "0x011A4C")
        self.assertEqual(table["size_bytes"], 18)
        self.assertEqual(table["values"], SPIRIT_ORB_TRAIL_TILE_TABLE)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 6)
        self.assertEqual(
            [r["name"] for r in routines],
            [
                "AI_SpiritOrb_State0_ElementalMediumAndCatalystScan",
                "AI_SpiritOrb_TransitionToState1Or2",
                "AI_SpiritOrb_ScanItemCatalysts",
                "AI_SpiritOrb_IntegrateAndUpdateTrail",
                "Tilemap_SampleLowNibbleAttribute",
                "AI_SpiritOrb_EmitTrailParticleVertex",
            ],
        )
        self.assertEqual(sum(r["size_bytes"] for r in routines), 816)
        self.assertEqual(self.evidence["instruction_count"], 234)
        self.assertEqual(len(self.evidence["instructions"]), 234)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x01171C")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x011A38")

    def test_state0_medium_aabb_entity_and_transition(self):
        # 1. Cancel flag $FF1990 clears flag, keeps $FF19A6, enters State 1 when unlocked
        sfx = []
        orb = self._make_orb(z_10_fixed=0x20000, z_10_word=2)
        ram = self._make_ram(ff1990=1, ff19a6=2, ff0d7a=0x04)
        self.assertEqual(run_state0_step(orb, ram, [], [], sfx), "STATE_TRANSITION")
        self.assertEqual(ram["ff1990"], 0)
        self.assertEqual((orb["state_04"], orb["field_2a"], orb["z_10_word"], orb["timer_02"]), (1, 2, 14, 0))
        self.assertEqual((orb["vel_x"], orb["vel_y"], orb["flags_36"] & 0x08), (0, 0, 0x08))
        self.assertEqual(sfx, [0x008004D2])

        # 2. Airborne Z decay clamped to 0 and bit 6 + bit 3 despawn
        orb = self._make_orb(z_10_fixed=0x10000, z_10_word=1, flags_40=0x48)
        self.assertEqual(run_state0_step(orb, self._make_ram(), [], [], []), "DESPAWNED")
        self.assertEqual((orb["z_10_fixed"], orb["type_00"]), (0, 0))

        # 3. Medium probes: +$39 bit 1, $FF17C2 <= 3 (+$6F bit 5), $FF17C3 <= 3 (+$6F bit 6)
        orb, ram = self._make_orb(flags_39=0x02), self._make_ram(ff19a6=3, ff0d7a=0x01)
        run_state0_step(orb, ram, [], [], [])
        self.assertEqual((ram["ff19a6"], orb["state_04"]), (0, 1))

        orb, ram = self._make_orb(flags_6f=0x20), self._make_ram(ff17c2=2, ff0d7a=0x04, ff16f1=0x01)
        run_state0_step(orb, ram, [], [], [])
        self.assertEqual((ram["ff19a6"], orb["state_04"], orb["field_2a"]), (2, 2, 4))

        orb, ram = self._make_orb(flags_6f=0x40), self._make_ram(ff17c3=3, ff0d7a=0x08)
        run_state0_step(orb, ram, [], [], [])
        self.assertEqual((ram["ff19a6"], orb["state_04"]), (3, 1))

        # 4. Entity AABB scan skips slots 0..1; Torch (0x64) + Crystal (0xB4) + affinity bits
        slot0 = self._make_entity(flags_39=0x08)
        torch = self._make_entity(x=170, y=125, flags_39=0x08)
        orb, ram = self._make_orb(z_10_fixed=0x30000, z_10_word=3), self._make_ram(ff0d7a=0x02)
        run_state0_step(orb, ram, [slot0, slot0, torch], [], [])
        self.assertEqual((slot0["state_02"], torch["state_02"], ram["ff19a6"], orb["state_04"]), (0, 1, 1, 1))

        crystal = self._make_entity(type_00=0x00B4, flags_3c=0x04)
        orb, ram = self._make_orb(z_10_fixed=0x30000, z_10_word=3), self._make_ram(ff0d7a=0x00)
        run_state0_step(orb, ram, [{}, {}, crystal], [], [])
        self.assertEqual((crystal["field_a6"], ram["ff19a6"], orb["state_04"]), (0xFF, 2, 2))

    def test_item_catalyst_matching_and_transmutation(self):
        # 1. Miss or non-0x0008 falls through to trail integration
        orb = self._make_orb(z_10_fixed=0x20000, z_10_word=2)
        self.assertEqual(
            run_state0_step(orb, self._make_ram(), [], [self._make_item(0x12, type_00=0x0007)], []),
            "INTEGRATE_AND_TRAIL",
        )

        # 2. Direct catalysts 0x12..0x15 for unlocked spirits 0..3
        for spirit_id, subtype in ITEM_DIRECT_CATALYSTS.items():
            item = self._make_item(subtype)
            orb = self._make_orb(z_10_fixed=0x20000, z_10_word=2)
            ram = self._make_ram(ff0d7a=(1 << spirit_id))
            run_state0_step(orb, ram, [], [item], [])
            self.assertEqual((ram["ff19a6"], orb["state_04"], item["subtype_32"]), (spirit_id, 1, subtype))

        # 3. All 5 item transmutations (1A->21, 1B->05+52=0x14, 1C->11, 1D->2A, 27->26)
        for src_sub, (spirit_id, dst_sub, extra_52) in ITEM_TRANSMUTATIONS.items():
            item = self._make_item(src_sub)
            orb = self._make_orb(z_10_fixed=0x20000, z_10_word=2)
            ram = self._make_ram(ff0d7a=(1 << spirit_id))
            run_state0_step(orb, ram, [], [item], [])
            self.assertEqual(item["subtype_32"], dst_sub)
            self.assertEqual(item["timer_22"], 0x1E)
            self.assertEqual(item["flags_36"] & 0x02, 0)
            self.assertEqual(item["field_52"], extra_52 if extra_52 is not None else 0)
            self.assertEqual((ram["ff19a6"], orb["state_04"]), (spirit_id, 1))

        # 4. Locked spirit or $FF16F1 inhibit rebounds to State 2
        item = self._make_item(0x1A)
        orb = self._make_orb(z_10_fixed=0x20000, z_10_word=2)
        run_state0_step(orb, self._make_ram(ff0d7a=0x00), [], [item], [])
        self.assertEqual((orb["state_04"], item["subtype_32"]), (2, 0x1A))

    def test_trail_allocation_and_nibble_sampler(self):
        # 1. Off-screen culling with 0x10 margin despawns entity
        orb_off = self._make_orb(x=400, y=100)
        self.assertIsNone(run_integrate_and_update_trail(orb_off, cam_x=0, cam_y=0))
        self.assertEqual(orb_off["type_00"], 0)

        # 2. 3-frame cooldown spawns 6 slots with alternating bit 7 sign
        orb = self._make_orb(x=100, y=100, facing_66=0)
        out0 = run_integrate_and_update_trail(orb, cam_x=0, cam_y=0)
        self.assertEqual((orb["pos_x_72"], orb["pos_y_76"]), (0x10000, -0x08000))
        self.assertEqual((orb["trail_timer_ac"], orb["trail_slots"][0]), (2, 0x82))
        self.assertEqual((out0["header"], orb["anim_06"], orb["vbuf_ptr_62"]), (0x80, 0x16, 0x00FF19A8))

        for _ in range(15):
            out = run_integrate_and_update_trail(orb, cam_x=0, cam_y=0)
        self.assertEqual(out["header"], 0x85)
        self.assertEqual([s & 0x80 for s in orb["trail_slots"]], [0x80, 0x00, 0x80, 0x00, 0x80, 0x00])

        orb_empty = self._make_orb(trail_timer_ac=2)
        out_empty = run_integrate_and_update_trail(orb_empty, cam_x=0, cam_y=0)
        self.assertIsNone(out_empty["header"])
        self.assertEqual(orb_empty["anim_06"], 0)

        # 3. Tilemap_SampleLowNibbleAttribute: ((y >> 3) << stride_shift) + (x >> 3), & 0x0F
        tilemap = [0xA0 | ((r * 16 + c) & 0x0F) for r in range(8) for c in range(16)]
        self.assertEqual(sample_low_nibble_attribute(tilemap, stride_shift=4, x=27, y=19), (2 * 16 + 3) & 0x0F)
        # Negative coordinate check for arithmetic shift right (asr.w #3): -1 >> 3 == -1
        # With x=-1, y=-1, stride_shift=4: d4 = (-1 << 4) + (-1) = -17
        wrapped_map = [0x00] * 128
        wrapped_map[-17] = 0x9C
        self.assertEqual(sample_low_nibble_attribute(wrapped_map, stride_shift=4, x=-1, y=-1), 0x0C)

    def test_trail_particle_vertex_emission_rotation_and_termination(self):
        # 1. All 4 facing rotations for age=2, positive (+1) and negative (-2) lateral offsets
        expected_rotations = [
            (0, 0x02, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], 1, 6)),
            (0, 0x82, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], -2, 6)),
            (1, 0x02, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], -18, -11)),
            (1, 0x82, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], -18, -14)),
            (2, 0x02, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], 1, -30)),
            (2, 0x82, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], -2, -30)),
            (3, 0x02, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], 18, -11)),
            (3, 0x82, (SPIRIT_ORB_TRAIL_TILE_TABLE[2], 18, -14)),
        ]
        for facing, slot_val, expected in expected_rotations:
            vert, _ = emit_trail_particle_vertex(slot_val, facing=facing)
            self.assertEqual(vert, expected)

        # 2. Age 17 is valid (table[17] == 19); age 18 writes 0xFF and wraps 0xFF + 1 -> 0x00
        self.assertEqual(emit_trail_particle_vertex(17, facing=3), ((19, 63, -11), 17))
        self.assertEqual(emit_trail_particle_vertex(0x92, facing=3), (None, 0xFF))

        orb = self._make_orb(trail_slots=[0x92, 0x05, 0, 0, 0, 0], trail_timer_ac=2, facing_66=0)
        out = run_integrate_and_update_trail(orb, cam_x=0, cam_y=0)
        self.assertEqual(orb["trail_slots"][0], 0x00)
        self.assertEqual(orb["trail_slots"][1], 0x06)
        self.assertEqual(out["header"], 0x80)
        self.assertEqual(out["vertices"], [(SPIRIT_ORB_TRAIL_TILE_TABLE[5], 1, 15)])


if __name__ == "__main__":
    unittest.main()
