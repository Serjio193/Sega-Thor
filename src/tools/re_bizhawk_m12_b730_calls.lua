-- Developer-only bounded call-context probe for the M12 table-to-B730 edge.
-- It records registers at selected PCs only and never reads or writes payload.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_B730_CALLS_OUTPUT") or "m12-b730-calls.json"
local max_override = tonumber(os.getenv("OASIS_M12_B730_CALLS_MAX_FRAMES") or "")
local MAX_EVENTS = 16384
local WATCH_PCS = {
    0x0003B3B2, 0x0003B3B6, 0x0003B3BC, 0x0003B3BE,
    0x0003B3C2, 0x0003B3E4, 0x0003B416, 0x0003B41A,
    0x0003B420, 0x0003B422, 0x0003B426, 0x0003B428,
    0x0003B430, 0x0003B432, 0x0003B436, 0x0003B438,
    0x0003B440, 0x0003B444, 0x0003B448, 0x0000B730
}

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function hex(value) return string.format("0x%08X", (value or 0) & 0xFFFFFFFF) end
local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end
local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
end
local function snapshot()
    local result = { a = {}, d = {}, sr = register("M68K SR") }
    for index = 0, 7 do
        result.a[index] = register("M68K A" .. index)
        result.d[index] = register("M68K D" .. index)
    end
    return result
end
local function snapshot_json(value)
    local result = '{"a":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. json(hex(value.a[index]))
    end
    result = result .. '],"d":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. json(hex(value.d[index]))
    end
    return result .. '],"sr":' .. json(hex(value.sr)) .. "}"
end
local function parse_scenario(path)
    local inputs, frames, rom_sha = {}, 0, "unknown"
    local input = assert(io.open(path, "r"))
    for line in input:lines() do
        line = trim(line)
        local key, value = line:match("^(%S+)=([^%s]+)$")
        if key == "rom_sha256" then rom_sha = value end
        if key == "stop_condition" then frames = tonumber(value:match("max_frames:(%d+)")) or 0 end
        local frame, buttons = line:match("^input frame=(%d+) port=1 buttons=(%S+)$")
        if frame then inputs[tonumber(frame)] = buttons end
    end
    input:close()
    return inputs, max_override or frames, rom_sha
end
local function button_map(value)
    local result = {}
    for button in (value or ""):gmatch("[^+]+") do result["P1 " .. trim(button)] = true end
    return result
end
local function append(list, item)
    if #list < MAX_EVENTS then list[#list + 1] = item end
end

local inputs, max_frames, rom_sha = parse_scenario(scenario_path)
local frame, executions = 0, {}
for _, watched_pc in ipairs(WATCH_PCS) do
    event.on_bus_exec(function()
        append(executions, { frame = frame, pc = hex(watched_pc), registers = snapshot() })
    end, watched_pc, "M12 B730 call context", "M68K BUS")
end

while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-b730-call-context.v1",')
output:write('"emulator":"bizhawk","version":', json(client.getversion()),
    ',"canonical_rom_sha256":', json(rom_sha), ',"scenario_path":', json(scenario_path),
    ',"frames_executed":', tostring(frame), ',"watched_pcs":[')
local watched = {}
for _, pc in ipairs(WATCH_PCS) do watched[#watched + 1] = json(hex(pc)) end
output:write(table.concat(watched, ","), '],"executions":[')
local events = {}
for _, item in ipairs(executions) do
    events[#events + 1] = '{"frame":' .. tostring(item.frame) .. ',"pc":' ..
        json(item.pc) .. ',"registers":' .. snapshot_json(item.registers) .. "}"
end
output:write(table.concat(events, ","), '],"limits":{"max_events":', tostring(MAX_EVENTS),
    '},"state_writes_emitted":false}')
output:close()
client.exitCode(0)
