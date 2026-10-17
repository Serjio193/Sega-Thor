import json

with open("build/session-00e0f4-run-B/replay-run/call-census.json") as f:
    data = json.load(f)

targets = data.get("targets", [])
print(f"Total targets in census: {len(targets)}")

displaced = {
    "613b2", "61e48", "61934", "6147e", "623ac", "6273a", "62788", "60000",
    "60286", "62ae0", "626bc", "0027fc", "002992", "6138e", "00e682", "0613f8",
    "016ebc", "009ad6", "009bf2", "0027a6", "00b15e", "00a8da", "017d12", "6281a",
    "6282e", "01ac90", "01b102", "00350e", "008e22", "00a7e6", "008e90", "00f788",
    "00a196", "00a342", "00b34c", "00c980", "00c998", "008dd4", "00557a", "005670",
    "008b8e", "008bac", "00deec", "00e0ba", "00e0f4"
}

displaced_clean = {x.lower().lstrip("0") for x in displaced}

remaining = []
for t in targets:
    pc_str = t.get("ENTRY_PC", "").lower().replace("0x", "")
    pc_clean = pc_str.lstrip("0")
    if pc_clean not in displaced_clean:
        remaining.append(t)

remaining.sort(key=lambda x: x.get("CALL_COUNT", 0), reverse=True)

print(f"\nRemaining targets sorted by CALL_COUNT: {len(remaining)}")
print("Top 20 remaining targets by call count:")
for i, t in enumerate(remaining[:20]):
    print(f"{i+1:2d}. {t.get('ENTRY_PC')} calls={t.get('CALL_COUNT')} callers={t.get('UNIQUE_CALLER_PCS')}")
