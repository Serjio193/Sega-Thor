-- Developer-only read-only witness capture for one hardware sprite piece.
-- It never writes emulator state or promotes ROM ownership.

local scenario_path = assert(os.getenv("OASIS_SCENARIO_FILE"), "OASIS_SCENARIO_FILE is required")
local output_path = os.getenv("OASIS_M12_SPRITE_PIECE_OUTPUT") or "hardware-sprite-piece-capture.json"
local run_id = os.getenv("OASIS_M12_RUN_ID") or "m12_m11_8_natural_v1"
local max_override = tonumber(os.getenv("OASIS_M12_SPRITE_PIECE_MAX_FRAMES") or "")
local SAT_BASE = 0xD000
local SAT_BYTES = 80 * 8
local SHADOW_BASE, SHADOW_END = 0xFF13CC, 0xFF13CC + SAT_BYTES
local MAX_WITNESSES = 8
local MAX_CATALOG_OBSERVATIONS = 4096
local MAX_EVENTS = 4096
local PRODUCER_PCS = { 0xB752, 0xB764, 0xB76E, 0xB77A }

local function trim(value) return value:gsub("^%s+", ""):gsub("%s+$", "") end
local function hex(value) return string.format("0x%08X", (value or 0) & 0xFFFFFFFF) end
local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end
local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
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
local function append(list, item, limit)
    if #list < limit then list[#list + 1] = item end
