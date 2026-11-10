#!/usr/bin/env python3
"""
M15.248 Phase 4: Deterministic 1,800-Frame Combat Gameplay Input Journal.
Preserves the pinned Scenario B 600-frame journal (frames 0..599) verbatim as prefix
and appends a fresh scripted 1,200-frame combat gameplay schedule (frames 600..1799)
exercising active combat: knife slashes, jump attacks, crouch jabs, directional flurries,
and armlet special triggers under pinned GPGX.
Strictly <= 200 lines per AGENTS.md Rule 3.
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


def scripted_combat_mask(f: int) -> int:
    rel = f - 600
    phase = (rel // 30) % 12
    sub = rel % 30
    if phase == 0:  # Approach enemy: Right + B slash combo
        return BIT_RIGHT | (BIT_B if (sub % 6 < 2) else 0)
    elif phase == 1:  # Jump slash: X jump + B attack mid-air
        return (BIT_X if sub < 6 else 0) | (BIT_B if (6 <= sub < 14) else 0) | BIT_RIGHT
    elif phase == 2:  # Down-Right crouch + rapid stabbing
        return BIT_DOWN | BIT_RIGHT | (BIT_B if (sub % 4 < 2) else 0)
    elif phase == 3:  # Retreat Left + turn slash
        return BIT_LEFT | (BIT_B if sub == 12 else 0)
    elif phase == 4:  # Up-Right advance + special attack A
        return BIT_UP | BIT_RIGHT | (BIT_A if (8 <= sub < 14) else 0)
    elif phase == 5:  # High-speed knife combo: 3 slashes
        return (BIT_B if (sub % 5 < 2) else 0) | (BIT_RIGHT if sub < 15 else BIT_DOWN)
    elif phase == 6:  # Up evasive maneuver + jump
        return BIT_UP | (BIT_X if (5 <= sub < 10) else 0) | (BIT_B if sub >= 15 else 0)
    elif phase == 7:  # Downward thrust attack
        return BIT_DOWN | (BIT_B if (sub % 6 < 3) else 0)
    elif phase == 8:  # Leftward combat sweep
        return BIT_LEFT | (BIT_B if (sub % 5 < 2) else 0)
    elif phase == 9:  # Jump attack sequence
        return (BIT_X if sub < 5 else 0) | (BIT_B if (5 <= sub < 12) else 0) | BIT_LEFT
    elif phase == 10:  # Aggressive Right pursuit + heavy slash
        return BIT_RIGHT | (BIT_B if (sub % 4 < 2) else 0)
    else:  # Armlet magic burst + guard
        return (BIT_A if (sub % 12 < 4) else 0) | BIT_DOWN


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
    if sys.platform != "win32":
        return str(r)
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
    b_polls = root / "build/ab_portable_m15_244/scenario_b/run_p0/input.polls"
    b_state = root / "build/ab_portable_m15_244/scenario_b/run_p0/initial.state"
    out_dir = root / "build/m15_248/longrun_combat_1800f"
    out_dir.mkdir(parents=True, exist_ok=True)

    b_lines = b_polls.read_text(encoding="utf-8").splitlines()
    assert b_lines[0] == "OASIS_INPUT_POLL_LOG 1" and len(b_lines) == 601
    ext_lines = [format_poll_line(f, scripted_combat_mask(f), "INJECTED_SCRIPTED") for f in range(600, 1800)]
    out_polls = out_dir / "input.polls"
    out_polls.write_bytes(("\n".join(b_lines + ext_lines) + "\n").encode("utf-8"))

    shutil.copyfile(b_state, out_dir / "initial.state")
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
        exe = to_wsl(runner) if sys.platform == "win32" else str(runner)
        prefix = ["wsl"] if sys.platform == "win32" else []
        cmd = prefix + [exe, "replay", to_wsl(core), to_wsl(rom), to_wsl(out_dir),
               "--normalized-rom", "--restore-initial-state", f"--native-island=step:{prod_csv}"]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        if not (out_dir / "replay-run" / "observed-landmarks.jsonl").exists():
            raise RuntimeError(proc.stderr[-500:] + proc.stdout[-500:])
        shutil.copyfile(out_dir / "replay-run" / "observed-landmarks.jsonl", lm)
    write_manifest(out_dir, runner, core, rom)

    manifest = {
        "schema": "oasis.m15_248.longrun-combat-capture-manifest.v1",
        "milestone": "M15.248",
        "scenario_name": "longrun_combat_1800f",
        "map_region": "BEACH_COMBAT_ARENA_EXTENDED",
        "initial_state_sha256": sha(out_dir / "initial.state"),
        "input_journal_sha256": sha(out_polls),
        "landmarks_sha256": sha(lm),
        "rom_sha256": sha(rom),
        "core_sha256": sha(core),
        "input_provenance": {
            "frames_0_to_599": "PINNED_SCENARIO_B_PREFIX",
            "frames_600_to_1799": "INJECTED_SCRIPTED_COMBAT"
        },
        "frame_count": 1800,
        "poll_count": 1800,
    }
    ev_out = root / "data/thor-evidence/longrun_combat_capture_manifest_m15_248.json"
    ev_out.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {out_dir} & {ev_out} (1800f, input_sha={manifest['input_journal_sha256'][:12]})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
