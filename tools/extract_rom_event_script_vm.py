#!/usr/bin/env python3
"""
M15.260 ROM Event Script VM & Dialogue Resource Semantic Extractor.
Extracts:
  - 31 Opcode definitions from 0x00F7D0
  - 21 Area spawn script tables from 0x04371E (all 188 room scripts)
  - 98 Dialogue text records from 0x051514
  - Village Elder NPC 0x003C interaction rules and Screen(1,8) spawn sequence
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from __future__ import annotations
import json
import pathlib
import struct
import sys

OPCODE_METADATA: dict[int, dict] = {
    0x00: {"name": "VM_OP_HALT_YIELD", "handler": "0x00F91C", "operand_bytes": 0, "types": [], "status": "FULLY_DECODED",
           "semantics": "Pops caller return frame (addq.l #4, a7; rts); terminates current frame execution loop."},
    0x01: {"name": "VM_OP_SPAWN_ENEMY", "handler": "0x00FCCE", "operand_bytes": 3, "types": ["u8:flag", "u16:enemy_type"], "status": "FULLY_DECODED",
           "semantics": "Checks enemy count ($FF17B7); allocates entity slot via 0x00DA2A; sets type and initializes via 0x008D06."},
    0x02: {"name": "VM_OP_LOAD_WORD_TABLE_17C6", "handler": "0x00F920", "operand_bytes": -1, "types": ["u16[]:words_until_ffff"], "status": "FULLY_DECODED",
           "semantics": "Populates coordinate/parameter array at $FF17C6 until 0xFFFF terminator."},
    0x03: {"name": "VM_OP_LOAD_WORD_PAIRS_17E6", "handler": "0x00F942", "operand_bytes": -1, "types": ["(u16,u16)[]:pairs_until_ffff"], "status": "FULLY_DECODED",
           "semantics": "Populates position pair array at $FF17E6 until 0xFFFF terminator."},
    0x04: {"name": "VM_OP_CONDITIONAL_BRANCH", "handler": "0x00F96A", "operand_bytes": 4, "types": ["u8:cond_type", "u8:param", "i16:disp_or_thresh"], "status": "FULLY_DECODED",
           "semantics": "Evaluates condition (0=entity dead/count, 1=player coord, 2=item, 3=frame timer, 4=wave defeat); branches on match."},
    0x05: {"name": "VM_OP_BRANCH_UNCONDITIONAL", "handler": "0x00FAAC", "operand_bytes": 2, "types": ["i16:relative_disp"], "status": "FULLY_DECODED",
           "semantics": "Relative jump in bytecode script: adda.w 2(a0), a0."},
    0x06: {"name": "VM_OP_LOOP_DECREMENT_BRANCH", "handler": "0x00FC6C", "operand_bytes": 3, "types": ["u8:counter_idx", "i16:relative_disp"], "status": "FULLY_DECODED",
           "semantics": "Decrements counter at $FF1826[counter_idx]; if non-zero, branches by displacement."},
    0x07: {"name": "VM_OP_SET_COUNTER", "handler": "0x00FC94", "operand_bytes": 3, "types": ["u8:counter_idx", "u16:count"], "status": "FULLY_DECODED",
           "semantics": "Initializes loop counter at $FF1826[counter_idx] to count."},
    0x08: {"name": "VM_OP_WAIT_FRAMES", "handler": "0x00FCA6", "operand_bytes": 1, "types": ["u8:frames"], "status": "FULLY_DECODED",
           "semantics": "Sets wait frame countdown in $FF17B6, saves PC to $FF17B2, and yields execution."},
    0x09: {"name": "VM_OP_SPAWN_PROJECTILE_SPECIAL", "handler": "0x010034", "operand_bytes": 7, "types": ["u8:mode", "u16:params", "i16:x", "i16:y"], "status": "FULLY_DECODED",
           "semantics": "Allocates entity slot via 0x00D9BE; initializes projectile type 8 at world coordinates."},
    0x0A: {"name": "VM_OP_PRELOAD_NEMESIS_DMA", "handler": "0x00FCB8", "operand_bytes": 7, "types": ["u8:flag", "u32:rom_src", "u16:vram_dst"], "status": "FULLY_DECODED",
           "semantics": "Queues Nemesis pattern decompression/DMA stream via 0x00D9A4 into VRAM destination."},
    0x0B: {"name": "VM_OP_SET_MAX_ENEMIES", "handler": "0x00FCC4", "operand_bytes": 1, "types": ["u8:max_count"], "status": "FULLY_DECODED",
           "semantics": "Updates active enemy concurrency threshold in $FF17B7."},
    0x0C: {"name": "VM_OP_LOAD_PALETTE_STAGE", "handler": "0x010086", "operand_bytes": 2, "types": ["u16:palette_idx"], "status": "FULLY_DECODED",
           "semantics": "Copies 32 bytes from 0x05D046 to palette staging buffer $FF136C without immediate reload flag."},
    0x0D: {"name": "VM_OP_LOAD_PALETTE_APPLY", "handler": "0x0100AA", "operand_bytes": 2, "types": ["u16:palette_idx"], "status": "FULLY_DECODED",
           "semantics": "Copies 32 bytes from 0x05D046 to $FF136C and flags VDP reload (bset.b #0, $FF164D)."},
    0x0E: {"name": "VM_OP_SPAWN_COMPLEX_ENEMY", "handler": "0x00FD8C", "operand_bytes": 11, "types": ["u8:flags", "u16:type", "i16:x", "i16:y", "u32:extra"], "status": "FULLY_DECODED",
           "semantics": "Checks enemy cap, allocates entity slot, and initializes multi-piece enemy via 0x0102BE."},
    0x0F: {"name": "VM_OP_SET_GLOBAL_FLAG", "handler": "0x010140", "operand_bytes": 3, "types": ["u8:mode", "u16:flag_index"], "status": "FULLY_DECODED",
           "semantics": "Sets (1), clears (0), or toggles (2) bitflag in global table $FF0DBC (bit=idx&7, byte=idx>>3)."},
    0x10: {"name": "VM_OP_SET_AREA_FLAG", "handler": "0x010170", "operand_bytes": 3, "types": ["u8:mode", "u16:flag_index"], "status": "FULLY_DECODED",
           "semantics": "Sets (1), clears (0), or toggles (2) bitflag in area table $FF0E1C."},
    0x11: {"name": "VM_OP_SET_ROOM_FLAG", "handler": "0x0101A0", "operand_bytes": 3, "types": ["u8:mode", "u16:flag_index"], "status": "FULLY_DECODED",
           "semantics": "Sets (1), clears (0), or toggles (2) bitflag in room table $FF0E2C."},
    0x12: {"name": "VM_OP_TILEMAP_SPECIAL_ACTION", "handler": "0x00FAB2", "operand_bytes": -1, "types": ["u8:action_id", "varies"], "status": "PARTIALLY_DECODED",
           "semantics": "Sub-dispatcher via 0x00FAC2 (12 sub-actions: animated tiles, plane priority, water current, tile swapping)."},
    0x13: {"name": "VM_OP_CALL_SUBROUTINE", "handler": "0x0101D0", "operand_bytes": 3, "types": ["u24:rom_target"], "status": "FULLY_DECODED",
           "semantics": "Saves current PC into $FF1836 and jumps to target bytecode subroutine address."},
    0x14: {"name": "VM_OP_RETURN_SUBROUTINE", "handler": "0x0101E4", "operand_bytes": 0, "types": [], "status": "FULLY_DECODED",
           "semantics": "Restores script PC from $FF1836 (return from bytecode subroutine)."},
    0x15: {"name": "VM_OP_SPAWN_GATED_PROP", "handler": "0x00FEDE", "operand_bytes": 17, "types": ["u8:flag_bank", "u16:type", "u16:flag_idx", "u8:f6", "u8:f7", "u16:msg_id", "i16:x", "i16:y", "i16:z", "u16:vram"], "status": "FULLY_DECODED",
           "semantics": "Tests bitflag (0=none, 1=$FF0DBC, 2=$FF0E1C, 3=$FF0E2C). If clear, spawns entity via 0x00DA2A + 0x00DF54. If set, skips 18-byte record."},
    0x16: {"name": "VM_OP_SPAWN_STATIC_PROP", "handler": "0x00FF1A", "operand_bytes": 17, "types": ["u8:flag_bank", "u16:type", "u16:flag_idx", "u8:f6", "u8:f7", "u16:msg_id", "i16:x", "i16:y", "i16:z", "u16:vram"], "status": "FULLY_DECODED",
           "semantics": "Unconditionally spawns entity using standard 18-byte record via 0x00DA2A + 0x008D06 + 0x00DF54."},
    0x17: {"name": "VM_OP_SPAWN_ENEMY_WAVE", "handler": "0x00FE0A", "operand_bytes": 5, "types": ["u8:flags", "u16:type_a", "u16:type_b"], "status": "FULLY_DECODED",
           "semantics": "Spawns paired wave entities (#$72 and #$70) and synchronizes wave scheduler via 0x010300."},
    0x18: {"name": "VM_OP_SPAWN_ITEM_CHEST", "handler": "0x00FF98", "operand_bytes": 11, "types": ["u8:payload", "u16:type", "u32:func", "i16:x", "i16:y"], "status": "FULLY_DECODED",
           "semantics": "Allocates slot via 0x00D9F0; spawns chest or pickup object with payload ID and open callback 0x009AD6."},
    0x19: {"name": "VM_OP_SPAWN_TRIGGER_OBJECT", "handler": "0x00FFDE", "operand_bytes": 17, "types": ["u8:payload", "u16:type", "i16:x", "i16:y", "varies"], "status": "FULLY_DECODED",
           "semantics": "Spawns interactive trigger entity (door, floor switch, chest) with default trigger callback 0x017A96."},
    0x1A: {"name": "VM_OP_PLAY_SOUND_EFFECT", "handler": "0x0100D6", "operand_bytes": 8, "types": ["u16:sound_id", "u16:vol", "u32:pan_mode"], "status": "FULLY_DECODED",
           "semantics": "Sends sound effect command packet to Z80 audio driver via 0x00E78E."},
    0x1B: {"name": "VM_OP_PLAY_BGM_TRACK", "handler": "0x0100E4", "operand_bytes": 4, "types": ["u32:bgm_command"], "status": "FULLY_DECODED",
           "semantics": "Sends BGM playback command to sound driver via 0x060004."},
    0x1C: {"name": "VM_OP_SET_ROOM_PARAMETER", "handler": "0x0100EE", "operand_bytes": 2, "types": ["u16:param"], "status": "FULLY_DECODED",
           "semantics": "Stores room-specific parameter word in $FF17C4."},
    0x1D: {"name": "VM_OP_PLANE_SCROLL_EFFECT", "handler": "0x0100F8", "operand_bytes": 3, "types": ["u8:plane_sel", "i16:scroll_delta"], "status": "FULLY_DECODED",
           "semantics": "Configures auto-scroll delta on Plane A ($FF1716) or Plane B ($FF173E) via 0x00DB64."},
    0x1E: {"name": "VM_OP_CAMERA_CUTSCENE_LOCK", "handler": "0x00F854", "operand_bytes": 5, "types": ["u8:mode", "u16:target_cam_x", "u16:target_cam_y"], "status": "FULLY_DECODED",
           "semantics": "Locks camera on target coordinates, activates cutscene flag $FF1982=0xFF, sets camera mode $FF19EC=2, and yields execution."},
}


def extract_all(rom_bytes: bytes) -> dict:
    # 1. Opcode table at 0x00F7D0
    opcodes = {}
    for op in range(31):
        entry_addr = 0x00F7D0 + op * 2
        offset = struct.unpack_from(">h", rom_bytes, entry_addr)[0]
        handler_addr = entry_addr + offset
        meta = OPCODE_METADATA.get(op, {"name": f"VM_OP_0x{op:02X}", "handler": f"0x{handler_addr:06X}", "operand_bytes": -1, "types": [], "status": "UNKNOWN", "semantics": "Unknown"})
        opcodes[f"0x{op:02X}"] = {
            "opcode": f"0x{op:02X}",
            "opcode_int": op,
            "entry_addr": f"0x{entry_addr:06X}",
            "offset": offset,
            "handler_addr": f"0x{handler_addr:06X}",
            "name": meta["name"],
            "operand_bytes": meta["operand_bytes"],
            "operand_types": meta["types"],
            "status": meta["status"],
            "semantics": meta["semantics"],
        }

    # 2. Dialogue Table at 0x051514 (98 messages)
    table_base = 0x051514
    dialogue_records = []
    for msg_id in range(98):
        entry_addr = table_base + msg_id * 2
        offset = struct.unpack_from(">h", rom_bytes, entry_addr)[0]
        text_addr = entry_addr + offset
        end = text_addr
        while end < len(rom_bytes) and rom_bytes[end] != 0:
            end += 1
        raw = rom_bytes[text_addr:end]

        pages = []
        curr_page_lines = []
        curr_line = ""
        for b in raw:
            if b == 0x01:
                curr_page_lines.append(curr_line)
                curr_line = ""
            elif b == 0x07:
                if curr_line:
                    curr_page_lines.append(curr_line)
                    curr_line = ""
                if curr_page_lines:
                    pages.append(curr_page_lines)
                    curr_page_lines = []
            elif b == ord("#"):
                continue  # Authentic speaker border tag
            elif 32 <= b <= 126:
                curr_line += chr(b)
        if curr_line:
            curr_page_lines.append(curr_line)
        if curr_page_lines:
            pages.append(curr_page_lines)

        dialogue_records.append({
            "message_id": msg_id,
            "entry_addr": f"0x{entry_addr:06X}",
            "offset": offset,
            "text_rom_addr": f"0x{text_addr:06X}",
            "raw_byte_len": len(raw),
            "page_count": len(pages),
            "pages": pages,
        })

    # 3. Area Spawn Script Tables at 0x04371E (21 areas)
    master_spawn_table = 0x04371E
    areas = []
    total_room_scripts = 0
    for area_idx in range(21):
        area_ptr = struct.unpack_from(">I", rom_bytes, master_spawn_table + area_idx * 4)[0]
        area_scripts = []
        # Area room count bounds from M15.259
        max_rooms = 16 if area_idx in (1, 2) else 10
        for r_idx in range(max_rooms):
            rentry = area_ptr + r_idx * 2
            if rentry + 2 > len(rom_bytes):
                break
            roff = struct.unpack_from(">h", rom_bytes, rentry)[0]
            script_addr = rentry + roff
            if 0x040000 <= script_addr <= 0x060000:
                area_scripts.append({
                    "room_slot": r_idx,
                    "room_id": f"0x{area_idx:02X}{r_idx:02X}",
                    "entry_addr": f"0x{rentry:06X}",
                    "offset": roff,
                    "script_addr": f"0x{script_addr:06X}",
                })
                total_room_scripts += 1
        areas.append({
            "area_index": area_idx,
            "area_table_addr": f"0x{area_ptr:06X}",
            "scripts": area_scripts,
        })

    # 4. Village Elder Screen(1,8) Specific Trace & Semantic Profile
    elder_spawn_record = rom_bytes[0x0466BA:0x0466BA+18]
    elder_semantics = {
        "screen_id": "Screen(1, 8)",
        "room_id": "0x0108",
        "descriptor_addr": "0x02F116",
        "spawn_script_addr": "0x0466B2",
        "entity_type_id": "0x003C",
        "entity_name": "Village Elder",
        "opcode_sequence": [
            {"step": 1, "pc": "0x0466B2", "opcode": "0x0A", "name": "VM_OP_PRELOAD_NEMESIS_DMA",
             "params": {"flag": 0, "rom_src": "0x166120", "vram_dst": "0x9200", "vram_tile_base": "0x0490"}},
            {"step": 2, "pc": "0x0466BA", "opcode": "0x15", "name": "VM_OP_SPAWN_GATED_PROP",
             "params": {
                 "flag_bank": 1, "flag_bank_name": "$FF0DBC (Global)", "flag_index": 141,
                 "flag_byte_offset": 17, "flag_bit_index": 5, "flag_address": "0x00FF0DBC + 17",
                 "entity_type": "0x003C", "world_x": 160, "world_y": 160, "world_z": 562,
                 "vram_tile_base": "0x0490", "message_id_param": 0
             }},
            {"step": 3, "pc": "0x0466CC", "opcode": "0x00", "name": "VM_OP_HALT_YIELD", "params": {}},
        ],
        "interaction_mechanics": {
            "entity_init_handler": "0x017D12 (Table 0x00DF54 - 0x2C + 0x3C*2)",
            "interaction_collision_check": "0x00818A (Checks target type 0x003C)",
            "talk_trigger_routine": "0x018C28 (Reads entity message index and calls 0x00C2EC)",
            "proximity_threshold_px": 28,
            "trigger_buttons": ["A", "C"],
            "player_state_lock": "0x36 (bset.b #1, $36(player) in 0x018C72)",
            "authentic_message_id": 5,
            "message_entry_addr": "0x05151E",
            "message_text_addr": "0x05178C",
            "dialogue_engine_entry": "0x00C326",
            "dialogue_frame_step_loop": "0x00C84E (Maintains animation while waiting input)",
            "button_wait_routine": "0x00C536 (Tests bits 4,5,6 of $FF165F)",
            "completion_state": "Sets global event flag 141 ($FF0DBC+17 bit 5); clears player lock; unfreezes simulation",
        },
        "text_content": dialogue_records[5],
        "gpgx_trace_status": {
            "reached_in_existing_fixtures": False,
            "reason": "Existing 6 scenarios (scenario_a..c, longrun_3600f, longrun_alt_1800f, longrun_combat_1800f) cover Beach (0x0100..0x0102) and Town/Waterway (0x0103..0x0105), but do not enter Village House interior 0x0108.",
            "reproducible_capture_procedure": (
                "1. Load scenario_b or canonical_beach_frame3600 checkpoint (Screen 0x0102). "
                "2. Navigate to Beach North doorway at [168, 272] triggering AABB 0x00C1CE transition to Room 0x0108. "
                "3. In Room 0x0108, walk to [160, 160] facing North towards Village Elder (0x003C). "
                "4. Press Button A or C; observe dialogue window opening, frame loop at 0x00C84E, button advance at 0x00C536, and flag 141 set."
            )
        }
    }

    fully_decoded_count = sum(1 for op in opcodes.values() if op["status"] == "FULLY_DECODED")
    partially_decoded_count = sum(1 for op in opcodes.values() if op["status"] == "PARTIALLY_DECODED")
    unknown_count = sum(1 for op in opcodes.values() if op["status"] == "UNKNOWN")

    return {
        "milestone": "M15.260",
        "rom_sha256": "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263",
        "summary_metrics": {
            "vm_opcodes_discovered": len(opcodes),
            "vm_opcodes_fully_decoded": fully_decoded_count,
            "vm_opcodes_partially_decoded": partially_decoded_count,
            "vm_opcodes_unknown": unknown_count,
            "dialogue_messages_extracted": len(dialogue_records),
            "world_areas_extracted": len(areas),
            "total_room_scripts_mapped": total_room_scripts,
            "village_elder_message_id": 5,
            "village_elder_script_addr": "0x0466B2",
            "village_elder_text_addr": "0x05178C",
        },
        "opcodes": opcodes,
        "dialogue_resources": {
            "table_address": "0x051514",
            "addressing_mode": "self_relative_int16_offset (entry + offset)",
            "control_codes": {
                "0x00": "end_of_dialogue (waits button press via 0x00C536, teardown via 0x00C45C)",
                "0x01": "line_feed (advances cursor to next line via 0x00C566)",
                "0x02": "clear_and_newline",
                "0x03": "frame_delay (byte operand specifies frame count)",
                "0x05": "wait_button_press (bsr 0x00C536)",
                "0x06": "scroll_four_lines",
                "0x07": "page_pause (waits button press via 0x00C536, then clears page)",
                "#": "speaker_border_delimiter (authentic tag in ROM text, omitted in rendered prose)",
            },
            "records": dialogue_records,
        },
        "spawn_scripts": {
            "master_table_addr": "0x04371E",
            "areas": areas,
        },
        "village_elder_trace": elder_semantics,
    }


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parent.parent
    rom_path = repo_root / "local-roms" / "Beyond Oasis (USA).md"
    assert rom_path.exists(), f"Missing ROM: {rom_path}"
    rom_bytes = rom_path.read_bytes()

    evidence_data = extract_all(rom_bytes)

    out_dirs = [repo_root / "data" / "thor-evidence", repo_root / "evidence"]
    for out_dir in out_dirs:
        out_dir.mkdir(parents=True, exist_ok=True)
        out_file = out_dir / "rom_event_script_vm_m15_260.json"
        out_file.write_text(json.dumps(evidence_data, indent=2), encoding="utf-8")
        print(f"Wrote {out_file} ({out_file.stat().st_size} bytes)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
