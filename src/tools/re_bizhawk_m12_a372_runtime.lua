-- Developer-only bounded A372 register/root probe for M12.
-- It records addresses, registers, and bounded RAM values; it emits no ROM payload.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_A372_RUNTIME_OUTPUT") or "m12-a372-runtime.json"
local max_override = tonumber(os.getenv("OASIS_M12_A372_MAX_FRAMES") or "")
local MAX_EVENTS = 2048
local MAX_WRITES = 4096
local ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function hex(value) return string.format("0x%08X", (value or 0) & 0xFFFFFFFF) end
local function quote(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end

local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
end

local function read_bytes(address, length)
    local ok, bytes = pcall(memory.read_bytes_as_array, address, length, "M68K BUS")
    if not ok or not bytes or #bytes < length then return nil end
    return bytes
end

local function read_byte(address)
    local bytes = read_bytes(address, 1)
    return bytes and bytes[1] or 0
end

local function read_word(address)
    local bytes = read_bytes(address, 2)
    if not bytes then return 0 end
    return ((bytes[1] & 0xFF) << 8) | (bytes[2] & 0xFF)
end

local function signed_word(value)
    return value >= 0x8000 and value - 0x10000 or value
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

local function event_record(kind, pc, frame)
    local selector = read_byte(0x00FF1858)
    local offset = read_word(0x00FF188C)
    local root = selector == 0 and 0x00A438 or 0x00A480
    return {
        kind = kind, pc = hex(pc), frame = frame,
        selector = hex(selector), root = hex(root),
        ff188c = hex(offset), ff188c_signed = signed_word(offset),
        d2 = hex(register("M68K D2")), d5 = hex(register("M68K D5")),
        a5 = hex(register("M68K A5")),
        ff13cc = hex(read_word(0x00FF13CC)),
        ff1996 = hex(read_byte(0x00FF1996))
    }
end

local function event_json(item)
    return '{"kind":' .. quote(item.kind) .. ',"pc":' .. quote(item.pc) ..
        ',"frame":' .. item.frame .. ',"selector":' .. quote(item.selector) ..
        ',"root":' .. quote(item.root) .. ',"ff188c":' .. quote(item.ff188c) ..
        ',"ff188c_signed":' .. item.ff188c_signed .. ',"d2":' .. quote(item.d2) ..
        ',"d5":' .. quote(item.d5) .. ',"a5":' .. quote(item.a5) ..
        ',"ff13cc":' .. quote(item.ff13cc) .. ',"ff1996":' .. quote(item.ff1996) .. '}'
end

local inputs, max_frames, rom_sha = parse_scenario(scenario_path)
local frame = 0
local events, writes, hook_counts, root_counts = {}, {}, {}, {}
local watched = { [0x00A196] = "A196", [0x00A342] = "A342", [0x00A372] = "A372" }
for _, name in pairs(watched) do hook_counts[name] = 0 end

for pc, kind in pairs(watched) do
    event.on_bus_exec(function()
        hook_counts[kind] = hook_counts[kind] + 1
        local item = event_record(kind, pc, frame)
        if kind == "A342" then root_counts[item.root] = (root_counts[item.root] or 0) + 1 end
        if #events < MAX_EVENTS then events[#events + 1] = item end
    end, pc, "M12 A372 register/root watch", "M68K BUS")
end

event.on_bus_write(function(address, value)
    if address >= 0x00FF13CC and address < 0x00FF140C and #writes < MAX_WRITES then
        writes[#writes + 1] = {
            frame = frame, pc = hex(register("M68K PC")), address = hex(address), value = hex(value)
        }
    end
end, "M12 A372 shadow-SAT write watch", "M68K BUS")

while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local function map_json(values)
    local result = {}
    for key, value in pairs(values) do result[#result + 1] = quote(key) .. ":" .. tostring(value) end
    table.sort(result)
    return "{" .. table.concat(result, ",") .. "}"
end

local event_values, write_values = {}, {}
for _, item in ipairs(events) do event_values[#event_values + 1] = event_json(item) end
for _, item in ipairs(writes) do
    write_values[#write_values + 1] = '{"frame":' .. item.frame .. ',"pc":' .. quote(item.pc) ..
        ',"address":' .. quote(item.address) .. ',"value":' .. quote(item.value) .. '}'
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-a372-runtime.v1",')
output:write('"emulator":"bizhawk","version":', quote(client.getversion()),
    ',"canonical_rom_sha256":', quote(rom_sha), ',"expected_rom_sha256":', quote(ROM_SHA256),
    ',"scenario_path":', quote(scenario_path), ',"frames_executed":', tostring(frame),
    ',"hook_counts":', map_json(hook_counts), ',"root_counts":', map_json(root_counts),
    ',"event_cap":', tostring(MAX_EVENTS), ',"write_cap":', tostring(MAX_WRITES),
    ',"events":[', table.concat(event_values, ","), '],"source_writes":[',
    table.concat(write_values, ","), '],"state_writes_emitted":false}')
output:close()
client.exitCode(0)
