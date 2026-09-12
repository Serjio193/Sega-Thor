-- Developer-only one-shot Ali transition probe from the existing BizHawk F1 state.
-- It records bounded selector/SAT provenance and never writes emulator state.

local output_path = os.getenv("OASIS_M12_ALI_OUTPUT") or "m12-ali-transition.json"
local state_path = os.getenv("OASIS_M12_ALI_STATE") or ""
local SETTLE_FRAMES = 3
local MAX_WAIT_FRAMES = 12
local SAT_SOURCE, SAT_SOURCE_LENGTH = 0xFF13CC, 0x158
local SAT_VRAM, SAT_VRAM_LENGTH = 0xD000, 0x158
local DESCRIPTOR, DESCRIPTOR_LENGTH = 0x03B8DE, 0x80
local RELATIVE_ROOT, RELATIVE_LENGTH = 0x03BDA6, 0x40
local WATCH_PCS = { 0x03B3DE, 0x03B416, 0x03B41A, 0x03B422,
    0x03B426, 0x03B448, 0x0000B730, 0x0000B742, 0x0000B752,
    0x0000B764, 0x0000B76E, 0x0000B77A, 0x00003820 }
local MAX_EVENTS = 512

local function hex(value)
    return string.format("0x%08X", (value or 0) & 0xFFFFFFFF)
end

