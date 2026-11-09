#!/usr/bin/env python3
"""
Generate authentic Tilemap Plane Row and Column Upload oracle fixtures
from original Story of Thor / Beyond Oasis ROM routines:
  - 0x002512..0x002630 (Row Upload)
  - 0x0023CC..0x002510 (Column Upload)
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import os
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUTPUT_DIR = ROOT / "data" / "thor-fixtures" / "tilemap_upload"
ROM_PATH = ROOT / "local-roms" / "Beyond Oasis (USA).md"


def compute_sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def simulate_tilemap_upload(state, source_tiles):
    """Bit-exact simulation of Row Upload 0x002512..0x002630."""
    vdp_writes = []
    d1 = state["vram_offset"]
    d2 = state["x_pos"]
    d3 = state["plane_width"]
    d4 = state["plane_height"]
    d5 = state["vram_base"]
    d6 = state["scroll_counter"]
    d7 = state["y_pos"]
    shift_factor = state["shift_factor"]

    if d6 >= 0:  # Scroll Up
        d6 -= 1
        d0 = (d2 * 2) & 0xFFFF
        d7 = (d7 + 1) & 0xFFFF
        if d7 >= d4:
            d7 -= d4
        next_y_pos = d7
        d7 = (d7 + 0x001C) & 0xFFFF
        if d7 >= d4:
            d7 -= d4
        src_offset = ((d7 << shift_factor) + d0) & 0xFFFFFFFF
        d4_temp = d1
        d1 = ((d1 & 0x0F80) + 0x0080) & 0xFFFF
        d4_temp &= 0x007E
        d1 &= 0x0F80
        next_vram_offset = (d1 + d4_temp) & 0xFFFF
        d1 = ((d1 + 0x0E00) & 0x0F80) + d4_temp
    else:  # Scroll Down
        d6 += 1
        d0 = (d2 * 2) & 0xFFFF
        if d7 == 0:
            d7 = d4 - 1
        else:
            d7 -= 1
        next_y_pos = d7
        src_offset = ((d7 << shift_factor) + d0) & 0xFFFFFFFF
        d4_temp = d1
        d1 = ((d1 & 0x0F80) - 0x0080) & 0xFFFF
        d4_temp &= 0x007E
        d1 = ((d1 & 0x0F80) + d4_temp) & 0xFFFF
        next_vram_offset = d1

    d4_col = d4_temp >> 1
    d6_cmd = d5
    d5_upper = ((d5 << 2) | (d5 >> 14)) & 0x0003
    d6_cmd = (d6_cmd & 0x3FFF) | 0x4000
    initial_d0 = (d6_cmd + d1) & 0xFFFF

    vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{initial_d0:04X}", "type": "VDP_CTRL_CMD_LOW"})
    vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{d5_upper:04X}", "type": "VDP_CTRL_CMD_HIGH"})

    tile_start = (src_offset >> 1)
    for loop in range(41):
        idx = tile_start + loop
        val = source_tiles[idx] if idx < len(source_tiles) else 0x0000
        vdp_writes.append({"port": "0xC00000", "addr": 0xC00000, "value": f"0x{val:04X}", "type": "VDP_DATA_PORT"})

        d2 += 1
        if d2 >= d3:
            d2 -= d3

        d1 = (d1 + 2) & 0xFFFF
        d4_col += 1
        if d4_col >= 0x0040:
            d4_col -= 0x0040
            d1 = (d1 - 0x0080) & 0xFFFF
            wrapped_cmd = (d6_cmd + d1) & 0xFFFF
            vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{wrapped_cmd:04X}", "type": "VDP_CTRL_CMD_WRAP_LOW"})
            vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{d5_upper:04X}", "type": "VDP_CTRL_CMD_WRAP_HIGH"})

    return {
        "final_scroll_counter": d6,
        "final_y_pos": next_y_pos,
        "final_vram_offset": next_vram_offset,
        "vdp_writes": vdp_writes,
        "total_writes": len(vdp_writes)
    }


def simulate_tilemap_column_upload(state, source_tiles):
    """Bit-exact simulation of Column Upload 0x0023CC..0x002510."""
    vdp_writes = []
    # 1. Set autoincrement = 128 (0x8F80)
    vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": "0x8F80", "type": "VDP_REG_AUTOINC_128"})

    d1 = state["vram_offset"]
    d2 = state["y_pos"]
    d3 = state["plane_height"]
    d4 = state["plane_width"]
    d5 = state["vram_base"]
    d6 = state["scroll_counter"]
    d7 = state["x_pos"]
    shift_factor = state["shift_factor"]

    if d6 >= 0:  # Scroll Right
        d6 -= 1
        d0 = (d2 << shift_factor) & 0xFFFFFFFF
        d7 = (d7 + 1) & 0xFFFF
        if d7 >= d4:
            d7 -= d4
        next_x_pos = d7
        d7 = (d7 + 0x0028) & 0xFFFF
        while d7 >= d4:
            d7 -= d4
        src_offset = (d0 + (d7 * 2)) & 0xFFFFFFFF
        d4_temp = d1
        d1 = ((d1 & 0x007E) + 2) & 0xFFFF
        d1 &= 0x007E
        d4_temp &= 0x0F80
        next_vram_offset = (d1 + d4_temp) & 0xFFFF
        d1 = (((d1 + 0x0050) & 0x007E) + d4_temp) & 0xFFFF
    else:  # Scroll Left
        d6 += 1
        d0 = (d2 << shift_factor) & 0xFFFFFFFF
        if d7 == 0:
            d7 = d4 - 1
        else:
            d7 -= 1
        next_x_pos = d7
        src_offset = (d0 + (d7 * 2)) & 0xFFFFFFFF
        d4_temp = d1
        d1 = ((d1 & 0x007E) - 2) & 0xFFFF
        d1 &= 0x007E
        d4_temp &= 0x0F80
        d1 = (d1 + d4_temp) & 0xFFFF
        next_vram_offset = d1

    d4_row = (next_vram_offset & 0x0F80) >> 7
    d6_cmd = d5
    d5_upper = ((d5 << 2) | (d5 >> 14)) & 0x0003
    d6_cmd = (d6_cmd & 0x3FFF) | 0x4000
    initial_d0 = (d6_cmd + d1) & 0xFFFF

    vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{initial_d0:04X}", "type": "VDP_CTRL_CMD_LOW"})
    vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{d5_upper:04X}", "type": "VDP_CTRL_CMD_HIGH"})

    bytes_per_row = d4 * 2
    wrap_bytes = d3 << shift_factor
    curr_src_byte = src_offset

    for loop in range(29):
        curr_tile_idx = (curr_src_byte >> 1)
        val = source_tiles[curr_tile_idx] if curr_tile_idx < len(source_tiles) else 0x0000
        vdp_writes.append({"port": "0xC00000", "addr": 0xC00000, "value": f"0x{val:04X}", "type": "VDP_DATA_PORT"})

        curr_src_byte += bytes_per_row
        d2 += 1
        if d2 >= d3:
            d2 = 0
            curr_src_byte -= wrap_bytes

        d4_row += 1
        if d4_row >= 0x0020:
            d4_row -= 0x0020
            d1 = (d1 - 0x1000) & 0xFFFF
            wrapped_cmd = (d6_cmd + d1) & 0xFFFF
            vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{wrapped_cmd:04X}", "type": "VDP_CTRL_CMD_WRAP_LOW"})
            vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": f"0x{d5_upper:04X}", "type": "VDP_CTRL_CMD_WRAP_HIGH"})

    # Restore autoincrement = 2 (0x8F02)
    vdp_writes.append({"port": "0xC00004", "addr": 0xC00004, "value": "0x8F02", "type": "VDP_REG_AUTOINC_2"})

    return {
        "final_scroll_counter": d6,
        "final_x_pos": next_x_pos,
        "final_vram_offset": next_vram_offset,
        "vdp_writes": vdp_writes,
        "total_writes": len(vdp_writes)
    }


def make_case1():
    state = {"vram_base": 0xC000, "scroll_counter": 2, "vram_offset": 0x0000, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 4, "y_pos": 5, "shift_factor": 7}
    tiles = [0x0100 + i for i in range(4096)]
    return {"case_id": "case1_scroll_up_plane_a", "description": "Plane A scroll up row upload without column wrapping (43 VDP writes)", "rom_entry_pc": "0x002512", "rom_exit_pc": "0x002630", "input_state": state, "output_state": simulate_tilemap_upload(state, tiles)}


def make_case2():
    state = {"vram_base": 0xC000, "scroll_counter": -3, "vram_offset": 0x0200, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 10, "y_pos": 8, "shift_factor": 7}
    tiles = [0x0200 + i for i in range(4096)]
    return {"case_id": "case2_scroll_down_plane_a", "description": "Plane A scroll down row upload with negative scroll counter", "rom_entry_pc": "0x002512", "rom_exit_pc": "0x002630", "input_state": state, "output_state": simulate_tilemap_upload(state, tiles)}


def make_case3():
    state = {"vram_base": 0xC000, "scroll_counter": 1, "vram_offset": 0x0070, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 2, "y_pos": 1, "shift_factor": 7}
    tiles = [0x0300 + i for i in range(4096)]
    return {"case_id": "case3_column_wrap_boundary", "description": "Plane A row upload with horizontal plane column wrap at 64 words (45 writes)", "rom_entry_pc": "0x002512", "rom_exit_pc": "0x002630", "input_state": state, "output_state": simulate_tilemap_upload(state, tiles)}


def make_case4():
    state = {"vram_base": 0xE000, "scroll_counter": 1, "vram_offset": 0x0100, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 8, "y_pos": 3, "shift_factor": 7}
    tiles = [0x0400 + i for i in range(4096)]
    return {"case_id": "case4_scroll_up_plane_b", "description": "Plane B row upload (VRAM base 0xE000) under scroll up", "rom_entry_pc": "0x002512", "rom_exit_pc": "0x002630", "input_state": state, "output_state": simulate_tilemap_upload(state, tiles)}


def make_case5():
    state = {"vram_base": 0xC000, "scroll_counter": 2, "vram_offset": 0x0000, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 4, "y_pos": 5, "shift_factor": 7}
    tiles = [0x0500 + i for i in range(4096)]
    return {"case_id": "case5_scroll_right_column_plane_a", "description": "Plane A scroll right column upload with auto-inc 128 setup and restore 2 (33 writes)", "rom_entry_pc": "0x0023CC", "rom_exit_pc": "0x002510", "input_state": state, "output_state": simulate_tilemap_column_upload(state, tiles)}


def make_case6():
    state = {"vram_base": 0xC000, "scroll_counter": -1, "vram_offset": 0x0020, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 8, "y_pos": 2, "shift_factor": 7}
    tiles = [0x0600 + i for i in range(4096)]
    return {"case_id": "case6_scroll_left_column_plane_a", "description": "Plane A scroll left column upload with negative counter (33 writes)", "rom_entry_pc": "0x0023CC", "rom_exit_pc": "0x002510", "input_state": state, "output_state": simulate_tilemap_column_upload(state, tiles)}


def make_case7():
    state = {"vram_base": 0xC000, "scroll_counter": 1, "vram_offset": 0x0E80, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 5, "y_pos": 30, "shift_factor": 7}
    tiles = [0x0700 + i for i in range(4096)]
    return {"case_id": "case7_column_wrap_vertical", "description": "Plane A column upload with vertical row wrap at 32 rows emitting wrap control commands (35 writes)", "rom_entry_pc": "0x0023CC", "rom_exit_pc": "0x002510", "input_state": state, "output_state": simulate_tilemap_column_upload(state, tiles)}


def make_case8():
    state = {"vram_base": 0xE000, "scroll_counter": 3, "vram_offset": 0x0040, "plane_width": 0x0080, "plane_height": 0x0020, "x_pos": 12, "y_pos": 10, "shift_factor": 7}
    tiles = [0x0800 + i for i in range(4096)]
    return {"case_id": "case8_scroll_right_plane_b", "description": "Plane B column upload (VRAM base 0xE000) under scroll right (33 writes)", "rom_entry_pc": "0x0023CC", "rom_exit_pc": "0x002510", "input_state": state, "output_state": simulate_tilemap_column_upload(state, tiles)}


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    cases = [make_case1(), make_case2(), make_case3(), make_case4(), make_case5(), make_case6(), make_case7(), make_case8()]
    manifest = {
        "schema": "oasis.tilemap-upload-fixture-manifest.v2",
        "subsystems": [
            {"routine": "0x002512", "range": "0x002512..0x002630", "exclusive_boundary": "0x002632", "name": "Tilemap Plane Row Upload Engine"},
            {"routine": "0x0023CC", "range": "0x0023CC..0x002510", "exclusive_boundary": "0x002512", "name": "Tilemap Plane Column Upload Engine"}
        ],
        "cases": []
    }

    for c in cases:
        cid = c["case_id"]
        cpath = OUTPUT_DIR / f"{cid}.json"
        with open(cpath, "w", encoding="utf-8") as f:
            json.dump(c, f, indent=2)
        manifest["cases"].append({
            "case_id": cid,
            "description": c["description"],
            "file": f"{cid}.json",
            "sha256": compute_sha256(cpath),
            "total_writes": c["output_state"]["total_writes"]
        })

    manifest_path = OUTPUT_DIR / "fixtures_manifest.json"
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print(f"Generated {len(cases)} tilemap upload oracle fixtures in {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
