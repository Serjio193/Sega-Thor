-- Developer-only M12 producer backtrace capture.
-- It records register state at the proven SAT producer routine and never
-- writes guest state or promotes gameplay semantics.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = assert(os.getenv("OASIS_M12_DYNAMIC_SAT_BACKTRACE_OUTPUT"), "output is required")
local max_override = tonumber(os.getenv("OASIS_M12_DYNAMIC_SAT_BACKTRACE_MAX_FRAMES") or "")
local WATCH_PCS = {
    0xB6AA, 0xB6BC, 0xB6BE, 0xB6C2, 0xB6C8, 0xB6CA,
    0xB6F2, 0xB6FE, 0xB724, 0xB726,
    0x00DEA, 0x00E3A, 0x00EFE, 0x03B376, 0x03B3D8, 0x03B416, 0x03B41A, 0x03B420,
    0x03B422, 0x03B426, 0x03B428, 0x03B42C, 0x03B42E, 0x03B430,
    0x03B436, 0x03B438, 0x03B440, 0x03B444, 0x03B448, 0x03CE86,
    0x00B730, 0x00B73C, 0x00B74A,
    0xB74E, 0xB768, 0xB772, 0xB77E,
}
local SHADOW_START, SHADOW_END = 0xFF13CC, 0xFF1524
local MAX_EVENTS, MAX_WRITES = 100000, 32768

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function quote(value) return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"' end
local function hex(value) return string.format("0x%06X", (value or 0) & 0xFFFFFF) end
local function reg(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and (value or 0) & 0xFFFFFFFF or 0
end
local function read_word(address)
    local ok, bytes = pcall(memory.read_bytes_as_array, address & 0xFFFFFF, 2, "M68K BUS")
    if not ok or not bytes or #bytes < 2 then return nil end
    return ((bytes[1] & 0xFF) << 8) | (bytes[2] & 0xFF)
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
local function json_array(values)
    local result = {}
    for index, value in ipairs(values) do result[index] = tostring(value) end
    return "[" .. table.concat(result, ",") .. "]"
end

local inputs, max_frames, rom_sha = parse_scenario(scenario_path)
local frame, executions, writes = 0, {}, {}
local last_watched_pc = nil
local function capture_exec(pc)
    if #executions >= MAX_EVENTS then return end
    local a0, a1, a6 = reg("M68K A0"), reg("M68K A1"), reg("M68K A6")
    executions[#executions + 1] = {
        frame = frame, pc = hex(pc), d0 = reg("M68K D0"), d1 = reg("M68K D1"),
        d2 = reg("M68K D2"), d3 = reg("M68K D3"), d4 = reg("M68K D4"),
        d5 = reg("M68K D5"), d6 = reg("M68K D6"),
        a0 = a0, a1 = a1, a6 = a6, field_plus_8 = read_word(a6 + 8),
        source_plus_2 = read_word(a0 + 2),
        source_plus_4 = read_word(a0 + 4),
        predecessor_pc = pc == 0x00B730 and last_watched_pc or nil,
    }
    last_watched_pc = hex(pc)
end
for _, pc in ipairs(WATCH_PCS) do
    local watched = pc
    event.on_bus_exec(function() capture_exec(watched) end, watched,
        "M12 dynamic SAT producer backtrace", "M68K BUS")
end
event.on_bus_write(function(address, value, flags)
    if address < SHADOW_START or address >= SHADOW_END or #writes >= MAX_WRITES then return end
    writes[#writes + 1] = { frame = frame, address = hex(address), value = value or 0,
        pc = hex(reg("M68K PC")), flags = flags or 0 }
end, "M12 producer shadow writes", "M68K BUS")

while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local function execution_json(item)
    return '{"frame":' .. item.frame .. ',"pc":' .. quote(item.pc) ..
        ',"d0":' .. item.d0 .. ',"d1":' .. item.d1 .. ',"d2":' .. item.d2 ..
        ',"d3":' .. item.d3 .. ',"d4":' .. item.d4 .. ',"d5":' .. item.d5 ..
        ',"d6":' .. item.d6 .. ',"a0":' .. item.a0 .. ',"a6":' .. item.a6 ..
        ',"field_plus_8":' .. tostring(item.field_plus_8 or "null") ..
        ',"a1":' .. item.a1 .. ',"source_plus_2":' .. tostring(item.source_plus_2 or "null") ..
        ',"source_plus_4":' .. tostring(item.source_plus_4 or "null") ..
        ',"predecessor_pc":' .. tostring(item.predecessor_pc and quote(item.predecessor_pc) or "null") .. '}'
end
local function write_json(item)
    return '{"frame":' .. item.frame .. ',"address":' .. quote(item.address) ..
        ',"value":' .. item.value .. ',"pc":' .. quote(item.pc) .. ',"flags":' .. item.flags .. '}'
end
local exec_values, write_values = {}, {}
for _, item in ipairs(executions) do exec_values[#exec_values + 1] = execution_json(item) end
for _, item in ipairs(writes) do write_values[#write_values + 1] = write_json(item) end
local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m12.dynamic-sat-producer-backtrace-capture.v1",')
output:write('"canonical_rom_sha256":', quote(rom_sha), ',"scenario_path":', quote(scenario_path),
    ',"frames_executed":', frame, ',"executions":[', table.concat(exec_values, ","),
    '],"shadow_writes":[', table.concat(write_values, ","), '],"state_writes_emitted":false}')
output:close()
client.exitCode(0)
client.exit()
