import json
from pathlib import Path

def main():
    with open('local-roms/Beyond Oasis (USA).md', 'rb') as f:
        rom = f.read()

    with open('data/thor-auto-migrator/native-ownership-index.json') as f:
        ownership = json.load(f)

    helper_ranges = [(r['start'], r['end'], r['owner_helper']) for r in ownership.get('ranges', []) if r.get('active', False)]
    exp_ranges = [
        (0x02575C, 0x0258DC, 'Slot 222 (Entity Cluster Coordinator)'),
        (0x00A9A0, 0x00AA40, 'Slot 223 (Sprite/Weapon Mode 0 Engine)'),
        (0x00AA42, 0x00AA90, 'Slot 224 (Sprite/Weapon Mode 1 Engine)'),
        (0x008F22, 0x009000, 'Slot 225 (Entity Motion Engine, shadowed)'),
        (0x002FDA, 0x00302E, 'Slot 226 (Palette Scaling Routine)'),
        (0x002512, 0x002630, 'Slot 227 (Tilemap Row Upload)'),
        (0x0023CC, 0x002510, 'Slot 228 (Tilemap Column Upload)'),
        (0x00D168, 0x00D264, 'Slot 229 (Tilemap Camera Range Check)'),
        (0x00D266, 0x00D2B2, 'Slot 230 (Camera Coordinate Normalizer)'),
        (0x062732, 0x062758, 'Slot 231 (Sound Pitch Mod)'),
        (0x062722, 0x062730, 'Slot 232 (Sound Envelope Reset)'),
    ]

    with open('build/codemap_scenario_c/code-map.json') as f:
        codemap = json.load(f)
    inst_counts = {int(i['cpu_pc'], 16): i.get('execution_count', 0) for i in codemap.get('instructions', [])}

    # Inspect candidate 0x005660
    candidates = [
        ("0x005660", 0x005660, 0x005670, "Palette Color Math Component Add / Saturate"),
        ("0x062610", 0x062610, 0x062694, "Sound Driver Channel Attenuation / Volume Compute"),
        ("0x062986", 0x062986, 0x0629A6, "Sound Driver Channel Pan / Stereo Control"),
        ("0x0626A6", 0x0626A6, 0x0626C0, "Sound Driver Channel Frequency Step Pitch Offset"),
        ("0x062900", 0x062900, 0x06291C, "Sound Driver Channel Timer Period Reset"),
        ("0x06293A", 0x06293A, 0x06295A, "Sound Driver Envelope Level Step"),
    ]

    for name, start, end, desc in candidates:
        print(f"\n==================================================")
        print(f"CANDIDATE: {name} (0x{start:06X}..0x{end:06X}): {desc}")
        # Check ownership overlap
        overlaps = []
        for s, e, h in helper_ranges + exp_ranges:
            if max(start, s) < min(end, e):
                overlaps.append((s, e, h))
        if overlaps:
            print(f"  OVERLAP CONFLICT: {overlaps}")
        else:
            print(f"  NO PRODUCTION / EXPERIMENTAL OVERLAPS (100% DISJOINT)")

        # Count steps in Scenario C
        steps = sum(inst_counts.get(pc, 0) for pc in range(start, end, 2))
        entry_calls = inst_counts.get(start, 0)
        print(f"  Scenario C calls: {entry_calls}, steps: {steps}")

if __name__ == '__main__':
    main()
