#!/usr/bin/env python3
"""
Unit tests for ROM Cutscene & Event Script Virtual Machine Core, 98-Opcode Dispatcher
& Camera Pan Engine (0x00F788..0x00F920, Milestone M15.463).
"""

import unittest

class ScriptVmCore:
    def __init__(self):
        self.script_pc = 0
        self.script_stream = []
        self.paused = False
        self.delay_timer = 0
        self.pan_timer = 0
        self.pan_mode = 0  # 0: Up, 1: Right, 2: Down, 3: Left
        self.pan_speed = 0
        self.cam_x = 0
        self.cam_y = 0
        self.cam_locked = False
        self.player_suppress_19ec = 0
        self.player_suppress_1a12 = 0
        self.executed_opcodes = []

    def tick(self):
        # 0x00F788: btst.b #1, $FF17B8
        if self.paused:
            return "PAUSED"

        # 0x00F796: move.b $FF17B6, d1
        if self.delay_timer > 0:
            self.delay_timer -= 1
            return "DELAY"

        # 0x00F7AA: move.b $FF17BD, d1
        if self.pan_timer > 0:
            self._pan_camera_tick()
            return "PANNING"

        # 0x00F7B4..0x00F7CE: Bytecode interpreter loop
        while self.script_pc < len(self.script_stream):
            opcode = self.script_stream[self.script_pc]
            self.script_pc += 1
            self.executed_opcodes.append(opcode)

            # Opcode 0x00 is frame yield / exit: 0x00F91C
            if opcode == 0x00:
                break
            elif opcode == 0x01:
                # Custom mock opcode for testing multi-step execution
                pass

        return "INTERPRETED"

    def _pan_camera_tick(self):
        # 0x00F894: mode dispatch
        delta = self.pan_speed << 16
        if self.pan_mode == 0:    # 0x00F8D0: Up (sub Y)
            self.cam_y -= delta
        elif self.pan_mode == 1:  # 0x00F8E4: Right (add X)
            self.cam_x += delta
        elif self.pan_mode == 2:  # 0x00F8F6: Down (add Y)
            self.cam_y += delta
        elif self.pan_mode == 3:  # 0x00F908: Left (sub X)
            self.cam_x -= delta

        # 0x00F8A4: subq.b #1, d1
        self.pan_timer -= 1
        if self.pan_timer == 0:
            # 0x00F8B0: clear camera lock & player suppress
            self.cam_locked = False
            self.player_suppress_19ec = 0
            self.player_suppress_1a12 = 0


class TestScriptVmCore(unittest.TestCase):
    def test_pause_and_delay_timers(self):
        vm = ScriptVmCore()
        vm.paused = True
        self.assertEqual(vm.tick(), "PAUSED")

        vm.paused = False
        vm.delay_timer = 2
        self.assertEqual(vm.tick(), "DELAY")
        self.assertEqual(vm.delay_timer, 1)
        self.assertEqual(vm.tick(), "DELAY")
        self.assertEqual(vm.delay_timer, 0)

    def test_camera_pan_up_down(self):
        vm = ScriptVmCore()
        vm.cam_x = 0x00500000
        vm.cam_y = 0x00800000
        vm.cam_locked = True
        vm.player_suppress_19ec = 1
        vm.player_suppress_1a12 = 1

        # Pan Up 2 frames at speed 4
        vm.pan_mode = 0
        vm.pan_speed = 4
        vm.pan_timer = 2
        self.assertEqual(vm.tick(), "PANNING")
        self.assertEqual(vm.cam_y, 0x00800000 - (4 << 16))
        self.assertTrue(vm.cam_locked)

        self.assertEqual(vm.tick(), "PANNING")
        self.assertEqual(vm.cam_y, 0x00800000 - (8 << 16))
        self.assertFalse(vm.cam_locked)  # Unlocked on completion
        self.assertEqual(vm.player_suppress_19ec, 0)
        self.assertEqual(vm.player_suppress_1a12, 0)

    def test_camera_pan_left_right(self):
        vm = ScriptVmCore()
        vm.cam_x = 0x00500000
        vm.cam_y = 0x00800000

        # Pan Right 1 frame at speed 2
        vm.pan_mode = 1
        vm.pan_speed = 2
        vm.pan_timer = 1
        vm.tick()
        self.assertEqual(vm.cam_x, 0x00500000 + (2 << 16))

        # Pan Left 1 frame at speed 3
        vm.pan_mode = 3
        vm.pan_speed = 3
        vm.pan_timer = 1
        vm.tick()
        self.assertEqual(vm.cam_x, 0x00500000 + (2 << 16) - (3 << 16))

    def test_bytecode_execution_and_frame_yield(self):
        vm = ScriptVmCore()
        # Script stream: Opcode 1, Opcode 1, Opcode 0 (yield), Opcode 1, Opcode 0
        vm.script_stream = [0x01, 0x01, 0x00, 0x01, 0x00]
        vm.script_pc = 0

        # Frame 1 tick
        status1 = vm.tick()
        self.assertEqual(status1, "INTERPRETED")
        self.assertEqual(vm.executed_opcodes, [0x01, 0x01, 0x00])
        self.assertEqual(vm.script_pc, 3)

        # Frame 2 tick
        status2 = vm.tick()
        self.assertEqual(status2, "INTERPRETED")
        self.assertEqual(vm.executed_opcodes, [0x01, 0x01, 0x00, 0x01, 0x00])
        self.assertEqual(vm.script_pc, 5)

if __name__ == '__main__':
    unittest.main()
