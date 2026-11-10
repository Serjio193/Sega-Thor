#!/usr/bin/env python3
"""
M15.247 Phase 10: Deterministic Extended 1,800-Frame Alternate Journal (Town/Village).
Preserves the pinned Scenario A 1,592-frame journal (frames 0..1591) verbatim as prefix
and appends a fresh scripted 208-frame active gameplay schedule (frames 1592..1799)
exploring town movement, NPC interaction, sword sweeps, and jumps under pinned GPGX.
Strictly <= 180 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter

QUERY_ORDER = [
    (0, 1, 0, 10), (0, 1, 0, 9), (0, 1, 0, 11), (0, 1, 0, 2), (0, 1, 0, 1), (0, 1, 0, 0),
    (0, 1, 0, 8), (0, 1, 0, 3), (0, 1, 0, 4), (0, 1, 0, 5), (0, 1, 0, 6), (0, 1, 0, 7),
    (1, 1, 0, 10), (1, 1, 0, 9), (1, 1, 0, 11), (1, 1, 0, 2), (1, 1, 0, 1), (1, 1, 0, 0),
    (1, 1, 0, 8), (1, 1, 0, 3), (1, 1, 0, 4), (1, 1, 0, 5), (1, 1, 0, 6), (1, 1, 0, 7),
]

BIT_B = 1 << 0
BIT_C = 1 << 1
BIT_START = 1 << 3
BIT_UP = 1 << 4
BIT_DOWN = 1 << 5
BIT_LEFT = 1 << 6
BIT_RIGHT = 1 << 7
BIT_A = 1 << 8
BIT_X = 1 << 9


def scripted_alt_mask(f: int) -> int:
    rel = f - 1592
    phase = (rel // 26) % 8
    sub = rel % 26
    if phase == 0:
        return BIT_UP | (BIT_B if sub % 8 < 2 else 0)
    elif phase == 1:
        return BIT_UP | BIT_LEFT | (BIT_X if sub == 5 else 0)
    elif phase == 2:
        return BIT_LEFT | (BIT_B if sub % 6 < 2 else 0)
    elif phase == 3:
        return BIT_DOWN | (BIT_A if sub == 10 else 0)
    elif phase == 4:
        return BIT_RIGHT | (BIT_B if sub % 8 < 2 else 0)
    elif phase == 5:
        return BIT_DOWN | BIT_RIGHT | (BIT_X if sub == 6 else 0)
    elif phase == 6:
        return BIT_UP | (BIT_B if sub % 8 < 2 else 0)
    else:
        return BIT_A if sub % 10 < 3 else 0


def format_poll_line(frame: int, mask: int, label: str) -> str:
    parts = [f"P {frame} 0 {mask:04x} {label} 24"]
    for p, d, i, btn in QUERY_ORDER:
        val = 0
        if p == 0:
            if btn == 0 and (mask & BIT_B): val = 1
            elif btn == 1 and (mask & BIT_C): val = 1
            elif btn == 8 and (mask & BIT_A): val = 1
            elif btn == 9 and (mask & BIT_X): val = 1
            elif btn == 3 and (mask & BIT_START): val = 1
            elif btn == 4 and (mask & BIT_UP): val = 1
            elif btn == 5 and (mask & BIT_DOWN): val = 1
            elif btn == 6 and (mask & BIT_LEFT): val = 1
            elif btn == 7 and (mask & BIT_RIGHT): val = 1
        parts.append(f"{p}:{d}:{i}:{btn}={val}")
    return " ".join(parts)


def to_wsl(p: pathlib.Path) -> str:
    r = p.resolve()
    drive = r.drive.replace(":", "").lower()
    posix = r.as_posix()[len(r.drive):] if r.drive else r.as_posix()
    return f"/mnt/{drive}{posix}"


def sha(p: pathlib.Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def write_manifest(d: pathlib.Path, runner: pathlib.Path, core: pathlib.Path, rom: pathlib.Path) -> None:
    cfg_str = (
        "cold_retro_load_game;frames=1800;input=scripted;poll=retro_input_poll+ordered_retro_input_state;"
        "landmark=60;watch=E376,9C40,E37A,E396,94A2,8EF8,B730,16A46;call=E376:9C40:E37A:E396;"
        "rts=9C7A;video=discard;audio=discard;sram=isolated-per-run"
    )
    lines = [
        "OASIS_GAMEPLAY_SESSION 1",
        "auto_full_calls_sha256=0000000000000000000000000000000000000000000000000000000000000000",
        "call_census=M68K_DIRECT_BSR_JSR_ABS_JSR_AN_V1",
        "call_census_sha256=0000000000000000000000000000000000000000000000000000000000000000",
        "call_watch=0xE376:0x9C40:0xE37A:0xE396;RTS=0x9C7A",
        "calls_sha256=0000000000000000000000000000000000000000000000000000000000000000",
        f"config={cfg_str}",
        f"config_sha256={hashlib.sha256(cfg_str.encode('utf-8')).hexdigest()}",
        f"core_path={to_wsl(core)}",
        f"core_sha256={sha(core)}",
        "cross_cpu_landmark=UNAVAILABLE_M68K_HOOK_ONLY",
        "frames=1800",
        f"initial_state_sha256={sha(d / 'initial.state')}",
        f"initial_state_size={(d / 'initial.state').stat().st_size}",
        "input_mode=scripted",
        "input_poll_count=1800",
        f"input_sha256={sha(d / 'input.polls')}",
        "landmark_period=60",
        f"landmarks_sha256={sha(d / 'landmarks.jsonl')}",
        "reset_method=retro_load_game_cold_start",
        f"rom_sha256={sha(rom)}",
        f"rom_size={rom.stat().st_size}",
        f"runner_path={to_wsl(runner)}",
        f"runner_sha256={sha(runner)}",
        "target_watch=0xE376,0x9C40,0x94A2,0x8EF8,0xB730,0x16A46",
    ]
    (d / "session.manifest").write_bytes(("\n".join(lines) + "\n").encode("utf-8"))


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    a_polls = root / "build/ab_portable_m15_244/scenario_a/run_p0/input.polls"
    out_dir = root / "build/m15_247/longrun_alt_1800f"
    out_dir.mkdir(parents=True, exist_ok=True)

    a_lines = a_polls.read_text(encoding="utf-8").splitlines()
    assert a_lines[0] == "OASIS_INPUT_POLL_LOG 1" and len(a_lines) == 1593
    ext_lines = [format_poll_line(f, scripted_alt_mask(f), "INJECTED_SCRIPTED") for f in range(1592, 1800)]
    out_polls = out_dir / "input.polls"
    out_polls.write_bytes(("\n".join(a_lines + ext_lines) + "\n").encode("utf-8"))

    shutil.copyfile(root / "build/checkpoints/canonical_frame2008.state", out_dir / "initial.state")
    runner = root / "build/auto-migrator/m15_247/linux-runner/oasis_gameplay_session"
    core = root / "build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so"
    rom = root / "local-roms/Beyond Oasis (USA).md"

    lm = out_dir / "landmarks.jsonl"
    if not lm.exists() or len(lm.read_text(encoding="utf-8").splitlines()) != 1800:
        if (out_dir / "replay-run").exists():
            shutil.rmtree(out_dir / "replay-run")
        lm.write_text("", encoding="utf-8")
        write_manifest(out_dir, runner, core, rom)
        prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
        cmd = ["wsl", to_wsl(runner), "replay", to_wsl(core), to_wsl(rom), to_wsl(out_dir),
               "--normalized-rom", "--restore-initial-state", f"--native-island=step:{prod_csv}"]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        if not (out_dir / "replay-run" / "observed-landmarks.jsonl").exists():
            raise RuntimeError(proc.stderr[-500:] + proc.stdout[-500:])
        shutil.copyfile(out_dir / "replay-run" / "observed-landmarks.jsonl", lm)
    write_manifest(out_dir, runner, core, rom)

    manifest = {
        "schema": "oasis.m15_247.longrun-alt-1800f-capture-manifest.v1",
        "milestone": "M15.247",
        "map_region": "TOWN_VILLAGE_EXTENDED",
        "initial_state_sha256": sha(out_dir / "initial.state"),
        "input_journal_sha256": sha(out_polls),
        "landmarks_sha256": sha(lm),
        "rom_sha256": sha(rom),
        "core_sha256": sha(core),
        "input_provenance": {"frames_0_to_1591": "PINNED_SCENARIO_A_PREFIX", "frames_1592_to_1799": "INJECTED_SCRIPTED"},
        "frame_count": 1800,
        "poll_count": 1800,
    }
    ev_out = root / "data/thor-evidence/longrun_alt_1800f_capture_manifest_m15_247.json"
    ev_out.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {out_dir} & {ev_out} (1800f, input_sha={manifest['input_journal_sha256'][:12]})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
