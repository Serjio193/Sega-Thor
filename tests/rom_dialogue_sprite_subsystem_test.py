#!/usr/bin/env python3
"""Unit tests for ROM dialogue & HUD sprite SAT compositor and allocation subsystem (0x00A342..0x00A524)."""

import unittest
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class SpriteEntry:
    y: int
    link: int
    attr: int
    x: int


def calculate_reservations(has_portrait: bool, has_cursor: bool) -> tuple[int, int]:
    """Calculates bytes and sprite count for SAT reservation (0x00A4C8..0x00A51A)."""
    bytes_res = 48
    sprites_res = 6
    if has_portrait:
        bytes_res += 24
        sprites_res += 3
    if has_cursor:
        bytes_res += 8
        sprites_res += 1
    return bytes_res, sprites_res


def calculate_cursor_dma(frame_id: int) -> tuple[int, int, int]:
    """Calculates ROM source address, VRAM dest, and word count for cursor DMA (0x00A3EC..0x00A41A)."""
    # d0 = frame_id
    # d1 = d0; d0 += d0 (2); d0 += d0 (4); d0 += d0 (8); d0 += d1 (9)
    # d0 <<= 4 (9 * 16 = 144)
    # d0 += 0x000AAEEB
    rom_addr = frame_id * 144 + 0x000AAEEB
    vram_dest = 0xB8E0
    word_len = 0x90
    return rom_addr, vram_dest, word_len


def emit_dialogue_sprites(
    templates: List[SpriteEntry],
    base_x: int,
    start_sprite: int,
    has_portrait: bool,
    has_cursor: bool
) -> List[SpriteEntry]:
    """Simulates 0x00A364..0x00A3DA sprite generation."""
    sprites = []
    current_link = start_sprite

    # 6 dialogue box border sprites
    for i in range(6):
        tmpl = templates[i]
        current_link += 1
        sprites.append(SpriteEntry(
            y=tmpl.y,
            link=current_link,
            attr=tmpl.attr,
            x=tmpl.x + base_x
        ))

    # 3 portrait sprites
    if has_portrait:
        portrait_base_x = 0x158 - base_x
        for i in range(6, 9):
            tmpl = templates[i]
            current_link += 1
            sprites.append(SpriteEntry(
                y=tmpl.y,
                link=current_link,
                attr=tmpl.attr,
                x=tmpl.x + portrait_base_x
            ))

    # 1 cursor sprite
    if has_cursor and current_link < 0x50:
        current_link += 1
        sprites.append(SpriteEntry(
            y=0x118,
            link=current_link,
            attr=0x85C7,
            x=base_x + 0x68
        ))

    return sprites


class RomDialogueSpriteSubsystemTest(unittest.TestCase):
    def test_reservation_base_only(self):
        b, s = calculate_reservations(has_portrait=False, has_cursor=False)
        self.assertEqual(b, 48)
        self.assertEqual(s, 6)

    def test_reservation_with_portrait_and_cursor(self):
        b, s = calculate_reservations(has_portrait=True, has_cursor=True)
        self.assertEqual(b, 80)
        self.assertEqual(s, 10)

    def test_cursor_dma_address_calculation(self):
        rom_addr, vram_dest, word_len = calculate_cursor_dma(0)
        self.assertEqual(rom_addr, 0x000AAEEB)
        self.assertEqual(vram_dest, 0xB8E0)
        self.assertEqual(word_len, 0x90)

        rom_addr_1, _, _ = calculate_cursor_dma(1)
        self.assertEqual(rom_addr_1, 0x000AAEEB + 144)

    def test_emit_dialogue_sprites_count_and_coordinates(self):
        # Dummy template
        templates = [SpriteEntry(y=100 + i*10, link=0, attr=0x8000, x=10) for i in range(9)]
        sprites = emit_dialogue_sprites(templates, base_x=20, start_sprite=0,
                                        has_portrait=True, has_cursor=True)
        self.assertEqual(len(sprites), 10)
        # Check border sprite 0
        self.assertEqual(sprites[0].x, 30) # 10 + 20
        self.assertEqual(sprites[0].link, 1)
        # Check portrait sprite 6
        self.assertEqual(sprites[6].x, 10 + (0x158 - 20))
        self.assertEqual(sprites[6].link, 7)
        # Check cursor sprite
        self.assertEqual(sprites[9].x, 20 + 0x68)
        self.assertEqual(sprites[9].attr, 0x85C7)
        self.assertEqual(sprites[9].link, 10)


if __name__ == "__main__":
    unittest.main()
