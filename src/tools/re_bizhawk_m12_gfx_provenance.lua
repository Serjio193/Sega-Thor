-- Developer-only targeted BizHawk capture for the ten unresolved 0x3820 callers.
-- It observes a frozen natural-input scenario; it never writes emulator state.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_GFX_RUNTIME_OUTPUT") or "m12-gfx-runtime.json"
local max_override = tonumber(os.getenv("OASIS_M12_GFX_MAX_FRAMES") or "")
local target = 0x3820
local callers = { 0x00D54A, 0x00D650, 0x02DB52, 0x02F6A0, 0x03B236,
    0x03B28A, 0x03B2FE, 0x03C07C, 0x03D5AE, 0x03E61A }
local caller_set = {}
for _, address in ipairs(callers) do caller_set[address] = true end

local function trim(value)
    return (value:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function hex(value)
    return string.format("0x%08X", (value or 0) & 0xFFFFFFFF)
end

local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end

local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or nil
end

local function snapshot()
    local result = { a = {}, d = {}, sr = register("M68K SR") }
    for index = 0, 7 do
        result.a[index] = register("M68K A" .. index)
        result.d[index] = register("M68K D" .. index)
    end
    return result
end

local function read_bytes(address, length)
    if not address or address < 0 or address >= 0x300000 then return nil end
    local ok, bytes = pcall(memory.read_bytes_as_array, address, length, "M68K BUS")
    if not ok or not bytes or #bytes < length then return nil end
    return bytes
end

local function parse_scenario(path)
    local inputs = {}
    local frames = 0
    local rom_sha = "unknown"
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

local function snapshot_json(value)
    local result = '{"d":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. json(hex(value.d[index]))
    end
    result = result .. '],"a":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. json(hex(value.a[index]))
    end
    return result .. '],"sr":' .. json(hex(value.sr)) .. '}'
end

local function bytes_json(bytes)
    if not bytes then return "null" end
    local result = {}
    for _, value in ipairs(bytes) do result[#result + 1] = string.format("%d", value) end
    return "[" .. table.concat(result, ",") .. "]"
end

local inputs, max_frames, rom_sha = parse_scenario(scenario_path)
local frame = 0
local sequence = 0
local pending = nil
local captures = {}
local call_counts = {}
for _, address in ipairs(callers) do call_counts[address] = 0 end

local function call_event(address)
    local registers = snapshot()
    call_counts[address] = call_counts[address] + 1
    pending = { site = address, sequence = sequence, frame = frame, registers = registers }
    sequence = sequence + 1
end

local function target_event()
    local registers = snapshot()
    local item = { sequence = sequence, frame = frame, registers = registers,
        caller = pending and pending.site or nil,
        caller_sequence = pending and pending.sequence or nil,
        caller_frame = pending and pending.frame or nil,
        source_bytes = bytes_json(read_bytes(registers.a[0], 8)) }
    captures[#captures + 1] = item
    pending = nil
    sequence = sequence + 1
end

for _, address in ipairs(callers) do
    local watched = address
    event.on_bus_exec(function() call_event(watched) end, watched,
        "M12 graphics caller watch", "M68K BUS")
end
event.on_bus_exec(target_event, target, "M12 graphics decoder entry", "M68K BUS")

while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-gfx-runtime-provenance.v1",')
output:write('"canonical_rom_sha256":', json(rom_sha), ',"scenario_path":', json(scenario_path),
    ',"frames_executed":', tostring(frame), ',"target":', json(hex(target)), ',"callers":[')
local caller_values = {}
for _, address in ipairs(callers) do
    caller_values[#caller_values + 1] = '{"address":' .. json(hex(address)) ..
        ',"count":' .. tostring(call_counts[address]) .. '}'
end
output:write(table.concat(caller_values, ","), '],"captures":[')
local capture_values = {}
for _, item in ipairs(captures) do
    capture_values[#capture_values + 1] = '{"sequence":' .. tostring(item.sequence) ..
        ',"frame":' .. tostring(item.frame) .. ',"caller":' ..
        (item.caller and json(hex(item.caller)) or "null") ..
        ',"caller_sequence":' .. (item.caller_sequence and tostring(item.caller_sequence) or "null") ..
        ',"caller_frame":' .. (item.caller_frame and tostring(item.caller_frame) or "null") ..
        ',"registers":' .. snapshot_json(item.registers) ..
        ',"source_probe":' .. item.source_bytes .. '}'
end
output:write(table.concat(capture_values, ","), '],"writes_emitted":false}')
output:close()
client.exitCode(0)
