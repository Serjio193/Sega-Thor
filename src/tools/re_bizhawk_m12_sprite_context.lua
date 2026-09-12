-- Developer-only bounded execution context probe for the live SAT producer.
-- It records a narrow ROM-PC window and source-RAM writes; it never writes
-- emulator state or extracts ROM/graphics payloads.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_SPRITE_CONTEXT_OUTPUT") or "m12-sprite-context.json"
local max_override = tonumber(os.getenv("OASIS_M12_SPRITE_CONTEXT_MAX_FRAMES") or "")
local MAX_EXECUTIONS = 4096
local MAX_WRITES = 2048
local SOURCE_START, SOURCE_END = 0xFF13CC, 0xFF1800
local MIN_WRITE_FRAME = tonumber(os.getenv("OASIS_M12_SPRITE_CONTEXT_MIN_WRITE_FRAME") or "600")
local WATCH_PCS = { 0x0003B37C, 0x0003B3B2, 0x0003B3B6, 0x0003B3BC,
    0x0003B3BE, 0x0003B3C2, 0x0003B3C6, 0x0003B3D8, 0x0003B3DE,
    0x0003B416, 0x0003B41A, 0x0003B420, 0x0003B422, 0x0003B426,
    0x0003B42A,
    0x00000EFE, 0x0003B448, 0x0003CE86, 0x0003CE8C,
    0x0000B730, 0x0000B742, 0x0000B752, 0x0000B754, 0x0000B760,
    0x0000B764, 0x0000B76E, 0x0000B77A, 0x0000B790 }

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function hex(value) return string.format("0x%08X", (value or 0) & 0xFFFFFFFF) end
local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end
local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
end
local function read_ram(address, length)
    local ok, bytes = pcall(memory.read_bytes_as_array, address, length, "M68K BUS")
    if not ok or not bytes or #bytes < length then return nil end
    return bytes
end
local function bytes_json(bytes)
    if not bytes then return "null" end
    local result = {}
    for _, value in ipairs(bytes) do result[#result + 1] = tostring(value) end
    return "[" .. table.concat(result, ",") .. "]"
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

local inputs, max_frames, rom_sha = parse_scenario(scenario_path)
local frame, executions, writes = 0, {}, {}
local function append(list, item, limit)
    if #list < limit then list[#list + 1] = item end
end

for _, address in ipairs(WATCH_PCS) do
    local watched = address
    event.on_bus_exec(function()
        local regs = snapshot()
        append(executions, { frame = frame, pc = hex(watched), registers = regs,
            source_bytes = bytes_json(read_ram(SOURCE_START, 16)) }, MAX_EXECUTIONS)
    end, watched, "M12 sprite producer context", "M68K BUS")
end

event.on_bus_write(function(address, value, flags)
    if frame < MIN_WRITE_FRAME then return end
    if address < SOURCE_START or address >= SOURCE_END then return end
    local regs = snapshot()
    append(writes, { frame = frame, pc = hex(register("M68K PC")), address = hex(address),
        value = hex(value), flags = hex(flags or 0), registers = regs }, MAX_WRITES)
end, "M12 sprite source writes", "M68K BUS")

while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-sprite-context.v1",')
output:write('"emulator":"bizhawk","version":', json(client.getversion()),
    ',"canonical_rom_sha256":', json(rom_sha), ',"scenario_path":', json(scenario_path),
    ',"frames_executed":', tostring(frame), ',"watched_pcs":[')
local watched_values = {}
for _, address in ipairs(WATCH_PCS) do watched_values[#watched_values + 1] = json(hex(address)) end
output:write(table.concat(watched_values, ","), '],"executions":[')
local values = {}
for _, item in ipairs(executions) do
    values[#values + 1] = '{"frame":' .. tostring(item.frame) .. ',"pc":' .. json(item.pc) ..
        ',"registers":' .. snapshot_json(item.registers) ..
        ',"source_bytes":' .. item.source_bytes .. '}'
end
output:write(table.concat(values, ","), '],"source_writes":[')
values = {}
for _, item in ipairs(writes) do
    values[#values + 1] = '{"frame":' .. tostring(item.frame) .. ',"pc":' .. json(item.pc) ..
        ',"address":' .. json(item.address) .. ',"value":' .. json(item.value) ..
        ',"flags":' .. json(item.flags) .. ',"registers":' .. snapshot_json(item.registers) .. '}'
end
output:write(table.concat(values, ","), '],"limits":{"max_executions":', tostring(MAX_EXECUTIONS),
    ',"max_writes":', tostring(MAX_WRITES), '},"state_writes_emitted":false}')
output:close()
client.exitCode(0)
