-- Developer-only exact-address ROM-read provenance probe for M12.
-- It watches only table-pointer and bounded source-record addresses; it never
-- writes emulator state or dumps an asset range.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_TARGETED_READS_OUTPUT") or "m12-targeted-reads.json"
local max_override = tonumber(os.getenv("OASIS_M12_TARGETED_READS_MAX_FRAMES") or "")
local MIN_FRAME = tonumber(os.getenv("OASIS_M12_TARGETED_READS_MIN_FRAME") or "600")
local MAX_EVENTS = 16384
local WATCH_ADDRESSES = {
    0x003B95C, 0x003B982, 0x003B998, 0x003B9A6,
    0x003B9BC, 0x003B9D2, 0x003B9E8,
    0x00FFAFAE,
    0x00171832, 0x001742DC, 0x001742E2, 0x001762E2, 0x00179440,
    0x0017C14A, 0x0017E3E2
}
for record = 0, 7 do
    local base = 0x003B8DE + record * 0x10
    for offset = 0, 12, 4 do WATCH_ADDRESSES[#WATCH_ADDRESSES + 1] = base + offset end
end
local SELECTOR_CALLBACK_PCS = {
    [0x0003A91C] = true, [0x0003A9F8] = true,
    [0x0003AA18] = true, [0x0003AA5A] = true,
    [0x0003AA98] = true, [0x0003B3BC] = true,
    [0x0003B420] = true
}
local PROVEN_SOURCE_STARTS = {
    0x00171864, 0x0017186C, 0x00171874, 0x0017187C,
    0x00171884, 0x00171892, 0x001718A6, 0x001718BA,
    0x001718D4, 0x001718EE, 0x0017190E, 0x0017192E,
    0x00171954, 0x0017197A, 0x001719A6, 0x001719D2,
    0x00171A04, 0x00171A0C, 0x00171A14, 0x00171A1C,
    0x00171A2A, 0x00171A38, 0x00171A46, 0x00171A54,
    0x001742F8, 0x00174358
}
for _, start in ipairs(PROVEN_SOURCE_STARTS) do
    for offset = 0, 6, 2 do WATCH_ADDRESSES[#WATCH_ADDRESSES + 1] = start + offset end
end

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function hex(value) return string.format("0x%08X", (value or 0) & 0xFFFFFFFF) end
local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end
local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
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
local frame, reads = 0, {}
for _, watched_address in ipairs(WATCH_ADDRESSES) do
    event.on_bus_read(function(address, value, flags)
        if frame < MIN_FRAME then return end
        local pc = register("M68K PC")
        if address == 0x00FFAFAE and not SELECTOR_CALLBACK_PCS[pc] then
            return
        end
        append(reads, { frame = frame, address = hex(address), pc = hex(pc),
            value = hex(value), flags = hex(flags or 0) })
    end, watched_address, "M12 exact ROM read " .. hex(watched_address), "M68K BUS")
end

while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-targeted-rom-reads.v1",')
output:write('"emulator":"bizhawk","version":', json(client.getversion()),
    ',"canonical_rom_sha256":', json(rom_sha), ',"scenario_path":', json(scenario_path),
    ',"frames_executed":', tostring(frame), ',"watch_addresses":[')
local addresses = {}
for _, address in ipairs(WATCH_ADDRESSES) do addresses[#addresses + 1] = json(hex(address)) end
output:write(table.concat(addresses, ","), '],"read_events":[')
local events = {}
for _, item in ipairs(reads) do
    events[#events + 1] = '{"frame":' .. tostring(item.frame) .. ',"address":' ..
        json(item.address) .. ',"pc":' .. json(item.pc) .. ',"value":' ..
        json(item.value) .. ',"flags":' .. json(item.flags) .. "}"
end
output:write(table.concat(events, ","), '],"limits":{"max_events":', tostring(MAX_EVENTS),
    '},"state_writes_emitted":false}')
output:close()
client.exitCode(0)
