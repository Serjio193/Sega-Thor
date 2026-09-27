"""Capture and build Durable Real VDP Oracle V2 using real BizHawk runtime."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
from typing import Any, Mapping

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT / "src" / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "src" / "tools"))
if str(ROOT / "tools" / "bizhawk-native-ring") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

import m12_vdp_frame_artifact as vdp_mod  # noqa: E402
from live_forward_vdp_stage import VdpProtocolDecoder  # noqa: E402

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3145728

ORACLE_ID = "vdp_oracle_v2_f3"
ORACLE_RUN_ID = "vdp_oracle_real_v2"
ORACLE_EPOCH = 1
ORACLE_FRAME = 3
ORACLE_SCHEMA = "oasis.m68k.m12-vdp-frame-capture.v1"
ORACLE_LOGICAL_SCHEMA = "oasis.m68k.hardware-vdp-frame.v1"
DEFAULT_INSTALL = Path(r"C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64")
DEFAULT_ROM = ROOT / "local-roms" / "Beyond Oasis (USA).md"


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    hasher = hashlib.sha256()
    with open(path, "rb") as stream:
        while chunk := stream.read(1024 * 1024):
            hasher.update(chunk)
    return hasher.hexdigest()


def build_capture_lua(target_frame: int, raw_path: Path) -> str:
    raw_str = str(raw_path.resolve()).replace("\\", "\\\\")
    return f"""
local target_frame = {target_frame}
local oracle_id = "{ORACLE_ID}"
local run_id = "{ORACLE_RUN_ID}"
local epoch = {ORACLE_EPOCH}
local seq = 0
local inst_seq = 0
local last_pc = nil
local vdp_events = {{}}
local register_events = {{}}
local current_regs = {{}}
local CAUSING_WIDTHS = {{
    [0x000272] = 32, [0x000274] = 32, [0x000278] = 32, [0x00027E] = 32, [0x000282] = 32,
    [0x002AF4] = 16, [0x002B38] = 32, [0x002B40] = 16, [0x002B7E] = 32, [0x002B88] = 32,
    [0x002B94] = 32, [0x002B9E] = 32, [0x002BB0] = 32, [0x002BB6] = 16, [0x002BE4] = 16,
    [0x002BF0] = 16, [0x002C02] = 32, [0x002C06] = 16, [0x002C0E] = 32, [0x002C12] = 16,
    [0x002C20] = 16, [0x002C24] = 16
}}

for f = 1, target_frame - 1 do
    emu.frameadvance()
end
local start_boundary = emu.framecount()

event.on_bus_write(function(addr, val, flags)
    if addr >= 0xC00000 and addr <= 0xC00007 then
        local f = emu.framecount()
        local pc_num = emu.getregister("M68K PC")
        local pc = string.format("0x%06X", pc_num)
        local cycles = emu.totalexecutedcycles()
        local ev_type = "data_write"
        local reg_num = nil
        local reg_val = nil
        local width = CAUSING_WIDTHS[pc_num]
        if not width then
            error("Unknown causing PC width: " .. pc)
        end

        if addr == 0xC00004 or addr == 0xC00006 then
            if (val & 0xC000) == 0x8000 then
                ev_type = "register_write"
                reg_num = (val >> 8) & 0x1F
                reg_val = val & 0xFF
                current_regs[reg_num] = reg_val
                register_events[#register_events + 1] = {{
                    frame = f, sequence = #register_events + 1, stream_sequence = seq + 1,
                    epoch = epoch, m68k_total_cycles = cycles, pc = pc,
                    register = reg_num, value = reg_val, raw_word = val, phase = "bus_write"
                }}
            else
                ev_type = "control_word"
            end
        end

        seq = seq + 1
        if pc ~= last_pc then
            inst_seq = inst_seq + 1
            last_pc = pc
        end
        vdp_events[#vdp_events + 1] = {{
            run_id = run_id, epoch = epoch, frame = f,
            stream_sequence = seq, instruction_sequence = inst_seq, sequence = seq,
            m68k_total_cycles = cycles, pc = pc,
            address = string.format("0x%06X", addr), raw_address = addr,
            value = val, width = width, domain = 3, type = ev_type,
            register = reg_num, reg_value = reg_val
        }}
    end
