#!/usr/bin/env python3
"""Unit tests for ROM demo mode overlay SAT compositor & camera particle spark generator (0x00A7E6..0x00A9A0)."""

import unittest
from dataclasses import dataclass
from typing import List, Optional, Tuple


@dataclass
class SparkEntity:
    entity_type: int
    world_x: int
    world_y: int
    lifetime: int


def process_demo_overlay(status_flags: int, current_sync: int) -> Tuple[int, Optional[Tuple[int, int, int]]]:
    """Simulates 0x00A7E6..0x00A8EE demo HUD overlay."""
    if status_flags & 0x80:
        return 0, None

    if status_flags & 0x40:
        sprite_count = 3
        dma = (0x000AA1FD, 0xF360, 0xC0) if current_sync != 1 else None
        return sprite_count, dma
    else:
        sprite_count = 2
        dma = (0x000AA1AD, 0xF360, 0x50) if current_sync != 0xFF else None
        return sprite_count, dma


def process_spark_particle(
    spark: SparkEntity,
    cam_x: int,
    cam_y: int,
    current_sprites: int
) -> Tuple[bool, Optional[Tuple[int, int]]]:
    """Simulates 0x00A8F0..0x00A99E camera-relative particle spark generator."""
    spark.lifetime -= 1
    if spark.lifetime <= 0:
        spark.entity_type = 0
        return False, None

    if current_sprites >= 80:
        return False, None

    # Screen coordinate calculation (0x00A950..0x00A980)
    screen_y = (spark.world_y - cam_y) + 120
    screen_x = (spark.world_x - cam_x) + 120

    # Bounds check against max_y (240) and max_x (336)
    adj_y = screen_y - 112
    if adj_y < 0 or adj_y > 240:
        return False, None

    adj_x = screen_x - 112
    if adj_x < 0 or adj_x > 336:
        return False, None

    return True, (screen_x, screen_y)


class RomDemoHudSparkGeneratorTest(unittest.TestCase):
    def test_demo_overlay_suppressed_when_bit7_set(self):
        sprites, dma = process_demo_overlay(status_flags=0x80, current_sync=0)
        self.assertEqual(sprites, 0)
        self.assertIsNone(dma)

    def test_demo_overlay_mode_3_sprites(self):
        sprites, dma = process_demo_overlay(status_flags=0x40, current_sync=0)
        self.assertEqual(sprites, 3)
        self.assertEqual(dma, (0x000AA1FD, 0xF360, 0xC0))

    def test_demo_overlay_mode_2_sprites(self):
        sprites, dma = process_demo_overlay(status_flags=0x00, current_sync=0)
        self.assertEqual(sprites, 2)
        self.assertEqual(dma, (0x000AA1AD, 0xF360, 0x50))

    def test_spark_lifetime_expiry_clears_type(self):
        spark = SparkEntity(entity_type=1, world_x=100, world_y=100, lifetime=1)
        emitted, _ = process_spark_particle(spark, cam_x=100, cam_y=100, current_sprites=10)
        self.assertFalse(emitted)
        self.assertEqual(spark.entity_type, 0)

    def test_spark_in_camera_bounds_emits(self):
        spark = SparkEntity(entity_type=1, world_x=150, world_y=150, lifetime=10)
        emitted, coords = process_spark_particle(spark, cam_x=100, cam_y=100, current_sprites=10)
        self.assertTrue(emitted)
        # screen_x = 150 - 100 + 120 = 170
        # screen_y = 150 - 100 + 120 = 170
        self.assertEqual(coords, (170, 170))

    def test_spark_out_of_bounds_dropped(self):
        spark = SparkEntity(entity_type=1, world_x=600, world_y=600, lifetime=10)
        emitted, coords = process_spark_particle(spark, cam_x=100, cam_y=100, current_sprites=10)
        self.assertFalse(emitted)
        self.assertIsNone(coords)


if __name__ == "__main__":
    unittest.main()