local function quote(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end

local function read_bytes(address, length, domain)
    local ok, bytes = pcall(memory.read_bytes_as_array, address, length, domain)
    if not ok or not bytes or #bytes < length then return nil end
    return bytes
end

local function read_word(address)
    local bytes = read_bytes(address, 2, "M68K BUS")
    if not bytes then return nil end
    return (bytes[1] << 8) | bytes[2]
end

local function bytes_json(bytes)
    if not bytes then return "null" end
    local values = {}
    for _, value in ipairs(bytes) do values[#values + 1] = tostring(value) end
    return "[" .. table.concat(values, ",") .. "]"
end

local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
end

local function registers()
    local value = { a = {}, d = {}, sr = register("M68K SR") }
    for index = 0, 7 do
        value.a[index] = register("M68K A" .. index)
        value.d[index] = register("M68K D" .. index)
    end
    return value
end

local function registers_json(value)
    local result = '{"a":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. quote(hex(value.a[index]))
    end
    result = result .. '],"d":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. quote(hex(value.d[index]))
    end
    return result .. '],"sr":' .. quote(hex(value.sr)) .. '}'
end

local function append(list, item)
    if #list < MAX_EVENTS then list[#list + 1] = item end
end

local function fnv(bytes)
    if not bytes then return nil end
    local result = 2166136261
    for _, value in ipairs(bytes) do result = ((result ~ value) * 16777619) & 0xFFFFFFFF end
    return result
end

local function diff_ranges(before, after)
    if not before or not after then return {} end
    local ranges, start = {}, nil
    for index = 1, math.min(#before, #after) + 1 do
        local changed = index <= #before and index <= #after and before[index] ~= after[index]
        if changed and not start then start = index - 1 end
        if not changed and start then
            ranges[#ranges + 1] = { start = start, length = index - 1 - start }
            start = nil
        end
    end
    return ranges
end

local function snapshot()
    local selector = read_word(0xFFAFAE)
    local descriptor_offset = selector and ((selector & 7) * 0x10) or 0
    local descriptor = read_bytes(DESCRIPTOR + descriptor_offset, 0x10, "M68K BUS")
    local source = read_bytes(SAT_SOURCE, SAT_SOURCE_LENGTH, "M68K BUS")
    local vram = read_bytes(SAT_VRAM, SAT_VRAM_LENGTH, "VRAM")
    local relative = read_bytes(RELATIVE_ROOT, RELATIVE_LENGTH, "M68K BUS")
    return {
        frame = emu.framecount(), selector = selector, descriptor = descriptor,
        descriptor_address = DESCRIPTOR + descriptor_offset, source = source,
        vram = vram, relative = relative, source_hash = fnv(source),
        vram_hash = fnv(vram), relative_hash = fnv(relative), regs = registers()
    }
end

local function metadata(name, fallback)
    local ok, value = pcall(function() return gameinfo[name]() end)
    return ok and value or fallback
end

local executions, writes, dma = {}, {}, {}
local vdp_regs, current_frame, b730_calls, decoder_calls = {}, 0, 0, 0

for _, watched in ipairs(WATCH_PCS) do
    local pc = watched
    event.on_bus_exec(function()
        local regs = registers()
        local item = { frame = current_frame, pc = hex(pc), regs = regs }
        if pc == 0x03B41A or pc == 0x03B422 or pc == 0x03B426 then
            item.selector = read_word(0xFFAFAE)
            item.descriptor = read_bytes(DESCRIPTOR, DESCRIPTOR_LENGTH, "M68K BUS")
        elseif pc == 0x0000B730 then
            b730_calls = b730_calls + 1
            item.a0_source = hex(regs.a[0])
            item.source_probe = bytes_json(read_bytes(regs.a[0], 0x20, "M68K BUS"))
        elseif pc == 0x00003820 then
            decoder_calls = decoder_calls + 1
        end
        append(executions, item)
    end, pc, "M12 one-shot Ali transition", "M68K BUS")
end

-- This is a bounded SAT source range, not a global RAM write hook. It is
-- armed before the transition so the exact last writer is retained.
for address = SAT_SOURCE, SAT_SOURCE + SAT_SOURCE_LENGTH - 1 do
    local watched_address = address
    event.on_bus_write(function(write_address, value, flags)
        local before = read_bytes(watched_address, 4, "M68K BUS")
        append(writes, { frame = current_frame, address = hex(write_address),
            value = hex(value), old = bytes_json(before), pc = hex(register("M68K PC")),
            flags = hex(flags or 0), regs = registers() })
    end, watched_address, "M12 exact SAT source writer", "M68K BUS")
end

event.on_bus_write(function(address, value, flags)
    if address == 0xFFAFAE then
        append(writes, { frame = current_frame, address = hex(address), value = hex(value),
            old = "not-read-before-selector-write", pc = hex(register("M68K PC")),
            flags = hex(flags or 0), regs = registers() })
    elseif address == 0xC00004 then
        local words = value > 0xFFFF and { (value >> 16) & 0xFFFF, value & 0xFFFF } or { value & 0xFFFF }
        for _, word in ipairs(words) do
            if (word & 0xC000) == 0x8000 then vdp_regs[(word >> 8) & 0x1F] = word & 0xFF end
        end
        if value > 0xFFFF and (value & 0x80) ~= 0 then
            local command = (value >> 16) & 0xFFFF
            local destination = (command & 0x3FFF) | ((value & 3) << 14)
            if destination == SAT_VRAM then
                local source_words = (vdp_regs[21] or 0) | ((vdp_regs[22] or 0) << 8) |
                    ((vdp_regs[23] or 0) << 16)
                local length_words = (vdp_regs[19] or 0) | ((vdp_regs[20] or 0) << 8)
                if length_words == 0 then length_words = 0x10000 end
                append(dma, { frame = current_frame, pc = hex(register("M68K PC")),
                    destination = hex(destination), source = hex((source_words << 1) & 0xFFFFFF),
                    length_words = length_words, control = hex(value) })
            end
        end
    end
end, "M12 VDP/SAT DMA correlation", "M68K BUS")

local loaded = false
local load_method = "slot1"
local ok, result = pcall(function() return savestate.loadslot(1, true) end)
loaded = ok and result == true
if not loaded and state_path ~= "" then
    load_method = "explicit-fallback"
    local fallback_ok, fallback_result = pcall(function() return savestate.load(state_path, true) end)
    loaded = fallback_ok and fallback_result == true
end

local state_a, state_b, changed = nil, nil, {}
if loaded then
    for _ = 1, SETTLE_FRAMES do
        joypad.set({}, 1)
        emu.frameadvance()
        current_frame = emu.framecount()
    end
    state_a = snapshot()
    joypad.set({ ["P1 Right"] = true }, 1)
    emu.frameadvance()
    current_frame = emu.framecount()
    local candidate = snapshot()
    if candidate.source_hash ~= state_a.source_hash or candidate.vram_hash ~= state_a.vram_hash or
        candidate.selector ~= state_a.selector or candidate.relative_hash ~= state_a.relative_hash then
        state_b = candidate
    else
        for _ = 1, MAX_WAIT_FRAMES do
            joypad.set({}, 1)
            emu.frameadvance()
            current_frame = emu.framecount()
            candidate = snapshot()
            if candidate.source_hash ~= state_a.source_hash or candidate.vram_hash ~= state_a.vram_hash or
                candidate.selector ~= state_a.selector or candidate.relative_hash ~= state_a.relative_hash then
                state_b = candidate
                break
            end
        end
    end
    if state_b then
        changed = {
            source = diff_ranges(state_a.source, state_b.source),
            vram = diff_ranges(state_a.vram, state_b.vram),
            relative = diff_ranges(state_a.relative, state_b.relative),
            descriptor = diff_ranges(state_a.descriptor, state_b.descriptor)
        }
    end
end

local function state_json(value)
    if not value then return "null" end
    return '{"frame":' .. tostring(value.frame) .. ',"selector":' ..
        (value.selector and tostring(value.selector) or "null") ..
        ',"descriptor_address":' .. quote(hex(value.descriptor_address)) ..
        ',"descriptor":' .. bytes_json(value.descriptor) ..
        ',"source_hash":' .. (value.source_hash and quote(hex(value.source_hash)) or "null") ..
        ',"vram_hash":' .. (value.vram_hash and quote(hex(value.vram_hash)) or "null") ..
        ',"relative_hash":' .. (value.relative_hash and quote(hex(value.relative_hash)) or "null") ..
        ',"source":' .. bytes_json(value.source) .. ',"vram":' .. bytes_json(value.vram) ..
        ',"relative":' .. bytes_json(value.relative) .. ',"regs":' .. registers_json(value.regs) .. '}'
end

local function ranges_json(value)
    local result = {}
    for _, item in ipairs(value or {}) do
        result[#result + 1] = '{"start":' .. tostring(item.start) .. ',"length":' .. tostring(item.length) .. '}'
    end
    return "[" .. table.concat(result, ",") .. "]"
end

local function events_json(list)
    local result = {}
    for _, item in ipairs(list) do
        result[#result + 1] = '{"frame":' .. tostring(item.frame) ..
            ',"pc":' .. quote(item.pc or "") .. ',"address":' .. quote(item.address or "") ..
            ',"value":' .. quote(item.value or "") .. ',"old":' .. (item.old or "null") ..
            ',"flags":' .. quote(item.flags or "") .. ',"regs":' .. registers_json(item.regs) .. '}'
    end
    return "[" .. table.concat(result, ",") .. "]"
end

local output = assert(io.open(output_path, "w"))
local canonical_rom_sha256 = os.getenv("OASIS_M12_ALI_ROM_SHA256") or "unknown"
output:write('{"schema":"oasis.m68k.m12-ali-transition.v1","emulator":"bizhawk",')
output:write('"version":', quote(client.getversion()), ',"rom_name":', quote(metadata("getromname", "unknown")),
    ',"rom_sha256":', quote(canonical_rom_sha256), ',"core_rom_hash":',
    quote(metadata("getromhash", "unknown")), ',"state_path":', quote(state_path),
    ',"state_load":{"ok":', loaded and "true" or "false", ',"method":', quote(load_method), '},')
output:write('"state_a":', state_json(state_a), ',"state_b":', state_json(state_b),
    ',"changed":{"source":', ranges_json(changed.source), ',"vram":', ranges_json(changed.vram),
    ',"relative":', ranges_json(changed.relative), ',"descriptor":', ranges_json(changed.descriptor),
    '},"executions":[')
local execution_values = {}
for _, item in ipairs(executions) do
    execution_values[#execution_values + 1] = '{"frame":' .. tostring(item.frame) ..
        ',"pc":' .. quote(item.pc) .. ',"selector":' .. (item.selector and tostring(item.selector) or "null") ..
        ',"a0_source":' .. (item.a0_source and quote(item.a0_source) or "null") ..
        ',"source_probe":' .. (item.source_probe or "null") .. ',"regs":' .. registers_json(item.regs) .. '}'
end
output:write(table.concat(execution_values, ","), '],"writes":', events_json(writes), ',"dma":[')
local dma_values = {}
for _, item in ipairs(dma) do
    dma_values[#dma_values + 1] = '{"frame":' .. tostring(item.frame) .. ',"pc":' .. quote(item.pc) ..
        ',"destination":' .. quote(item.destination) .. ',"source":' .. quote(item.source) ..
        ',"length_words":' .. tostring(item.length_words) .. ',"control":' .. quote(item.control) .. '}'
end
output:write(table.concat(dma_values, ","), '],"b730_calls":', tostring(b730_calls),
    ',"decoder_0x3820_calls":', tostring(decoder_calls),
    ',"limits":{"settle_frames":', tostring(SETTLE_FRAMES), ',"max_wait_frames":',
    tostring(MAX_WAIT_FRAMES), ',"max_events":', tostring(MAX_EVENTS),
    '},"state_writes_emitted":false}')
output:close()
client.exitCode(0)