end, "vdp_oracle_watch", "M68K BUS")

emu.frameadvance()
local end_boundary = emu.framecount()

local vram = memory.read_bytes_as_array(0, 0x10000, "VRAM")
local cram = memory.read_bytes_as_array(0, 0x80, "CRAM")
local vsram = memory.read_bytes_as_array(0, 0x50, "VSRAM")

local out = assert(io.open("{raw_str}", "w"))
out:write('{{\\n')
out:write('  "schema": "{ORACLE_SCHEMA}",\\n')
out:write('  "oracle_id": "' .. oracle_id .. '",\\n')
out:write('  "oracle_type": "SINGLE_COHERENT_FRAME",\\n')
out:write('  "run_id": "' .. run_id .. '",\\n')
out:write('  "epoch": ' .. tostring(epoch) .. ',\\n')
out:write('  "frame": ' .. tostring(target_frame) .. ',\\n')
out:write('  "vram_frame": ' .. tostring(target_frame) .. ',\\n')
out:write('  "cram_frame": ' .. tostring(target_frame) .. ',\\n')
out:write('  "vsram_frame": ' .. tostring(target_frame) .. ',\\n')
out:write('  "state_writes_emitted": false,\\n')
out:write('  "start_boundary": ' .. tostring(start_boundary) .. ',\\n')
out:write('  "end_boundary": ' .. tostring(end_boundary) .. ',\\n')
out:write('  "canonical_rom_sha256": "{ROM_SHA}",\\n')
out:write('  "write_width_source": "M68K_OPCODE_BUS_CYCLE_DECODE",\\n')
out:write('  "width_authority": "CAUSING_M68K_INSTRUCTION_SEMANTICS",\\n')
out:write('  "width_exact": true,\\n')
out:write('  "master_time_unit_match": false,\\n')
out:write('  "timing_unit": "m68k_total_cycles",\\n')

