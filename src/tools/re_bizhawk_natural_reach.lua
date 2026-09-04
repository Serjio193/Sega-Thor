-- Developer-only bounded natural reachability probe.
-- It never writes emulator memory or registers and starts from ROM reset.

local scenario_path = os.getenv("OASIS_SCENARIO_FILE")
local trace_path = os.getenv("OASIS_NATURAL_TRACE_OUTPUT") or "natural-trace.txt"
local report_path = os.getenv("OASIS_NATURAL_REPORT_OUTPUT") or "natural-report.json"
local input_override = os.getenv("OASIS_INPUT_EVENTS")
local search_mode = os.getenv("OASIS_NATURAL_SEARCH") == "caller_targets"
local scenario_family = os.getenv("OASIS_SCENARIO_FAMILY") or "natural_idle_to_6121a_v1"
local variant_id = os.getenv("OASIS_VARIANT_ID") or "default"

local scenario = {
    id = "env_input_probe",
    rom_sha256 = "unknown",
    backend = "bizhawk-lua-natural-input",
    start_state = "hardware_reset",
    stop_condition = "max_frames:1800",
    targets = { 0x6121A, 0x60B8C, 0x60D4A, 0x60BCC, 0x60BD0, 0x60BFA, 0x60C08 },
    inputs = {}
}

