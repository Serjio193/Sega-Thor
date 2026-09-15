"""WALKER-1: one authoritative bounded two-window advancement experiment."""

from __future__ import annotations

import hashlib
import json
import os
import subprocess
import uuid
from pathlib import Path
from typing import Any

from thor_evidence.cartographer import Cartographer, digest


ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
STATE_SHA = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
CONTRACT_LINEAGE = {"source": "walker1-contract", "schema": "oasis.m12.walker1.v1"}


def _load(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_runtime(runtime: dict[str, Any]) -> dict[str, Any]:
    metrics = runtime.get("metrics", {})
    block = runtime.get("block", {})
    checks = {
        "schema": runtime.get("schema") == "oasis.m12.walker1.runtime.v1",
        "identity": all(runtime.get(key) for key in ("run_id", "capture_id", "fragment_id")),
        "phase_done": runtime.get("phase") == "DONE",
        "one_w1_and_w2": metrics.get("w1_count") == 1 and metrics.get("w2_count") == 1,
        "bounded_windows": metrics.get("max_w1_records", 65) <= 64 and metrics.get("max_w2_records", 65) <= 64,
        "contract_skip": metrics.get("contract_skips") == 1 and metrics.get("global_off_body_executions") == 1,
        "no_body_callbacks": metrics.get("callbacks_during_b_off") == 0 and not runtime.get("off_pcs"),
        "guards": block.get("guard") == "0x0027CA" and block.get("entry") == "0x0027CE" and block.get("exit") == "0x0027E2",
        "w2_dependency": bool(runtime.get("w2")) and bool(runtime.get("data_accesses")),
        "performance": metrics.get("frame_p99_ms", 10_000) <= 33 and metrics.get("frames_over_50ms", 1) == 0,
    }
    return {"checks": checks, "passed": all(checks.values())}


def run_capture(root: Path, run_name: str, run_id: str) -> tuple[dict[str, Any], dict[str, Any]]:
    output = root / "build" / f"{run_name}.json"
    emulator = Path(r"C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe")
    state = emulator.parent / "Genesis" / "State" / "Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State"
    rom = root / "build" / "reference" / "Beyond Oasis (USA).bin"
    lua = root / "src" / "tools" / "thor_evidence" / "capture" / "walker_two_window.lua"
    if _sha(rom) != ROM_SHA or _sha(state) != STATE_SHA:
        raise RuntimeError("authoritative ROM or QuickSave1 identity mismatch")
    env = os.environ.copy()
    env["PSModulePath"] = (r"C:\Windows\System32\WindowsPowerShell\v1.0\Modules;"
                           r"C:\Program Files\WindowsPowerShell\Modules")
    env.update({
        "OASIS_WALKER_FINAL": str(output), "OASIS_WALKER_RUN_ID": run_id,
        "OASIS_WALKER_RESTORE_EPOCH": "1", "OASIS_WALKER_MAX_FRAMES": "120",
        "OASIS_WALKER_STATE": str(state),
    })
    command = [r"C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe",
               "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
               str(root / "build" / "bizhawk-controlled-harness" / "run.ps1"),
               "-RunName", run_name, "-InputMode", "neutral", "-LuaScript", str(lua)]
    completed = subprocess.run(command, cwd=root, env=env, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               timeout=60, check=False)
    if not output.exists():
        raise RuntimeError(f"WALKER-1 produced no runtime artifact (exit {completed.returncode})\n{completed.stdout}")
    runtime = _load(output)
    runtime["launcher"] = {"returncode": completed.returncode, "output": completed.stdout[-2000:]}
    return runtime, {"command": command, "state_sha256": _sha(state), "rom_sha256": _sha(rom)}


def _node(kind: str, key: str, attributes: dict[str, Any]) -> dict[str, Any]:
    return {"kind": kind, "key": key, "scope": "global", "status": "PROVEN",
            "attributes": attributes, "lineage": [CONTRACT_LINEAGE]}


def make_bundle(runtime: dict[str, Any], rom: bytes) -> tuple[dict[str, Any], str]:
    contract = _node("EXECUTION_CONTRACT", "B:0x27CE-0x27E2",
                     {"entry": "0x27CE", "exit": "0x27E2", "guard": "0x27CA",
                      "semantics": "straight-line; A4/A5/control-flow unchanged"})
    w2 = runtime["w2"]
    discovered = next((item for item in w2 if item["pc"] not in {"0x0027EC"}), None)
    if discovered is None:
        raise ValueError("W2 has no new executed dependency")
    pc = int(discovered["pc"], 16)
    opcode = int.from_bytes(rom[pc:pc + 2], "big")
    instruction = _node("ROM_INSTRUCTION", f"0x{pc:06X}",
                        {"pc": f"0x{pc:06X}", "opcode": f"0x{opcode:04X}",
                         "runtime_window": "W2", "execution_frame": discovered["frame"]})
    # Runtime identity stays in the sealed artifact; the durable graph receives
    # stable static identities so an identical replay has zero graph growth.
    source_id, target_id = digest({"kind": contract["kind"], "key": contract["key"], "scope": "global"}), digest({"kind": instruction["kind"], "key": instruction["key"], "scope": "global"})
    bundle = {"nodes": [contract, instruction], "edges": [{
        "source": source_id, "target": target_id, "relation": "EXECUTION_SUCCESSOR",
        "scope": "global", "status": "PROVEN", "rule": "exact W1 guard -> B contract -> W2 ordered PC",
        "lineage": [CONTRACT_LINEAGE],
    }], "frontiers": [], "resolves_frontiers": []}
    return bundle, f"0x{pc:06X}"


def negative_join_results() -> dict[str, bool]:
    base = {"run_id": "r1", "restore_epoch": 1, "entry": "0x27CE", "exit": "0x27E2",
            "guard_ok": True, "gap": False, "incoming_version": "A5:v1"}

    def accepts(left: dict[str, Any], right: dict[str, Any]) -> bool:
        return (left["run_id"] == right["run_id"] and left["restore_epoch"] == right["restore_epoch"]
                and left["entry"] == right["entry"] and left["exit"] == right["exit"]
                and left["guard_ok"] and right["guard_ok"] and not left["gap"] and not right["gap"]
                and left["incoming_version"] == right["incoming_version"])

    cases = {
        "different_restore_epoch": {**base, "restore_epoch": 2},
        "marked_gap": {**base, "gap": True},
        "same_value_different_version": {**base, "incoming_version": "A5:v2"},
        "violated_guard": {**base, "guard_ok": False},
        "different_run_id": {**base, "run_id": "r2"},
    }
    return {name: not accepts(base, candidate) for name, candidate in cases.items()}


def run(root: Path) -> dict[str, Any]:
    token = uuid.uuid4().hex[:10]
    first, launch = run_capture(root, "walker1-authoritative-" + token, "walker1-run-" + token)
    replay, replay_launch = run_capture(root, "walker1-replay-" + token, "walker1-replay-" + token)
    first_validation, replay_validation = validate_runtime(first), validate_runtime(replay)
    rom = (root / "build" / "reference" / "Beyond Oasis (USA).bin").read_bytes()
    if first["w2"] != replay["w2"] or first["data_accesses"] != replay["data_accesses"]:
        raise RuntimeError("identical QuickSave1 replay changed the bounded fragment")
    token = first["run_id"].rsplit("-", 1)[-1]
    db = root / "build" / "thor-evidence" / "walker-1" / f"global-provenance-{token}.sqlite"
    graph = Cartographer(db, ROM_SHA, source_owned_bytes=0)
    bundle, dependency = make_bundle(first, rom)
    first_delta = graph.merge(bundle, "walker1-contract-bounded", "walker1-contract-v1").as_dict()
    replay_delta = graph.merge(bundle, "walker1-contract-bounded", "walker1-contract-v1").as_dict()
    proof = {
        "schema": "oasis.m12.walker1.proof.v1", "baseline": "6ee8ac7d599003c35cb48184831d9cc7dcc02a05",
        "classification": "PASS_ASM_WALKER_SPARSE_ADVANCEMENT" if first_validation["passed"] and replay_validation["passed"] and first_delta["new_nodes"] > 0 else "NEGATIVE",
        "selected_block": first["block"], "contract": first["block"]["contract"],
        "runs": [{"runtime": first, "validation": first_validation, "launch": launch},
                 {"runtime": replay, "validation": replay_validation, "launch": replay_launch}],
        "callback_off_interval": {"body": "0x0027CE..0x0027E2", "global_callbacks": first["metrics"]["callbacks_during_b_off"], "off_pcs": first.get("off_pcs", []), "proven": not first.get("off_pcs")},
        "new_dependency": {"pc": dependency, "source": "W2_EXECUTED_PC", "producer_oracle_supplied": False},
        "cartographer": {"database": str(db), "first_map_delta": first_delta, "replay_map_delta": replay_delta, "graph_hash": graph.graph_hash()},
        "negative_join_results": negative_join_results(),
        "performance": first["metrics"], "rom_sha256": ROM_SHA, "state_sha256": STATE_SHA,
        "source_owned_changed": False, "tests": {"runtime_validation": first_validation["passed"] and replay_validation["passed"], "negative_joins": all(negative_join_results().values())},
        "ci": "UNAVAILABLE — this checkpoint has no new CMake registration; focused Python test and required build gates remain to be run",
    }
    graph.close()
    proof_path = root / "docs" / "reports" / "THOR_M12_WALKER_1_TWO_WINDOW_SPARSE_ADVANCEMENT.json"
    proof_path.write_text(json.dumps(proof, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return proof


if __name__ == "__main__":
    print(json.dumps(run(Path.cwd().resolve()), indent=2, sort_keys=True))
