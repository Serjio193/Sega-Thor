-- Developer-only live SAT/VDP provenance probe for M12.
-- It observes a fixed natural-input scenario and never writes emulator state.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_SAT_OUTPUT") or "m12-sat-provenance.json"
local max_override = tonumber(os.getenv("OASIS_M12_SAT_MAX_FRAMES") or "")
local MAX_EVENTS = 4096
local MAX_SNAPSHOTS = 128
local MAX_DMA_LAUNCHES = 512
local MAX_SOURCE_RAM_EVENTS = 8192
local SOURCE_RAM_START, SOURCE_RAM_END = 0xFF13CC, 0xFF1800

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function hex(value) return string.format("0x%08X", (value or 0) & 0xFFFFFFFF) end
local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end
local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or nil
end
local function read_domain(address, length, domain)
    local ok, bytes = pcall(memory.read_bytes_as_array, address, length, domain)
    if not ok or not bytes or #bytes < length then return nil end
    return bytes
end
local function bytes_json(bytes)
    if not bytes then return "null" end
    local result = {}
    for _, value in ipairs(bytes) do result[#result + 1] = tostring(value) end
    return "[" .. table.concat(result, ",") .. "]"
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
local function push(list, item)
    if #list < MAX_EVENTS then list[#list + 1] = item end
end

local inputs, max_frames, rom_sha = parse_scenario(scenario_path)
local frame = 0
local vdp_controls, vdp_data, ram_writes, source_ram_writes = {}, {}, {}, {}
local sat_dma_launches = {}
local sat_bases, sat_base_set, sat_snapshots = {}, {}, {}
local last_snapshot_key = {}
local vdp_regs = {}

local function add_sat_base(value, pc)
    local base = (value & 0x7F) << 9
    if not sat_base_set[base] then
        sat_base_set[base] = true
        sat_bases[#sat_bases + 1] = { base = base, source_value = value, pc = pc, frame = frame }
    end
end

event.on_bus_write(function(address, value, flags)
    local pc = register("M68K PC") or 0
    if address == 0xC00004 then
        local item = { frame = frame, pc = hex(pc), value = hex(value), flags = hex(flags or 0) }
        local words = value > 0xFFFF and { (value >> 16) & 0xFFFF, value & 0xFFFF } or { value & 0xFFFF }
        for _, word in ipairs(words) do
            local reg = (word >> 8) & 0x1F
            if (word & 0xC000) == 0x8000 then
                item.register = reg
                item.data = word & 0xFF
                vdp_regs[reg] = word & 0xFF
                if reg == 5 then add_sat_base(word & 0xFF, pc) end
            end
        end
        if value > 0xFFFF and (value & 0x80) ~= 0 and #sat_dma_launches < MAX_DMA_LAUNCHES then
            local command_word = (value >> 16) & 0xFFFF
            local destination = (command_word & 0x3FFF) | ((value & 3) << 14)
            local is_sat_destination = sat_base_set[destination] == true
            if is_sat_destination then
                local source_words = (vdp_regs[21] or 0) |
                    ((vdp_regs[22] or 0) << 8) | ((vdp_regs[23] or 0) << 16)
                local length_words = (vdp_regs[19] or 0) | ((vdp_regs[20] or 0) << 8)
                if length_words == 0 then length_words = 0x10000 end
                local source_address = (source_words << 1) & 0xFFFFFF
                local byte_length = math.min(length_words * 2, 0x400)
                sat_dma_launches[#sat_dma_launches + 1] = {
                    frame = frame, pc = hex(pc), control_value = hex(value),
                    destination = hex(destination), source_address = hex(source_address),
                    length_words = length_words, source_regs = { vdp_regs[21] or 0,
                        vdp_regs[22] or 0, vdp_regs[23] or 0 },
                    source_bytes = bytes_json(read_domain(source_address, byte_length, "M68K BUS"))
                }
            end
        end
        push(vdp_controls, item)
    elseif address == 0xC00000 then
        push(vdp_data, { frame = frame, pc = hex(pc), value = hex(value), flags = hex(flags or 0) })
    elseif address >= 0xFF0000 and address <= 0xFFFFFF then
        push(ram_writes, { frame = frame, pc = hex(pc), address = hex(address),
            value = hex(value), flags = hex(flags or 0) })
        if frame >= 600 and address >= SOURCE_RAM_START and address < SOURCE_RAM_END and
            #source_ram_writes < MAX_SOURCE_RAM_EVENTS then
            source_ram_writes[#source_ram_writes + 1] = { frame = frame, pc = hex(pc),
                address = hex(address), value = hex(value), flags = hex(flags or 0) }
        end
    end
end, "M12 SAT VDP/RAM writes", "M68K BUS")

local function sample_sat(base_info)
    local bytes = read_domain(base_info.base, 0x400, "VRAM")
    if not bytes then return end
    local sum, nonzero = 2166136261, 0
    for _, value in ipairs(bytes) do
        if value ~= 0 then nonzero = nonzero + 1 end
        sum = ((sum ~ value) * 16777619) & 0xFFFFFFFF
    end
    local key = tostring(sum) .. ":" .. tostring(nonzero)
    if last_snapshot_key[base_info.base] == key or #sat_snapshots >= MAX_SNAPSHOTS then return end
    last_snapshot_key[base_info.base] = key
    sat_snapshots[#sat_snapshots + 1] = { frame = frame, base = hex(base_info.base),
        source_register_value = hex(base_info.source_value), source_pc = hex(base_info.pc),
        source_frame = base_info.frame, fnv1a32 = hex(sum), nonzero_bytes = nonzero,
        bytes = bytes_json(bytes) }
end

event.onframeend(function()
    for _, base_info in ipairs(sat_bases) do sample_sat(base_info) end
end)

while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local function events_json(events, fields)
    local values = {}
    for _, item in ipairs(events) do
        local parts = {}
        for _, field in ipairs(fields) do
            local value = item[field]
            parts[#parts + 1] = json(field) .. ":" .. (type(value) == "number" and tostring(value) or json(value))
        end
        values[#values + 1] = "{" .. table.concat(parts, ",") .. "}"
    end
    return table.concat(values, ",")
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-sat-provenance.v1",')
output:write('"emulator":"bizhawk","version":', json(client.getversion()),
    ',"canonical_rom_sha256":', json(rom_sha), ',"scenario_path":', json(scenario_path),
    ',"frames_executed":', tostring(frame), ',"sat_bases":[')
local bases = {}
for _, item in ipairs(sat_bases) do
    bases[#bases + 1] = '{"base":' .. json(hex(item.base)) .. ',"source_register_value":' ..
        json(hex(item.source_value)) .. ',"pc":' .. json(hex(item.pc)) ..
        ',"frame":' .. tostring(item.frame) .. '}'
end
output:write(table.concat(bases, ","), '],"sat_dma_launches":[')
local dma = {}
for _, item in ipairs(sat_dma_launches) do
    dma[#dma + 1] = '{"frame":' .. tostring(item.frame) .. ',"pc":' .. json(item.pc) ..
        ',"control_value":' .. json(item.control_value) .. ',"destination":' ..
        json(item.destination) .. ',"source_address":' .. json(item.source_address) ..
        ',"length_words":' .. tostring(item.length_words) .. ',"source_regs":[' ..
        tostring(item.source_regs[1]) .. ',' .. tostring(item.source_regs[2]) .. ',' ..
        tostring(item.source_regs[3]) .. '],"source_bytes":' .. item.source_bytes .. '}'
end
output:write(table.concat(dma, ","), '],"source_ram_writes":[')
output:write(events_json(source_ram_writes,
        { "frame", "pc", "address", "value", "flags" }),
    '],"vdp_controls":[')
output:write(events_json(vdp_controls, { "frame", "pc", "value", "flags", "register", "data" }),
    '],"vdp_data_writes":[', events_json(vdp_data, { "frame", "pc", "value", "flags" }),
    '],"ram_writes":[', events_json(ram_writes, { "frame", "pc", "address", "value", "flags" }),
    '],"sat_snapshots":[')
local snapshots = {}
for _, item in ipairs(sat_snapshots) do
    snapshots[#snapshots + 1] = '{"frame":' .. tostring(item.frame) .. ',"base":' .. json(item.base) ..
        ',"source_register_value":' .. json(item.source_register_value) .. ',"source_pc":' ..
        json(item.source_pc) .. ',"source_frame":' .. tostring(item.source_frame) ..
        ',"fnv1a32":' .. json(item.fnv1a32) .. ',"nonzero_bytes":' .. tostring(item.nonzero_bytes) ..
        ',"bytes":' .. item.bytes .. '}'
end
output:write(table.concat(snapshots, ","), '],"limits":{"max_events":', tostring(MAX_EVENTS),
    ',"max_snapshots":', tostring(MAX_SNAPSHOTS), ',"max_dma_launches":',
    tostring(MAX_DMA_LAUNCHES), ',"max_source_ram_events":', tostring(MAX_SOURCE_RAM_EVENTS),
    '},"state_writes_emitted":false}')
output:close()
client.exitCode(0)