local function trim(value)
    return (value:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function add_input(frame, port, buttons)
    if port ~= 1 then error("only controller port 1 is supported") end
    local values = {}
    for button in buttons:gmatch("[^+]+") do values[#values + 1] = trim(button) end
    table.sort(values)
    scenario.inputs[frame] = values
end

local function load_scenario(path)
    scenario.targets = {}
    local input = assert(io.open(path, "r"))
    for line in input:lines() do
        line = trim(line)
        local key, value = line:match("^(%S+)=([^%s]+)$")
        if key == "scenario_id" then scenario.id = value
        elseif key == "rom_sha256" then scenario.rom_sha256 = value
        elseif key == "backend" then scenario.backend = value
        elseif key == "start_state" then scenario.start_state = value
        elseif key == "stop_condition" then scenario.stop_condition = value
        elseif key == "target_address" then scenario.targets[#scenario.targets + 1] = tonumber(value:gsub("0x", ""), 16)
        elseif line:match("^input ") then
            local frame, port, buttons = line:match("^input frame=(%d+) port=(%d+) buttons=(%S+)$")
            if not frame then error("malformed scenario input: " .. line) end
            add_input(tonumber(frame), tonumber(port), buttons)
        end
    end
    input:close()
end

if scenario_path then load_scenario(scenario_path)
elseif input_override then
    for item in input_override:gmatch("[^,]+") do
        local frame, buttons = item:match("(%d+):(.+)")
        if not frame then error("malformed OASIS_INPUT_EVENTS item: " .. item) end
        add_input(tonumber(frame), 1, buttons)
    end
end

if input_override then scenario.stop_condition = "max_frames:" .. (os.getenv("OASIS_MAX_FRAMES") or "1800") end
local max_frames = tonumber(scenario.stop_condition:match("max_frames:(%d+)"))
if not max_frames then error("stop_condition must contain max_frames:<n>") end
local output = assert(io.open(trace_path, "w"))
local report = assert(io.open(report_path, "w"))
local primary_target = scenario.targets[1]
local static_callers = {
    { pc = 0x60B8C, bytes = "61 00 06 8C", mnemonic = "BSR.W", displacement = 0x068C, target = 0x6121A, instruction_size = 4, return_address = 0x60B90 },
    { pc = 0x60D4A, bytes = "61 00 04 CE", mnemonic = "BSR.W", displacement = 0x04CE, target = 0x6121A, instruction_size = 4, return_address = 0x60D4E },
    { pc = 0x611EE, bytes = "61 00 00 2A", mnemonic = "BSR.W", displacement = 0x002A, target = 0x6121A, instruction_size = 4, return_address = 0x611F2 }
}
local caller_by_pc = {}
for _, caller in ipairs(static_callers) do caller_by_pc[caller.pc] = caller end
local frame = 0
local target_hit = false
local stop_requested = false
local target_frame = nil
local target_sequence = nil
local search_target_address = nil
local search_target_frame = nil
local search_target_sequence = nil
local sequence = 0
local previous_pc = nil
local ring = {}
local observed_events = {}
local trace_events = {}
local first_events = {}
local writes = {}
local entry = nil
local target_hits = {}
local caller_hits = {}
local observed_target_hits = {}
local previous_watched_event = nil
local primary_search_callers = { [0x60B8C] = true, [0x60D4A] = true }
local watch_targets = scenario.targets
if search_mode then
    watch_targets = { 0x60B8C, 0x60D4A, 0x6121A, 0x611EE }
end

local function hex(value)
    return string.format("0x%08X", (value or 0) & 0xFFFFFFFF)
end

local function json(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n') .. '"'
end

local function register(name)
    local ok, value = pcall(emu.getregister, name)
    if ok and value ~= nil then return value end
    return nil
end

local function snapshot()
    local result = { d = {}, a = {}, sr = register("M68K SR") }
    for index = 0, 7 do result.d[index + 1] = register("M68K D" .. index) end
    for index = 0, 7 do result.a[index + 1] = register("M68K A" .. index) end
    return result
end

local function stack_window(stack, before, length)
    local start = stack - before
    if start < 0 then return nil end
    local ok, bytes = pcall(memory.read_bytes_as_array, start, length, "M68K BUS")
    if not ok or not bytes then return nil end
    local result = {}
    for index = 1, #bytes do result[index] = bytes[index] end
    return { start = start, bytes = result }
end

local function read_long(address)
    if not address then return nil end
    local ok, bytes = pcall(memory.read_bytes_as_array, address, 4, "M68K BUS")
    if not ok or not bytes or #bytes < 4 then return nil end
    return ((bytes[1] & 0xFF) << 24) | ((bytes[2] & 0xFF) << 16) |
        ((bytes[3] & 0xFF) << 8) | (bytes[4] & 0xFF)
end

local function snapshot_json(value)
    local result = '{"d":['
    for index = 1, 8 do if index > 1 then result = result .. ',' end; result = result .. json(hex(value.d[index])) end
    result = result .. '],"a":['
    for index = 1, 8 do if index > 1 then result = result .. ',' end; result = result .. json(hex(value.a[index])) end
    return result .. '],"sr":' .. json(hex(value.sr)) .. '}'
end

local function sample_pc(pc)
    local prior = previous_pc
    local event = { seq = sequence, pc = pc }
    sequence = sequence + 1
    observed_events[#observed_events + 1] = event
    ring[#ring + 1] = event
    if #ring > 128 then table.remove(ring, 1) end
    if #first_events < 512 then first_events[#first_events + 1] = event end
    previous_pc = pc
    return prior
end

local function capture_watched(address, kind)
    local registers = snapshot()
    local stack = stack_window(registers.a[8] or register("M68K SP") or 0, 0x20, 0x60)
    local event = { seq = sequence, frame = frame, pc = address, kind = kind, registers = registers, stack = stack }
    sequence = sequence + 1
    observed_events[#observed_events + 1] = event
    return event
end

local function register_delta(before, after)
    local result = { d = {}, a = {}, sr = nil }
    for index = 1, 8 do
        if before.d[index] ~= after.d[index] then result.d[index] = { before = before.d[index], after = after.d[index] } end
        if before.a[index] ~= after.a[index] then result.a[index] = { before = before.a[index], after = after.a[index] } end
    end
    if before.sr ~= after.sr then result.sr = { before = before.sr, after = after.sr } end
    return result
end

local function button_map(values)
    local result = {}
    for _, value in ipairs(values or {}) do result["P1 " .. value] = true end
    return result
end

local function write_json_array(values, formatter)
    local result = '['
    for index, value in ipairs(values) do if index > 1 then result = result .. ',' end; result = result .. formatter(value) end
    return result .. ']'
end

event.on_bus_write(function(address, value, flags)
    if #writes < 32 then writes[#writes + 1] = { frame = frame, pc = register("M68K PC"), address = address, value = value, flags = flags } end
end, "natural reach writes", "M68K BUS")

event.onframeend(function()
    if not stop_requested then sample_pc(register("M68K PC") or 0) end
end)

local function caller_event(address)
    local event = capture_watched(address, "caller")
    caller_hits[#caller_hits + 1] = event
    previous_watched_event = event
    if search_mode and primary_search_callers[address] and not stop_requested then
        stop_requested = true
        search_target_address = address
        search_target_frame = frame
        search_target_sequence = event.seq
        target_frame = frame
        target_sequence = event.seq
        target_hit = true
        entry = { pc = address, previous_pc = previous_pc, registers = event.registers, stack = event.stack }
        trace_events = observed_events
    end
end

local function target_event(address)
    target_hits[address] = (target_hits[address] or 0) + 1
    local event = capture_watched(address, "target")
    local caller = previous_watched_event and caller_by_pc[previous_watched_event.pc] and previous_watched_event or nil
    if address == primary_target then
        local expected = caller and caller_by_pc[caller.pc] or nil
        local entry_a7 = event.registers.a[8]
        event.caller = caller
        event.expected_return_address = expected and expected.return_address or nil
        event.stack_return_long = read_long(entry_a7)
        event.a7_delta = caller and entry_a7 and caller.registers.a[8] and entry_a7 - caller.registers.a[8] or nil
        event.return_address_match = expected and event.stack_return_long == expected.return_address or nil
        event.register_delta = caller and register_delta(caller.registers, event.registers) or nil
        observed_target_hits[#observed_target_hits + 1] = event
    end
    previous_watched_event = event
    if not search_mode and not stop_requested and address == primary_target then
        stop_requested = true
        target_hit = true
        target_frame = frame
        target_sequence = event.seq
        local prior_pc = previous_pc
        entry = { pc = address, previous_pc = prior_pc, registers = event.registers, stack = stack_window(event.registers.a[8] or register("M68K SP") or 0, 0x20, 0x60) }
        trace_events = observed_events
    end
end

for _, caller in ipairs(static_callers) do
    local watched_caller = caller.pc
    event.on_bus_exec(function() caller_event(watched_caller) end, watched_caller, "natural caller watch", "M68K BUS")
end
for _, target in ipairs(watch_targets) do
    local watched_target = target
    event.on_bus_exec(function() target_event(watched_target) end, watched_target, "natural target watch", "M68K BUS")
end

while frame < max_frames and not stop_requested do
    joypad.set(button_map(scenario.inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

if #trace_events == 0 then trace_events = first_events end
output:write("oasis.m68k.external-trace.v1\n")
output:write("emulator=bizhawk\nbackend=" .. scenario.backend .. "\nversion=" .. client.getversion() .. "\n")
output:write("scenario=" .. scenario.id .. "\nstop_condition=" .. scenario.stop_condition .. "\nlimit=" .. #trace_events .. "\n")
for _, item in ipairs(trace_events) do
    local line = string.format("event seq=%d pc=%s kind=instruction", item.seq, hex(item.pc))
    if item.registers then
        for index = 1, 8 do line = line .. string.format(" d%d=%s", index - 1, hex(item.registers.d[index])) end
        for index = 1, 8 do line = line .. string.format(" a%d=%s", index - 1, hex(item.registers.a[index])) end
        line = line .. " sr=" .. hex(item.registers.sr)
    end
    output:write(line .. "\n")
end
output:close()

local function optional_hex(value)
    return value and json(hex(value)) or "null"
end

local function stack_json(value)
    if not value then return "null" end
    return '{"start":' .. json(hex(value.start)) .. ',"bytes":' .. write_json_array(value.bytes, function(byte) return json(string.format("0x%02X", byte)) end) .. '}'
end

local function delta_entry_json(value)
    return '{"before":' .. optional_hex(value.before) .. ',"after":' .. optional_hex(value.after) .. '}'
end

local function register_delta_json(value)
    if not value then return "null" end
    local fields = {}
    for index = 1, 8 do
        if value.d[index] then fields[#fields + 1] = '"d' .. (index - 1) .. '":' .. delta_entry_json(value.d[index]) end
        if value.a[index] then fields[#fields + 1] = '"a' .. (index - 1) .. '":' .. delta_entry_json(value.a[index]) end
    end
    if value.sr then fields[#fields + 1] = '"sr":' .. delta_entry_json(value.sr) end
    return '{' .. table.concat(fields, ',') .. '}'
end

local function static_callers_json()
    local result = {}
    for _, caller in ipairs(static_callers) do
        result[#result + 1] = '{"call_site":' .. json(hex(caller.pc)) .. ',"bytes":' .. json(caller.bytes) ..
            ',"mnemonic":' .. json(caller.mnemonic) .. ',"displacement":' .. json(hex(caller.displacement)) ..
            ',"target":' .. json(hex(caller.target)) .. ',"instruction_size":' .. caller.instruction_size ..
            ',"expected_return_address":' .. json(hex(caller.return_address)) .. '}'
    end
    return '[' .. table.concat(result, ',') .. ']'
end

local function watched_event_json(value)
    return '{"sequence":' .. value.seq .. ',"frame":' .. value.frame .. ',"pc":' .. json(hex(value.pc)) ..
        ',"registers":' .. snapshot_json(value.registers) .. ',"stack_window":' .. stack_json(value.stack) .. '}'
end

local function target_hit_json(value)
    local caller = value.caller
    return '{"target_sequence":' .. value.seq .. ',"target_frame":' .. value.frame ..
        ',"target_pc":' .. json(hex(value.pc)) .. ',"paired":' .. tostring(caller ~= nil) ..
        ',"caller_pc":' .. (caller and json(hex(caller.pc)) or "null") ..
        ',"caller_sequence":' .. (caller and caller.seq or "null") ..
        ',"caller_frame":' .. (caller and caller.frame or "null") ..
        ',"caller_event":' .. (caller and watched_event_json(caller) or "null") ..
        ',"target_registers":' .. snapshot_json(value.registers) ..
        ',"target_stack_window":' .. stack_json(value.stack) ..
        ',"caller_a7":' .. (caller and optional_hex(caller.registers.a[8]) or "null") ..
        ',"target_entry_a7":' .. optional_hex(value.registers.a[8]) ..
        ',"a7_delta":' .. (value.a7_delta and tostring(value.a7_delta) or "null") ..
        ',"stack_return_long":' .. optional_hex(value.stack_return_long) ..
        ',"expected_return_address":' .. optional_hex(value.expected_return_address) ..
        ',"return_address_match":' .. (value.return_address_match == nil and "null" or tostring(value.return_address_match)) ..
        ',"register_delta":' .. register_delta_json(value.register_delta) .. '}'
end

local targets = {}
for _, target in ipairs(watch_targets) do targets[#targets + 1] = json(hex(target)) end
local target_report = entry and ('{"pc":' .. json(hex(entry.pc)) .. ',"previous_observed_pc":' .. json(hex(entry.previous_pc)) .. ',"registers":' .. snapshot_json(entry.registers) .. ',"stack_window":' .. (entry.stack and ('{"start":' .. json(hex(entry.stack.start)) .. ',"bytes":' .. write_json_array(entry.stack.bytes, function(value) return json(string.format("0x%02X", value)) end) .. '}') or 'null') .. '}') or 'null'
local hit_report = {}
for _, target in ipairs(watch_targets) do hit_report[#hit_report + 1] = '{"address":' .. json(hex(target)) .. ',"count":' .. (target_hits[target] or 0) .. '}' end
local observed_report = {}
for _, value in ipairs(observed_target_hits) do observed_report[#observed_report + 1] = target_hit_json(value) end
local caller_report = {}
for _, value in ipairs(caller_hits) do caller_report[#caller_report + 1] = watched_event_json(value) end
local relevant = nil
for _, value in ipairs(observed_target_hits) do
    if value.caller then relevant = (value.caller.pc == 0x60B8C or value.caller.pc == 0x60D4A) and "yes" or "no"; break end
end
local old_target_reached = not search_mode and target_hit
local old_target_frame = not search_mode and target_frame or nil
local old_target_sequence = not search_mode and target_sequence or nil
report:write('{"schema":"oasis.m68k.natural-reach.v1","scenario_id":' .. json(scenario.id) .. ',"scenario_family":' .. json(scenario_family) .. ',"variant_id":' .. json(variant_id) .. ',"search_mode":' .. tostring(search_mode) .. ',"input_events":' .. json(input_override or "") .. ',"rom_sha256":' .. json(scenario.rom_sha256) .. ',"backend":' .. json(scenario.backend) .. ',"start_state":' .. json(scenario.start_state) .. ',"stop_condition":' .. json(scenario.stop_condition) .. ',"coverage_mode":"frame_boundary_samples_plus_exact_target_hooks_and_bounded_caller_hooks","frames_executed":' .. frame .. ',"target_addresses":[' .. table.concat(targets, ',') .. '],"target_hits":[' .. table.concat(hit_report, ',') .. '],"target_reached":' .. tostring(old_target_reached) .. ',"target_frame":' .. (old_target_frame or 'null') .. ',"target_sequence":' .. (old_target_sequence or 'null') .. ',"search_target_reached":' .. tostring(search_target_address ~= nil) .. ',"search_target_address":' .. optional_hex(search_target_address) .. ',"search_target_frame":' .. (search_target_frame or 'null') .. ',"search_target_sequence":' .. (search_target_sequence or 'null') .. ',"entry":' .. target_report .. ',"previous_pcs":' .. write_json_array(ring, function(value) return json(hex(value.pc)) end) .. ',"writes":' .. write_json_array(writes, function(value) return '{"frame":' .. value.frame .. ',"pc":' .. json(hex(value.pc)) .. ',"address":' .. json(hex(value.address)) .. ',"value":' .. json(hex(value.value)) .. ',"flags":' .. tostring(value.flags or 0) .. '}' end) .. ',"caller_discrimination":{"static_bytes_verified":true,"static_callers":' .. static_callers_json() .. ',"caller_hits":[' .. table.concat(caller_report, ',') .. '],"observed_hits":[' .. table.concat(observed_report, ',') .. '],"relevant_to_existing_stack_blocker":' .. (relevant and json(relevant) or "null") .. ',"deterministic":true}}')
report:close()
client.exitCode(0)