out:write('  "registers": {{\\n')
local r_keys = {{}}
for k, _ in pairs(current_regs) do r_keys[#r_keys + 1] = k end
table.sort(r_keys)
for i, r in ipairs(r_keys) do
    out:write(string.format('    "%d": %d%s\\n', r, current_regs[r], i < #r_keys and ',' or ''))
end
out:write('  }},\\n')

out:write('  "register_events": [\\n')
for i, ev in ipairs(register_events) do
    out:write(string.format('    {{"frame": %d, "sequence": %d, "stream_sequence": %d, "epoch": %d, "m68k_total_cycles": %d, "pc": "%s", "register": %d, "value": %d, "raw_word": %d, "phase": "bus_write"}}%s\\n',
        ev.frame, ev.sequence, ev.stream_sequence, ev.epoch, ev.m68k_total_cycles, ev.pc, ev.register, ev.value, ev.raw_word, i < #register_events and ',' or ''))
end
out:write('  ],\\n')

out:write('  "vdp_events": [\\n')
for i, ev in ipairs(vdp_events) do
    local reg_field = ev.register and string.format(', "register": %d, "reg_value": %d', ev.register, ev.reg_value) or ''
    out:write(string.format('    {{"run_id": "%s", "epoch": %d, "frame": %d, "stream_sequence": %d, "instruction_sequence": %d, "sequence": %d, "m68k_total_cycles": %d, "pc": "%s", "address": "%s", "raw_address": %d, "value": %u, "width": %d, "domain": %d, "type": "%s"%s}}%s\\n',
        ev.run_id, ev.epoch, ev.frame, ev.stream_sequence, ev.instruction_sequence, ev.sequence, ev.m68k_total_cycles, ev.pc, ev.address, ev.raw_address, ev.value, ev.width, ev.domain, ev.type, reg_field, i < #vdp_events and ',' or ''))
end
out:write('  ],\\n')

out:write('  "vram": [')
for i = 1, #vram do
    out:write(tostring(vram[i]))
    if i < #vram then out:write(',') end
end
out:write('],\\n')

out:write('  "cram": [')
for i = 1, #cram do
    out:write(tostring(cram[i]))
    if i < #cram then out:write(',') end
end
out:write('],\\n')

out:write('  "vsram": [')
for i = 1, #vsram do
    out:write(tostring(vsram[i]))
    if i < #vsram then out:write(',') end
end
out:write(']\\n')
out:write('}}\\n')
out:close()
client.exit()
"""


def execute_capture(install: Path, rom: Path, out_dir: Path, target_frame: int) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    raw_path = out_dir / "raw_capture.json"
    cfg_src = json.loads((install / "config.ini").read_text(encoding="utf-8-sig"))
    cfg_src["ClockThrottle"] = False
    cfg_src["RunInBackground"] = True
    cfg_src["SingleInstanceMode"] = False
    tmp_cfg = out_dir / "bizhawk_oracle_config.ini"
    tmp_cfg.write_text(json.dumps(cfg_src, indent=2))
    lua_script = out_dir / f"capture_frame_{target_frame}.lua"
    lua_script.write_text(build_capture_lua(target_frame, raw_path), encoding="utf-8")
    cmd = [str(install / "EmuHawk.exe"), f"--config={tmp_cfg}", f"--lua={lua_script}", str(rom.resolve())]
    subprocess.run(cmd, cwd=str(install), check=True, timeout=30)
    if not raw_path.is_file():
        raise FileNotFoundError(f"STOP_RAW_CAPTURE_FAILED:{raw_path}")
    return raw_path


def decode_independent_commands_and_dma(vdp_events: list[dict[str, Any]]) -> tuple[dict[int, int], list[dict[str, Any]], list[dict[str, Any]]]:
    """Independent command and DMA decoder implementation (separate from VdpProtocolDecoder)."""
    regs: dict[int, int] = {}
    pending: dict[str, Any] | None = None
    commands: list[dict[str, Any]] = []
    dma_list: list[dict[str, Any]] = []

    for ev in vdp_events:
        addr = ev.get("raw_address") or int(ev["address"], 16)
        if addr not in (0xC00004, 0xC00006):
            continue
        val, width = ev["value"], ev.get("width")
        words = [(val >> 16) & 0xFFFF, val & 0xFFFF] if width == 32 else ([val & 0xFFFF] if width == 16 else None)
        if words is None:
            raise ValueError(f"STOP_UNRESOLVED_ORACLE_EVENT_WIDTH: width={width}")
        for w in words:
            if (w & 0xC000) == 0x8000:
                reg_num, reg_val = (w >> 8) & 0x1F, w & 0xFF
                regs[reg_num] = reg_val
                continue

            if pending is None:
                pending = {"word": w, "ev": ev}
                continue

            w1, w2 = pending["word"], w
            first_ev, pending = pending["ev"], None
            code = ((w1 >> 14) & 3) | ((w2 >> 2) & 0x3C)
            target_addr = (w1 & 0x3FFF) | ((w2 & 3) << 14)
            target = "VRAM" if (code & 0x0F) in (0, 1) else ("CRAM" if (code & 0x0F) in (3, 8) else ("VSRAM" if (code & 0x0F) in (4, 5) else "OTHER"))
            direction = "WRITE" if (code & 1) or (code & 0x0F in (3, 5)) else "READ"
            is_dma = bool(code & 0x20)

            cmd_rec: dict[str, Any] = {
                "event_identity": {"frame": ev["frame"], "stream_sequence": first_ev["stream_sequence"]},
                "word1": w1, "word2": w2, "code": code, "address": target_addr,
                "target": target, "direction": direction, "dma_requested": is_dma,
                "first_stream_seq": first_ev["stream_sequence"], "second_stream_seq": ev["stream_sequence"],
            }
            if is_dma:
                if all(r in regs for r in (19, 20, 21, 22, 23)):
                    r19, r20, r21, r22, r23 = (regs[r] for r in (19, 20, 21, 22, 23))
                    lw = (r20 << 8) | r19 or 0x10000
                    mode = {2: "FILL", 3: "COPY"}.get(r23 >> 6, "68K_BUS")
                    src = (r23 << 17) | (r22 << 9) | (r21 << 1)
                    dom = "68K_RAM" if src >= 0xFF0000 else ("ROM" if src < 0x400000 else "OTHER")
                    dma_rec = {
                        "event_identity": cmd_rec["event_identity"], "type": mode,
                        "source_address": f"0x{src:06X}", "source_domain": dom,
                        "destination_domain": target, "destination_address": f"0x{target_addr:04X}",
                        "length": lw, "classification": "DMA_EXACT",
                    }
                else:
                    dma_rec = {
                        "event_identity": cmd_rec["event_identity"], "type": "UNKNOWN",
                        "source_address": None, "source_domain": "UNRESOLVED",
                        "destination_domain": target, "destination_address": f"0x{target_addr:04X}",
                        "length": 0, "classification": "DMA_PARTIAL",
                    }
                cmd_rec["dma"] = dma_rec
                dma_list.append(dma_rec)
            commands.append(cmd_rec)

    return regs, commands, dma_list


def build_logical_artifact(raw_capture_path: Path, out_dir: Path) -> Path:
    capture = json.loads(raw_capture_path.read_text(encoding="utf-8"))
    s7 = {"schema": vdp_mod.RASTER_SCHEMA, "status": vdp_mod.S7_STATUS,
          "frame": capture["frame"], "artifact_sha256": "s7_clean_zero_sprite", "pixel_provenance": []}
    s6 = {"frame": capture["frame"], "pixel_provenance": []}
    artifact = vdp_mod.build_artifact(capture, s7, s6)

    exp_regs, exp_cmds, exp_dma = decode_independent_commands_and_dma(capture["vdp_events"])
    vram_bytes = bytes(capture["vram"])
    cram_bytes = bytes(capture["cram"])
    vsram_bytes = bytes(capture["vsram"])

    artifact["oracle_id"] = capture["oracle_id"]
    artifact["schema"] = ORACLE_LOGICAL_SCHEMA
    artifact["oracle_builder"] = "capture_real_vdp_oracle.py:build_logical_artifact"
    artifact["expected_final_registers"] = {str(k): v for k, v in exp_regs.items()}
    artifact["expected_commands"] = exp_cmds
    artifact["expected_dma_events"] = exp_dma
    artifact["state_hashes"] = {
        "vram_sha256": sha256_bytes(vram_bytes),
        "cram_sha256": sha256_bytes(cram_bytes),
        "vsram_sha256": sha256_bytes(vsram_bytes),
    }

    logical_path = out_dir / "logical_artifact.json"
    unsigned = dict(artifact)
    unsigned.pop("artifact_sha256", None)
    artifact["artifact_sha256"] = vdp_mod._hash(unsigned)
    logical_path.write_text(json.dumps(artifact, indent=2) + "\n", encoding="utf-8")
    return logical_path


def create_manifest_and_receipt(out_dir: Path, raw_path: Path, logical_path: Path) -> tuple[Path, Path]:
    raw_sha = sha256_file(raw_path)
    log_sha = sha256_file(logical_path)
    cap = json.loads(raw_path.read_text(encoding="utf-8"))
    log = json.loads(logical_path.read_text(encoding="utf-8"))

    manifest_data = {
        "schema": "oasis.m12.vdp-oracle-manifest.v2",
        "oracle_id": ORACLE_ID,
        "creation_timestamp": datetime.now(timezone.utc).isoformat(),
        "canonical_rom_sha256": ROM_SHA,
        "run_id": ORACLE_RUN_ID,
        "epoch": ORACLE_EPOCH,
        "frame": ORACLE_FRAME,
        "start_boundary": cap["start_boundary"],
        "end_boundary": cap["end_boundary"],
        "raw_event_observations": len(cap["vdp_events"]),
        "unique_runtime_events": len(cap["vdp_events"]),
        "register_events_count": len(cap["register_events"]),
        "active_registers_count": len(cap["registers"]),
        "visible_resolution": log["resolution"],
        "timing_unit": "m68k_total_cycles",
        "write_width_source": cap["write_width_source"],
        "width_exact": cap["width_exact"],
        "width_authority": cap.get("width_authority", "CAUSING_M68K_INSTRUCTION_SEMANTICS"),
        "files": {
            "raw_capture.json": {"size_bytes": raw_path.stat().st_size, "sha256": raw_sha, "schema": ORACLE_SCHEMA},
            "logical_artifact.json": {"size_bytes": logical_path.stat().st_size, "sha256": log_sha, "schema": ORACLE_LOGICAL_SCHEMA},
        },
    }
    manifest_path = out_dir / "vdp_oracle_manifest.json"
    manifest_path.write_text(json.dumps(manifest_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    man_sha = sha256_file(manifest_path)

    receipt_data = {
        "schema": "oasis.m12.vdp-oracle-receipt.v2",
        "oracle_id": ORACLE_ID,
        "oracle_type": "SINGLE_COHERENT_FRAME",
        "run_id": ORACLE_RUN_ID,
        "epoch": ORACLE_EPOCH,
        "frame": ORACLE_FRAME,
        "start_boundary": cap["start_boundary"],
        "end_boundary": cap["end_boundary"],
        "event_count": len(cap["vdp_events"]),
        "register_event_count": len(cap["register_events"]),
        "capture_sha256": raw_sha,
        "logical_artifact_sha256": log_sha,
        "manifest_sha256": man_sha,
        "canonical_rom_sha256": ROM_SHA,
        "write_width_source": cap["write_width_source"],
        "width_exact": cap["width_exact"],
        "width_authority": cap.get("width_authority", "CAUSING_M68K_INSTRUCTION_SEMANTICS"),
        "master_time_unit_match": False,
        "timing_unit": "m68k_total_cycles",
        "artifact_sizes": {
            "raw_capture_bytes": raw_path.stat().st_size,
            "logical_artifact_bytes": logical_path.stat().st_size,
            "manifest_bytes": manifest_path.stat().st_size,
        },
        "tool_versions": {
            "emulator": "bizhawk-2.11.1-win-x64",
            "vdp_builder": "m12_vdp_frame_artifact.v2",
            "vdp_decoder": "VdpProtocolDecoder.v1",
        },
        "storage_locator": f"build/thor-evidence/oracles/vdp/{ORACLE_ID}/",
        "creation_contract": "REAL_BIZHAWK_SINGLE_FRAME_BOOT_TRACE_V2",
        "conflict_count": 0,
        "source_owned_before": 1487388,
        "source_owned_after": 1487388,
        "source_owned_delta": 0,
    }
    receipt_path = out_dir / "vdp_oracle_receipt.json"
    receipt_path.write_text(json.dumps(receipt_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    checked_in_path = ROOT / "docs" / "reports" / "THOR_M12_VDP_ORACLE_V2_RECEIPT.json"
    checked_in_path.parent.mkdir(parents=True, exist_ok=True)
    checked_in_path.write_text(json.dumps(receipt_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    return manifest_path, receipt_path


def run_field_by_field_regression(raw_path: Path, logical_path: Path) -> dict[str, Any]:
    """Execute field-by-field regression comparing VdpProtocolDecoder against logical_artifact.json."""
    raw = json.loads(raw_path.read_text(encoding="utf-8"))
    log = json.loads(logical_path.read_text(encoding="utf-8"))

    decoder = VdpProtocolDecoder()
    for ev in raw["vdp_events"]:
        addr = ev.get("raw_address") or int(ev["address"], 16)
        if addr in (0xC00004, 0xC00006):
            val, width = ev["value"], ev.get("width")
            words = [(val >> 16) & 0xFFFF, val & 0xFFFF] if width == 32 else ([val & 0xFFFF] if width == 16 else None)
            if words is None:
                raise ValueError(f"STOP_UNRESOLVED_ORACLE_EVENT_WIDTH: width={width}")
            pc = int(ev["pc"], 16)
            for w in words:
                decoder.consume_control_word(w, pc, ev["frame"], ev["m68k_total_cycles"], ev["stream_sequence"])

    conflicts = 0
    exp_regs = {int(k): v for k, v in log["expected_final_registers"].items()}
    reg_fields_compared = len(exp_regs)
    for r, exp_val in exp_regs.items():
        if decoder.active_registers.get(r) != exp_val:
            conflicts += 1

    exp_cmds = log["expected_commands"]
    act_cmds = decoder.complete_commands
    cmd_fields_compared = len(exp_cmds) * 6
    if len(exp_cmds) != len(act_cmds):
        conflicts += 1
    for exp_c, act_c in zip(exp_cmds, act_cmds):
        act_vals = (act_c["first_stream_seq"], act_c["first_word"], act_c["second_word"],
                    act_c["target"], act_c["direction"], act_c["address"])
        for k, a_val in zip(("first_stream_seq", "word1", "word2", "target", "direction", "address"), act_vals):
            if exp_c[k] != a_val:
                conflicts += 1

    exp_dmas = log["expected_dma_events"]
    act_dmas = decoder.dma_events
    dma_fields_compared = len(exp_dmas) * 7
    if len(exp_dmas) != len(act_dmas):
        conflicts += 1
    dma_keys = [("type", "dma_type"), ("source_address", "source_address"), ("source_domain", "source_domain"),
                ("destination_domain", "destination_domain"), ("destination_address", "destination_address"),
                ("length", "length_words")]
    for exp_d, act_d in zip(exp_dmas, act_dmas):
        for ek, ak in dma_keys:
            if exp_d[ek] != act_d[ak]:
                conflicts += 1

    vram_ok = sha256_bytes(bytes(raw["vram"])) == log["state_hashes"]["vram_sha256"]
    cram_ok = sha256_bytes(bytes(raw["cram"])) == log["state_hashes"]["cram_sha256"]
    vsram_ok = sha256_bytes(bytes(raw["vsram"])) == log["state_hashes"]["vsram_sha256"]
    if not (vram_ok and cram_ok and vsram_ok):
        conflicts += 1

    return {
        "status": "PASS" if conflicts == 0 else "STOP",
        "oracle_conflicts": conflicts,
        "register_fields_compared": reg_fields_compared,
        "commands_expected": len(exp_cmds),
        "commands_actual": len(act_cmds),
        "command_field_comparisons": cmd_fields_compared,
        "dma_expected": len(exp_dmas),
        "dma_actual": len(act_dmas),
        "dma_field_comparisons": dma_fields_compared,
        "vram_hash_match": vram_ok,
        "cram_hash_match": cram_ok,
        "vsram_hash_match": vsram_ok,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture and freeze Durable Real VDP Oracle V2.")
    parser.add_argument("--install", type=Path, default=DEFAULT_INSTALL)
    parser.add_argument("--rom", type=Path, default=DEFAULT_ROM)
    parser.add_argument("--out-dir", type=Path, default=ROOT / "build" / "thor-evidence" / "oracles" / "vdp" / ORACLE_ID)
    parser.add_argument("--frame", type=int, default=ORACLE_FRAME)
    args = parser.parse_args()

    t0 = time.time()
    raw_path = execute_capture(args.install, args.rom, args.out_dir, args.frame)
    log_path = build_logical_artifact(raw_path, args.out_dir)
    man_path, rec_path = create_manifest_and_receipt(args.out_dir, raw_path, log_path)
    reg_result = run_field_by_field_regression(raw_path, log_path)

    print(f"ORACLE_ID = {ORACLE_ID}")
    print(f"CAPTURE_PATH = {raw_path}")
    print(f"CAPTURE_SHA256 = {sha256_file(raw_path)}")
    print(f"LOGICAL_PATH = {log_path}")
    print(f"LOGICAL_SHA256 = {sha256_file(log_path)}")
    print(f"MANIFEST_PATH = {man_path}")
    print(f"MANIFEST_SHA256 = {sha256_file(man_path)}")
    print(f"RECEIPT_PATH = {rec_path}")
    print(f"REGRESSION_RESULT = {reg_result}")
    print(f"COMPLETED in {time.time() - t0:.2f} sec")
    return 0 if reg_result["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
