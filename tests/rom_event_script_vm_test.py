#!/usr/bin/env python3
"""
Deterministic Verification Suite for M15.260 ROM Event Script VM &
Dialogue Resource Semantic Extraction.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from __future__ import annotations
import json
import pathlib
import sys


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parent.parent
    json_path = repo_root / "data" / "thor-evidence" / "rom_event_script_vm_m15_260.json"
    assert json_path.exists(), f"Missing extracted evidence JSON: {json_path}"

    data = json.loads(json_path.read_text(encoding="utf-8"))
    assert data["milestone"] == "M15.260"
    assert data["rom_sha256"] == "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

    sm = data["summary_metrics"]
    assert sm["vm_opcodes_discovered"] == 31
    assert sm["vm_opcodes_fully_decoded"] == 30
    assert sm["vm_opcodes_partially_decoded"] == 1
    assert sm["vm_opcodes_unknown"] == 0
    assert sm["dialogue_messages_extracted"] == 98
    assert sm["world_areas_extracted"] == 21
    assert sm["village_elder_message_id"] == 5
    assert sm["village_elder_script_addr"] == "0x0466B2"
    assert sm["village_elder_text_addr"] == "0x05178C"

    # 1. Verify all 31 opcodes have authentic jump table offsets and valid handlers
    opcodes = data["opcodes"]
    assert len(opcodes) == 31
    expected_handlers = {
        "0x00": "0x00F91C", "0x01": "0x00FCCE", "0x02": "0x00F920", "0x03": "0x00F942",
        "0x04": "0x00F96A", "0x05": "0x00FAAC", "0x06": "0x00FC6C", "0x07": "0x00FC94",
        "0x08": "0x00FCA6", "0x09": "0x010034", "0x0A": "0x00FCB8", "0x0B": "0x00FCC4",
        "0x0C": "0x010086", "0x0D": "0x0100AA", "0x0E": "0x00FD8C", "0x0F": "0x010140",
        "0x10": "0x010170", "0x11": "0x0101A0", "0x12": "0x00FAB2", "0x13": "0x0101D0",
        "0x14": "0x0101E4", "0x15": "0x00FEDE", "0x16": "0x00FF1A", "0x17": "0x00FE0A",
        "0x18": "0x00FF98", "0x19": "0x00FFDE", "0x1A": "0x0100D6", "0x1B": "0x0100E4",
        "0x1C": "0x0100EE", "0x1D": "0x0100F8", "0x1E": "0x00F854"
    }
    for op_hex, exp_handler in expected_handlers.items():
        assert op_hex in opcodes, f"Missing opcode {op_hex}"
        assert opcodes[op_hex]["handler_addr"] == exp_handler, f"Handler mismatch for {op_hex}"
        assert opcodes[op_hex]["status"] in ("FULLY_DECODED", "PARTIALLY_DECODED")

    # 2. Verify Dialogue Resources
    dres = data["dialogue_resources"]
    assert dres["table_address"] == "0x051514"
    records = dres["records"]
    assert len(records) == 98

    # Verify Message 5 (Village Elder)
    msg5 = records[5]
    assert msg5["message_id"] == 5
    assert msg5["text_rom_addr"] == "0x05178C"
    assert msg5["page_count"] == 2
    assert "Prince Ali, are you all right?" in msg5["pages"][0][0]
    assert "They may have attacked the" in msg5["pages"][1][0]
    assert "castle as well." in msg5["pages"][1][1]

    # Verify Message 0 (Dytto water spirit)
    msg0 = records[0]
    assert msg0["text_rom_addr"] == "0x0515FC"
    assert "I am Dytto, a water spirit." in msg0["pages"][0][0]

    # Verify Message 97 (Final Story / Epilogue)
    msg97 = records[97]
    assert msg97["text_rom_addr"] == "0x0541AE"
    assert "I know you're very tough," in msg97["pages"][0][0]

    # 3. Verify Spawn Scripts
    ss = data["spawn_scripts"]
    assert ss["master_table_addr"] == "0x04371E"
    assert len(ss["areas"]) == 21
    # Check Area 1 contains Room 0x0108 script at 0x0466B2
    area1 = ss["areas"][1]
    room8_script = next(s for s in area1["scripts"] if s["room_id"] == "0x0108")
    assert room8_script["script_addr"] == "0x0466B2"

    # 4. Verify Village Elder Trace Semantics
    elder = data["village_elder_trace"]
    assert elder["room_id"] == "0x0108"
    assert elder["spawn_script_addr"] == "0x0466B2"
    assert elder["entity_type_id"] == "0x003C"
    assert len(elder["opcode_sequence"]) == 3
    assert elder["opcode_sequence"][0]["opcode"] == "0x0A"
    assert elder["opcode_sequence"][1]["opcode"] == "0x15"
    assert elder["opcode_sequence"][2]["opcode"] == "0x00"

    # Check Gating Flag 141 ($FF0DBC+17 bit 5)
    flag_params = elder["opcode_sequence"][1]["params"]
    assert flag_params["flag_index"] == 141
    assert flag_params["flag_byte_offset"] == 17
    assert flag_params["flag_bit_index"] == 5

    # Check Interaction Mechanics
    mech = elder["interaction_mechanics"]
    assert mech["entity_init_handler"] == "0x017D12 (Table 0x00DF54 - 0x2C + 0x3C*2)"
    assert mech["authentic_message_id"] == 5
    assert mech["message_text_addr"] == "0x05178C"
    assert mech["dialogue_engine_entry"] == "0x00C326"
    assert mech["dialogue_frame_step_loop"] == "0x00C84E (Maintains animation while waiting input)"
    assert mech["button_wait_routine"] == "0x00C536 (Tests bits 4,5,6 of $FF165F)"

    # Check GPGX Status is Honest
    assert elder["gpgx_trace_status"]["reached_in_existing_fixtures"] is False
    assert len(elder["gpgx_trace_status"]["reproducible_capture_procedure"]) > 20

    print("ROM_EVENT_SCRIPT_VM_TEST: ALL ASSERTIONS PASSED (31 opcodes, 98 dialogue messages, 21 areas, Village Elder verified)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