end
local function value_bytes(value)
    if value > 0xFFFF then
        return { (value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF }
    end
    return { (value >> 8) & 0xFF, value & 0xFF }
end
local function overlap(start_address, length, range_start, range_length)
    local finish = start_address + length
    local range_finish = range_start + range_length
    local first = math.max(start_address, range_start)
    local last = math.min(finish, range_finish)
    return first, last
end

local inputs, max_frames, rom_sha = parse_scenario(scenario_path)
local frame, sequence = 0, 0
local writes, executions, dmas, witnesses, catalog_observations = {}, {}, {}, {}, {}
local sat_mutations, shadow_mutations, sat_base_events = {}, {}, {}
local dma_by_frame = {}
local sat_dmas_by_frame = {}
local vdp_regs = {}
local vdp_data_address = 0
local vdp_sat_base = SAT_BASE

local function next_sequence()
    sequence = sequence + 1
    return sequence
end
local function producer_execution(pc)
    local item = { frame = frame, sequence = next_sequence(), pc = hex(pc),
        address = hex(SHADOW_BASE), bytes = read_domain(SHADOW_BASE, 8, "M68K BUS") }
    append(executions, item, MAX_EVENTS)
end
for _, pc in ipairs(PRODUCER_PCS) do
    local watched = pc
    event.on_bus_exec(function() producer_execution(watched) end, watched,
        "M12 sprite piece producer", "M68K BUS")
end

event.on_bus_write(function(address, value, flags)
    if address < SHADOW_BASE or address >= SHADOW_END then return end
    local raw_bytes = value_bytes(value)
    local item = { frame = frame, sequence = next_sequence(), pc = hex(register("M68K PC")),
        address = hex(address), value = hex(value), flags = hex(flags or 0),
        destination_start = address, destination_end = address + #raw_bytes,
        raw_bytes = raw_bytes, phase = "bus_write" }
    append(writes, item, MAX_EVENTS)
    append(shadow_mutations, item, MAX_EVENTS)
end, "M12 sprite piece shadow writes", "M68K BUS")

event.on_bus_write(function(address, value)
    if address ~= 0xC00004 then return end
    local words = value > 0xFFFF and { (value >> 16) & 0xFFFF, value & 0xFFFF } or
        { value & 0xFFFF }
    for _, word in ipairs(words) do
        local reg = (word >> 8) & 0x1F
        if (word & 0xC000) == 0x8000 then
            vdp_regs[reg] = word & 0xFF
            if reg == 5 then
                local new_base = (word & 0x7F) << 9
                if new_base ~= vdp_sat_base then
                    append(sat_base_events, { frame = frame, sequence = next_sequence(),
                        phase = "bus_write", register = 5, value = word & 0xFF,
                        sat_base = new_base }, MAX_EVENTS)
                    vdp_sat_base = new_base
                end
            end
        end
    end
    local command_word = value > 0xFFFF and ((value >> 16) & 0xFFFF) or (value & 0xFFFF)
    local extension = value > 0xFFFF and (value & 0xFFFF) or 0
    vdp_data_address = (command_word & 0x3FFF) | ((extension & 3) << 14)
end, "M12 sprite piece VDP registers", "M68K BUS")

event.on_bus_write(function(address, value)
    if address ~= 0xC00000 then return end
    local raw_bytes = value_bytes(value)
    local start_address = vdp_data_address
    local first, last = overlap(start_address, #raw_bytes, SAT_BASE, SAT_BYTES)
    if first < last then
        local affected = {}
        for index = first - start_address + 1, last - start_address do
            affected[#affected + 1] = raw_bytes[index]
        end
        append(sat_mutations, { frame = frame, sequence = next_sequence(), phase = "bus_write",
            kind = "CPU_VDP_DATA_PORT_WRITE", destination_start = first,
            destination_end = last, source = "", length = last - first,
            raw_affected_bytes = affected }, MAX_EVENTS)
    end
    vdp_data_address = (start_address + #raw_bytes) & 0xFFFF
end, "M12 sprite piece SAT data-port mutations", "M68K BUS")

event.on_bus_write(function(address, value, flags)
    if address ~= 0xC00004 or value <= 0xFFFF or (value & 0x80) == 0 then return end
    local command_word = (value >> 16) & 0xFFFF
    local destination = (command_word & 0x3FFF) | ((value & 3) << 14)
    if destination < SAT_BASE or destination >= SAT_BASE + SAT_BYTES then return end
    local source_words = (vdp_regs[21] or 0) | ((vdp_regs[22] or 0) << 8) |
        ((vdp_regs[23] or 0) << 16)
    local length_words = (vdp_regs[19] or 0) | ((vdp_regs[20] or 0) << 8)
    if length_words == 0 then length_words = 0x10000 end
    local item = { frame = frame, sequence = next_sequence(), pc = hex(register("M68K PC")),
        source_value = (source_words << 1) & 0xFFFFFF, destination_value = destination,
        source = hex((source_words << 1) & 0xFFFFFF), destination = hex(destination),
        length_words = length_words, source_bytes = read_domain((source_words << 1) & 0xFFFFFF,
            math.min(length_words * 2, 0x400), "M68K BUS") }
    append(dmas, item, MAX_EVENTS)
    if not sat_dmas_by_frame[frame] then sat_dmas_by_frame[frame] = {} end
    append(sat_dmas_by_frame[frame], item, 256)
    local first, last = overlap(destination, length_words * 2, SAT_BASE, SAT_BYTES)
    if first < last then
        local affected = {}
        local offset = first - destination
        if item.source_bytes then
            for index = offset + 1, math.min(offset + last - first, #item.source_bytes) do
                affected[#affected + 1] = item.source_bytes[index]
            end
        end
        append(sat_mutations, { frame = frame, sequence = item.sequence, phase = "bus_write",
            kind = "VDP_DMA", destination_start = first, destination_end = last,
            source = item.source, length = last - first, raw_affected_bytes = affected }, MAX_EVENTS)
    end
    if destination == SAT_BASE then dma_by_frame[frame] = item end
end, "M12 sprite piece SAT publication", "M68K BUS")

local function sat_entry_bytes(sat, index)
    local result = {}
    local offset = index * 8
    for byte = 1, 8 do result[byte] = sat[offset + byte] end
    return result
end

local function capture_catalog()
    local sat_full = read_domain(SAT_BASE, 80 * 8, "VRAM")
    if not sat_full then return end
    for index = 0, 79 do
        local sat = sat_entry_bytes(sat_full, index)
        local word1 = sat[3] * 256 + sat[4]
        local attr = sat[5] * 256 + sat[6]
        local width = ((word1 >> 10) & 3) + 1
        local height = ((word1 >> 8) & 3) + 1
        local tile = attr & 0x7FF
        local palette = (attr >> 13) & 3
        if tile ~= 0 and #catalog_observations < MAX_CATALOG_OBSERVATIONS then
            local tiles = {}
            for cell_x = 0, width - 1 do
                for cell_y = 0, height - 1 do
                    local tile_index = tile + cell_x * height + cell_y
                    tiles[#tiles + 1] = {
                        tile_index = tile_index, vram_address = tile_index * 32,
                        bytes = read_domain(tile_index * 32, 32, "VRAM")
                    }
                end
            end
            local publication, publication_offset, publication_bytes
            local entry_address = SAT_BASE + index * 8
            for _, candidate in ipairs(sat_dmas_by_frame[frame] or {}) do
                local offset = entry_address - candidate.destination_value
                if offset >= 0 and offset + 8 <= candidate.length_words * 2 and
                    candidate.source_bytes and #candidate.source_bytes >= offset + 8 then
                    publication, publication_offset = candidate, offset
                    publication_bytes = {}
                    for byte = 1, 8 do publication_bytes[byte] = candidate.source_bytes[offset + byte] end
                    break
                end
            end
            local observed_shadow_address = hex(SHADOW_BASE + index * 8)
            local observed_shadow_bytes = read_domain(SHADOW_BASE + index * 8, 8, "M68K BUS")
            if publication and publication_offset and publication_bytes then
                observed_shadow_address = hex(publication.source_value + publication_offset)
                observed_shadow_bytes = publication_bytes
            end
            local causality = { status = "UNPROVEN" }
            if publication and publication_offset then
                local source_address = publication.source_value + publication_offset
                local producer_write
                for _, candidate in ipairs(writes) do
                    if candidate.frame == frame and candidate.destination_start <= source_address and
                        candidate.destination_end > source_address and
                        candidate.destination_start < source_address + 8 and
                        candidate.sequence < publication.sequence then
                        producer_write = candidate
                    end
                end
                if producer_write then
                    causality = {
                        status = "ORDERED_PRODUCER_WRITE_TO_DMA",
                        producer_event_id = tostring(frame) .. ":" .. tostring(producer_write.sequence),
                        dma_event_id = tostring(frame) .. ":" .. tostring(publication.sequence),
                        source_offset = publication_offset,
                        destination_offset = entry_address - SAT_BASE,
                        ordering_relation = "producer_write_sequence_lt_dma_sequence"
                    }
                end
            end
            catalog_observations[#catalog_observations + 1] = {
                frame = frame, sat_entry = index, sat_address = hex(SAT_BASE + index * 8),
                capture_phase = "frame_end",
                sat_bytes = sat, shadow_address = observed_shadow_address,
                shadow_bytes = observed_shadow_bytes,
                dma = dma_by_frame[frame], publication = publication,
                publication_offset = publication_offset, publication_bytes = publication_bytes,
                causality = causality,
                tiles = tiles,
                cram_base_address = palette * 32,
                cram_bytes = read_domain(palette * 32, 32, "CRAM")
            }
        end
    end
end

local function capture_witness()
    capture_catalog()
    if #witnesses >= MAX_WITNESSES then return end
    local sat = read_domain(SAT_BASE, 8, "VRAM")
    local shadow = read_domain(SHADOW_BASE, 8, "M68K BUS")
    local dma = dma_by_frame[frame]
    if not sat or not shadow or not dma then return end
    local word1 = sat[3] * 256 + sat[4]
    local attr = sat[5] * 256 + sat[6]
    local width = ((word1 >> 10) & 3) + 1
    local height = ((word1 >> 8) & 3) + 1
    local tile = attr & 0x7FF
    local palette = (attr >> 13) & 3
    -- Ignore the initial hidden/blank SAT entry; the witness must be an
    -- actually populated hardware sprite piece.
    if tile == 0 then return end
    local tile_bytes = read_domain(tile * 32, width * height * 32, "VRAM")
    local cram_bytes = read_domain(palette * 32, 32, "CRAM")
    if width ~= 1 or height ~= 1 or not tile_bytes or not cram_bytes then return end
    append(witnesses, { frame = frame, sat_index = 0, sat_address = hex(SAT_BASE),
        shadow_address = hex(SHADOW_BASE), shadow_bytes = shadow, sat_bytes = sat,
        tile_bytes = tile_bytes, cram_bytes = cram_bytes, dma = dma,
        producer_writes = writes, producer_executions = executions }, MAX_WITNESSES)
end

event.onframeend(capture_witness)
while frame < max_frames do
    joypad.set(button_map(inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local function list_json(values, fields)
    local result = {}
    for _, item in ipairs(values) do
        local parts = {}
        for _, field in ipairs(fields) do
            local value = item[field]
            if type(value) == "table" then
                parts[#parts + 1] = json(field) .. ":" .. bytes_json(value)
            elseif type(value) == "number" then
                parts[#parts + 1] = json(field) .. ":" .. tostring(value)
            else
                parts[#parts + 1] = json(field) .. ":" .. json(value)
            end
        end
        result[#result + 1] = "{" .. table.concat(parts, ",") .. "}"
    end
    return table.concat(result, ",")
end
local function dma_json(item)
    if not item then return "null" end
    return '{"frame":' .. tostring(item.frame) ..
        ',"sequence":' .. tostring(item.sequence) ..
        ',"pc":' .. json(item.pc) .. ',"source":' .. json(item.source) ..
        ',"destination":' .. json(item.destination) ..
        ',"length_words":' .. tostring(item.length_words) ..
        ',"source_bytes":' .. bytes_json(item.source_bytes) .. '}'
end
local function catalog_json(item)
    local tile_json = {}
    for _, tile in ipairs(item.tiles) do
        tile_json[#tile_json + 1] = '{"tile_index":' .. tostring(tile.tile_index) ..
            ',"vram_address":' .. tostring(tile.vram_address) ..
            ',"bytes":' .. bytes_json(tile.bytes) .. '}'
    end
    local causal = item.causality or { status = "UNPROVEN" }
    local causal_json = '{"status":' .. json(causal.status)
    if causal.producer_event_id then
        causal_json = causal_json .. ',"producer_event_id":' .. json(causal.producer_event_id) ..
            ',"dma_event_id":' .. json(causal.dma_event_id) ..
            ',"source_offset":' .. tostring(causal.source_offset) ..
            ',"destination_offset":' .. tostring(causal.destination_offset) ..
            ',"ordering_relation":' .. json(causal.ordering_relation)
    end
    causal_json = causal_json .. '}'
    return '{"frame":' .. tostring(item.frame) ..
        ',"sat_entry":' .. tostring(item.sat_entry) ..
        ',"capture_phase":' .. json(item.capture_phase) ..
        ',"sat_address":' .. json(item.sat_address) ..
        ',"sat_bytes":' .. bytes_json(item.sat_bytes) ..
        ',"shadow_address":' .. json(item.shadow_address) ..
        ',"shadow_bytes":' .. bytes_json(item.shadow_bytes) ..
        ',"dma":' .. dma_json(item.dma) ..
        ',"publication":' .. dma_json(item.publication) ..
        ',"publication_offset":' .. (item.publication_offset and tostring(item.publication_offset) or "null") ..
        ',"publication_bytes":' .. bytes_json(item.publication_bytes) ..
        ',"causality":' .. causal_json ..
        ',"tiles":[' .. table.concat(tile_json, ",") .. ']' ..
        ',"cram_base_address":' .. tostring(item.cram_base_address) ..
        ',"cram_bytes":' .. bytes_json(item.cram_bytes) .. '}'
end
local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m68k.m12-sprite-piece-capture.v1",')
output:write('"run_id":', json(run_id), ',"emulator":"bizhawk","version":', json(client.getversion()),
    ',"canonical_rom_sha256":', json(rom_sha), ',"scenario_path":', json(scenario_path),
    ',"frames_executed":', tostring(frame), ',"catalog_observations":[')
local catalog_json_values = {}
for _, item in ipairs(catalog_observations) do
    catalog_json_values[#catalog_json_values + 1] = catalog_json(item)
end
output:write(table.concat(catalog_json_values, ","), '],"witnesses":[')
local witness_json = {}
for _, item in ipairs(witnesses) do
    witness_json[#witness_json + 1] = '{"frame":' .. tostring(item.frame) ..
        ',"sat_index":0,"sat_address":' .. json(item.sat_address) ..
        ',"shadow_address":' .. json(item.shadow_address) ..
        ',"shadow_bytes":' .. bytes_json(item.shadow_bytes) ..
        ',"sat_bytes":' .. bytes_json(item.sat_bytes) ..
        ',"tile_bytes":' .. bytes_json(item.tile_bytes) ..
        ',"cram_bytes":' .. bytes_json(item.cram_bytes) ..
        ',"dma":{"frame":' .. tostring(item.dma.frame) ..
        ',"sequence":' .. tostring(item.dma.sequence) ..
        ',"pc":' .. json(item.dma.pc) .. ',"source":' .. json(item.dma.source) ..
        ',"destination":' .. json(item.dma.destination) ..
        ',"length_words":' .. tostring(item.dma.length_words) ..
        ',"source_bytes":' .. bytes_json(item.dma.source_bytes) .. '},' ..
        '"producer_writes":[' .. list_json(item.producer_writes,
            {"frame", "sequence", "pc", "address", "value", "flags"}) .. '],' ..
        '"producer_executions":[' .. list_json(item.producer_executions,
            {"frame", "sequence", "pc", "address", "bytes"}) .. ']}'
end
output:write(table.concat(witness_json, ","),
    '],"sat_mutation_observation_complete":true,"shadow_mutation_observation_complete":true,"sat_mutation_channels":["vdp_dma","cpu_vdp_data_port","sat_base_register"],')
output:write('"sat_mutation_events":[', list_json(sat_mutations,
    {"frame", "sequence", "phase", "kind", "destination_start", "destination_end",
     "source", "length", "raw_affected_bytes"}), '],')
output:write('"shadow_mutation_events":[', list_json(shadow_mutations,
    {"frame", "sequence", "phase", "address", "destination_start", "destination_end",
     "value", "flags", "raw_bytes"}), '],')
output:write('"sat_base_events":[', list_json(sat_base_events,
    {"frame", "sequence", "phase", "register", "value", "sat_base"}),
    '],"sat_base_initial":', tostring(SAT_BASE), ',"state_writes_emitted":false}')
output:close()
client.exitCode(0)
