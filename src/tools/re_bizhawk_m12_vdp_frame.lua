-- Developer-only, read-only coherent VDP state capture for M12 S8.
-- It captures one frame's register history plus VRAM/CRAM/VSRAM and never
-- writes emulator state or promotes ROM ownership.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_VDP_OUTPUT") or "hardware-vdp-frame-capture.json"
local run_id = os.getenv("OASIS_M12_RUN_ID") or "m12_m11_8_natural_v1"
local target_frame = tonumber(os.getenv("OASIS_M12_VDP_FRAME") or "779")
local max_override = tonumber(os.getenv("OASIS_M12_VDP_MAX_FRAMES") or "")

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end
local function bytes_json(bytes)
    if not bytes then return "null" end
    local result = {}
    for _, value in ipairs(bytes) do result[#result + 1] = tostring(value) end
    return "[" .. table.concat(result, ",") .. "]"
end
local function hex(value) return string.format("0x%08X", (value or 0) & 0xFFFFFFFF) end
local function read_domain(address, length, domain)
    local ok, bytes = pcall(memory.read_bytes_as_array, address, length, domain)
    if not ok or not bytes or #bytes < length then return nil end
    return bytes
end
local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
end
local function parse_scenario(path)
    local input = assert(io.open(path, "r"))
    local frames, rom_sha = 0, "unknown"
    for line in input:lines() do
        line = trim(line)
        local key, value = line:match("^(%S+)=([^%s]+)$")
        if key == "rom_sha256" then rom_sha = value end
        if key == "stop_condition" then frames = tonumber(value:match("max_frames:(%d+)")) or 0 end
    end
    input:close()
    return max_override or frames, rom_sha
end

local max_frames, rom_sha = parse_scenario(scenario_path)
local frame, sequence, captured = 0, 0, nil
local register_events = {}
local current_registers = {}

local function append_event(register_number, value, raw_word)
    sequence = sequence + 1
    local event_item = {frame = frame, sequence = sequence, register = register_number,
        value = value, raw_word = raw_word, pc = hex(register("M68K PC")), phase = "bus_write"}
    register_events[#register_events + 1] = event_item
    current_registers[register_number] = value
end

event.on_bus_write(function(address, value)
    if address ~= 0xC00004 then return end
    local words = value > 0xFFFF and {(value >> 16) & 0xFFFF, value & 0xFFFF} or {value & 0xFFFF}
    for _, word in ipairs(words) do
        if (word & 0xC000) == 0x8000 then
            append_event((word >> 8) & 0x1F, word & 0xFF, word)
        end
    end
end, "M12 VDP register evidence", "M68K BUS")

event.onframeend(function()
    if frame ~= target_frame or captured then return end
    local vram = read_domain(0, 0x10000, "VRAM")
    local cram = read_domain(0, 0x80, "CRAM")
    local vsram = read_domain(0, 0x50, "VSRAM")
    if not vram or not cram or not vsram then return end
    captured = {frame = frame, vram_frame = frame, cram_frame = frame, vsram_frame = frame,
        vram = vram, cram = cram, vsram = vsram,
        registers = current_registers}
end)

while frame <= max_frames do
    emu.frameadvance()
    frame = frame + 1
    if captured then break end
end
if not captured then error("M12 VDP target frame was not captured") end

local function events_json(values)
    local result = {}
    for _, item in ipairs(values) do
        result[#result + 1] = '{"frame":' .. tostring(item.frame) ..
            ',"sequence":' .. tostring(item.sequence) .. ',"register":' .. tostring(item.register) ..
            ',"value":' .. tostring(item.value) .. ',"raw_word":' .. tostring(item.raw_word) ..
            ',"pc":' .. json(item.pc) .. ',"phase":"bus_write"}'
    end
    return table.concat(result, ",")
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-vdp-frame-capture.v1",')
output:write('"run_id":', json(run_id), ',"frame":', tostring(captured.frame),
    ',"vram_frame":', tostring(captured.vram_frame), ',"cram_frame":', tostring(captured.cram_frame),
    ',"vsram_frame":', tostring(captured.vsram_frame),
    ',"canonical_rom_sha256":', json(rom_sha), ',"scenario_path":', json(scenario_path),
    ',"emulator":"bizhawk","version":', json(client.getversion()),
    ',"state_writes_emitted":false,"register_events":[', events_json(register_events),
    '],"vram":', bytes_json(captured.vram), ',"cram":', bytes_json(captured.cram),
    ',"vsram":', bytes_json(captured.vsram), '}')
output:close()
client.exitCode(0)
