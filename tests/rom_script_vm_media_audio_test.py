#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Opcodes $09, $0C, $0D, $1A..$1D:
Media, Audio, Palette & Screen Shake Subsystem (0x010034..0x010140, Milestone M15.469).
"""

import unittest

class ScriptMediaAudioEngine:
    def __init__(self):
        self.palette_buffer = [0] * 16
        self.active_palette_id = 0
        self.cram_transfer_requested = False
        self.sfx_played = []
        self.bgm_played = []
        self.screen_shake_timer = 0
        self.channel_effects = []
        self.spawned_items = []

    def op_09_spawn_item(self, category, item_id, x, y, lifetime, alloc_fail=False):
        if alloc_fail:
            return "YIELD_FRAME"
        item = {
            'type': 8,
            'category': category,
            'id': item_id,
            'pos_x': x,
            'pos_y': y,
            'launch_flag': True,
            'lifetime': lifetime & 0x3F
        }
        self.spawned_items.append(item)
        return "SPAWNED"

    def op_0c_load_palette(self, pal_id, rom_palette_data):
        self.active_palette_id = pal_id
        self.palette_buffer = list(rom_palette_data)

    def op_0d_load_and_apply_palette(self, pal_id, rom_palette_data):
        self.op_0c_load_palette(pal_id, rom_palette_data)
        self.cram_transfer_requested = True

    def op_1a_play_sfx(self, d1, d2, d0):
        self.sfx_played.append((d1, d2, d0))

    def op_1b_play_bgm(self, cmd):
        self.bgm_played.append(cmd)

    def op_1c_screen_shake(self, frames):
        self.screen_shake_timer = frames

    def op_1d_channel_effect(self, channel_id, effect_type):
        buf_name = "$FF1716" if channel_id < 3 else "$FF173E"
        ch = channel_id if channel_id < 3 else channel_id - 3
        self.channel_effects.append((buf_name, ch, effect_type))


class TestScriptMediaAudioEngine(unittest.TestCase):
    def test_op_09_item_spawn(self):
        engine = ScriptMediaAudioEngine()
        res = engine.op_09_spawn_item(category=1, item_id=5, x=150, y=200, lifetime=42)
        self.assertEqual(res, "SPAWNED")
        self.assertEqual(len(engine.spawned_items), 1)
        it = engine.spawned_items[0]
        self.assertEqual(it['type'], 8)
        self.assertEqual(it['category'], 1)
        self.assertEqual(it['id'], 5)
        self.assertTrue(it['launch_flag'])
        self.assertEqual(it['lifetime'], 42)

    def test_op_0c_0d_palette_loading(self):
        engine = ScriptMediaAudioEngine()
        dummy_pal = [0x000, 0x222, 0x444, 0x666] * 4

        # 0x0C loads buffer only
        engine.op_0c_load_palette(2, dummy_pal)
        self.assertEqual(engine.active_palette_id, 2)
        self.assertEqual(engine.palette_buffer, dummy_pal)
        self.assertFalse(engine.cram_transfer_requested)

        # 0x0D loads and sets CRAM request flag
        engine.op_0d_load_and_apply_palette(3, dummy_pal)
        self.assertEqual(engine.active_palette_id, 3)
        self.assertTrue(engine.cram_transfer_requested)

    def test_op_1a_1b_audio_dispatch(self):
        engine = ScriptMediaAudioEngine()
        engine.op_1a_play_sfx(10, 20, 0x00B00224)
        self.assertEqual(engine.sfx_played, [(10, 20, 0x00B00224)])

        engine.op_1b_play_bgm(0x00050001)
        self.assertEqual(engine.bgm_played, [0x00050001])

    def test_op_1c_1d_shake_and_channel_effects(self):
        engine = ScriptMediaAudioEngine()
        engine.op_1c_screen_shake(30)
        self.assertEqual(engine.screen_shake_timer, 30)

        # Channel 1 (< 3)
        engine.op_1d_channel_effect(1, 0)
        # Channel 4 (>= 3)
        engine.op_1d_channel_effect(4, 2)
        self.assertEqual(engine.channel_effects, [
            ("$FF1716", 1, 0),
            ("$FF173E", 1, 2)
        ])

if __name__ == '__main__':
    unittest.main()
