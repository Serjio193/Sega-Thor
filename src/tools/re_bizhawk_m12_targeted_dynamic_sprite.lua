-- Developer-only bounded dynamic SAT capture.
-- The script starts from hardware reset, applies only scenario inputs, and
-- records every frame's SAT window plus writes to the accepted shadow range.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = assert(os.getenv("OASIS_M12_DYNAMIC_SAT_OUTPUT"), "OASIS_M12_DYNAMIC_SAT_OUTPUT is required")
local max_override = tonumber(os.getenv("OASIS_M12_DYNAMIC_SAT_MAX_FRAMES") or "")
local SHADOW_START, SHADOW_BYTES = 0xFF13CC, 0x158
local SAT_BASE, SAT_BYTES = 0xD000, 0x400
local MAX_WRITES = 32768

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function quote(value) return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"' end
local function hex(value) return string.format("0x%06X", (value or 0) & 0xFFFFFF) end
local function read_domain(address, length, domain)
    local ok, bytes = pcall(memory.read_bytes_as_array, address, length, domain)
    if not ok or not bytes or #bytes < length then error("read failed: " .. domain) end
    return bytes
end
local function bytes_json(bytes)
    local values = {}
    for index, value in ipairs(bytes) do values[index] = tostring(value) end
    return "[" .. table.concat(values, ",") .. "]"
end
local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
end
local function button_map(value)
    local result = {}
    for button in (value or ""):gmatch("[^+]+") do result["P1 " .. trim(button)] = true end
    return result
end

local inputs, max_frames, rom_sha = {}, 0, "unknown"
local scenario = assert(io.open(scenario_path, "r"))
for line in scenario:lines() do
    line = trim(line)
    local key, value = line:match("^(%S+)=([^%s]+)$")
    if key == "rom_sha256" then rom_sha = value end
    if key == "stop_condition" then max_frames = tonumber(value:match("max_frames:(%d+)")) or 0 end
    local frame, buttons = line:match("^input frame=(%d+) port=1 buttons=(%S+)$")
    if frame then inputs[tonumber(frame)] = buttons end
end
scenario:close()
max_frames = max_override or max_frames

local frame, writes, snapshots = 0, {}, {}
event.on_bus_write(function(address, value, flags)
    if address < SHADOW_START or address >= SHADOW_START + SHADOW_BYTES then return end
    if #writes >= MAX_WRITES then return end
    writes[#writes + 1] = { frame = frame, address = hex(address), value = value or 0,
        pc = hex(register("M68K PC")), flags = flags or 0 }
end, "M12 dynamic SAT shadow writes", "M68K BUS")

local function snapshot()
    snapshots[#snapshots + 1] = {
        frame = frame, sat = read_domain(SAT_BASE, SAT_BYTES, "VRAM"),
        shadow = read_domain(SHADOW_START, SHADOW_BYTES, "M68K BUS"),
    }
end

snapshot()
while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
    snapshot()
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m12.targeted-dynamic-sat-capture.v1",')
output:write('"emulator":"bizhawk","version":', quote(client.getversion()),
    ',"canonical_rom_sha256":', quote(rom_sha), ',"scenario_path":', quote(scenario_path),
    ',"frames_executed":', tostring(frame), ',"sat_base":', quote(hex(SAT_BASE)),
    ',"sat_bytes":', tostring(SAT_BYTES), ',"shadow_base":', quote(hex(SHADOW_START)),
    ',"shadow_bytes":', tostring(SHADOW_BYTES), ',"snapshots":[')
local rows = {}
for _, item in ipairs(snapshots) do
    rows[#rows + 1] = '{"frame":' .. tostring(item.frame) .. ',"sat":' .. bytes_json(item.sat) ..
        ',"shadow":' .. bytes_json(item.shadow) .. '}'
end
output:write(table.concat(rows, ","), '],"shadow_writes":[')
rows = {}
for _, item in ipairs(writes) do
    rows[#rows + 1] = '{"frame":' .. tostring(item.frame) .. ',"address":' .. quote(item.address) ..
        ',"value":' .. tostring(item.value) .. ',"pc":' .. quote(item.pc) ..
        ',"flags":' .. tostring(item.flags) .. '}'
end
output:write(table.concat(rows, ","), '],"limits":{"max_writes":', tostring(MAX_WRITES),
    '},"state_writes_emitted":false}')
output:close()
client.exitCode(0)
client.exit()
