-- Developer-only bounded runtime stack provenance probe.
-- It watches one known natural path and never changes emulator state.

local scenario_path = os.getenv("OASIS_SCENARIO_FILE")
local report_path = os.getenv("OASIS_STACK_REPORT_OUTPUT") or "stack-provenance.json"
local trace_path = os.getenv("OASIS_STACK_TRACE_OUTPUT") or "stack-provenance.txt"
local input_override = os.getenv("OASIS_INPUT_EVENTS")
local stack_watch_text = os.getenv("OASIS_STACK_WATCH_ADDRESS")
local stack_watch_address = stack_watch_text and tonumber(stack_watch_text:gsub("0x", ""), 16) or nil

local scenario = {
    id = "env_stack_probe",
    rom_sha256 = "unknown",
    backend = "bizhawk-lua-stack-provenance",
    start_state = "hardware_reset",
    stop_condition = "max_frames:1800",
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
    local input = assert(io.open(path, "r"))
    for line in input:lines() do
        line = trim(line)
        local key, value = line:match("^(%S+)=([^%s]+)$")
        if key == "scenario_id" then scenario.id = value
        elseif key == "rom_sha256" then scenario.rom_sha256 = value
        elseif key == "backend" then scenario.backend = value
        elseif key == "start_state" then scenario.start_state = value
        elseif key == "stop_condition" then scenario.stop_condition = value
        elseif line:match("^input ") then
            local frame, port, buttons = line:match("^input frame=(%d+) port=(%d+) buttons=(%S+)$")
            if not frame then error("malformed scenario input: " .. line) end
            add_input(tonumber(frame), tonumber(port), buttons)
        end
    end
    input:close()
end

if not scenario_path then error("OASIS_SCENARIO_FILE is required") end
load_scenario(scenario_path)
if input_override then
    scenario.inputs = {}
    for item in input_override:gmatch("[^,]+") do
        local frame, buttons = item:match("(%d+):(.+)")
        if not frame then error("malformed OASIS_INPUT_EVENTS item: " .. item) end
        add_input(tonumber(frame), 1, buttons)
    end
    scenario.stop_condition = "max_frames:" .. (os.getenv("OASIS_MAX_FRAMES") or "1800")
end
local max_frames = tonumber(scenario.stop_condition:match("max_frames:(%d+)"))
if not max_frames then error("stop_condition must contain max_frames:<n>") end

local output = assert(io.open(trace_path, "w"))
local report = assert(io.open(report_path, "w"))
local frame = 0
local sequence = 0
local stop_requested = false
local events = {}
local hits = {}
local writer_events = {}
local pre_60bcc = nil
local callee_entry = nil
local callee_return = nil
local pre_60bd0 = nil
local post_60bd0 = nil
local caller_60b8c = nil
local return_60b90 = nil
local target_6121a = nil
local stack_writer_instruction_60b66 = nil
local target_60bfa = nil
local target_60c08 = nil

local watched = {
    [0x60B8C] = "caller_60b8c",
    [0x6121A] = "target_6121a",
    [0x60B90] = "return_continuation",
    [0x60BCC] = "pre_60bcc",
    [0x60B66] = "stack_writer_instruction_60b66",
    [0x604BC] = "callee_entry",
    [0x604E4] = "callee_rts",
    [0x60BD0] = "consume_60bd0",
    [0x60BD2] = "post_60bd0",
    [0x60BFA] = "target_60bfa",
    [0x60C08] = "target_60c08"
}

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

local function stack_window(stack)
    if not stack or stack < 0x20 then return nil end
    local start = stack - 0x20
    local ok, bytes = pcall(memory.read_bytes_as_array, start, 0x60, "M68K BUS")
    if not ok or not bytes then return nil end
    return { start = start, bytes = bytes }
end

local function read_long(address)
    if not address then return nil end
    local ok, bytes = pcall(memory.read_bytes_as_array, address, 4, "M68K BUS")
    if not ok or not bytes or #bytes < 4 then return nil end
    return ((bytes[1] & 0xFF) << 24) | ((bytes[2] & 0xFF) << 16) |
        ((bytes[3] & 0xFF) << 8) | (bytes[4] & 0xFF)
end

local function stack_json(value)
    if not value then return "null" end
    local values = {}
    for _, byte in ipairs(value.bytes) do values[#values + 1] = json(string.format("0x%02X", byte)) end
    return '{"start":' .. json(hex(value.start)) .. ',"bytes":[' .. table.concat(values, ',') .. ']}'
end

local function snapshot_json(value)
    local result = '{"d":['
    for index = 1, 8 do if index > 1 then result = result .. ',' end; result = result .. json(hex(value.d[index])) end
    result = result .. '],"a":['
    for index = 1, 8 do if index > 1 then result = result .. ',' end; result = result .. json(hex(value.a[index])) end
    return result .. '],"sr":' .. json(hex(value.sr)) .. '}'
end

local function optional_hex(value)
    return value and json(hex(value)) or "null"
end

local function event_json(value)
    if not value then return "null" end
    return '{"sequence":' .. value.sequence .. ',"frame":' .. value.frame ..
        ',"pc":' .. json(hex(value.pc)) .. ',"kind":' .. json(value.kind) ..
        ',"registers":' .. snapshot_json(value.registers) ..
        ',"a7":' .. optional_hex(value.registers.a[8]) ..
        ',"stack_window":' .. stack_json(value.stack) .. '}'
end

local function register_delta(before, after)
    if not before or not after then return nil end
    local result = { d = {}, a = {}, sr = nil }
    for index = 1, 8 do
        if before.d[index] ~= after.d[index] then result.d[index] = { before = before.d[index], after = after.d[index] } end
        if before.a[index] ~= after.a[index] then result.a[index] = { before = before.a[index], after = after.a[index] } end
    end
    if before.sr ~= after.sr then result.sr = { before = before.sr, after = after.sr } end
    return result
end

local function delta_json(value)
    if not value then return "null" end
    local fields = {}
    for index = 1, 8 do
        if value.d[index] then fields[#fields + 1] = '"d' .. (index - 1) .. '":{"before":' .. optional_hex(value.d[index].before) .. ',"after":' .. optional_hex(value.d[index].after) .. '}' end
        if value.a[index] then fields[#fields + 1] = '"a' .. (index - 1) .. '":{"before":' .. optional_hex(value.a[index].before) .. ',"after":' .. optional_hex(value.a[index].after) .. '}' end
    end
    if value.sr then fields[#fields + 1] = '"sr":{"before":' .. optional_hex(value.sr.before) .. ',"after":' .. optional_hex(value.sr.after) .. '}' end
    return '{' .. table.concat(fields, ',') .. '}'
end

local function button_map(values)
    local result = {}
    for _, value in ipairs(values or {}) do result["P1 " .. value] = true end
    return result
end

local function capture(address, kind)
    local registers = snapshot()
    local value = { sequence = sequence, frame = frame, pc = address, kind = kind,
        registers = registers, stack = stack_window(registers.a[8]) }
    sequence = sequence + 1
    events[#events + 1] = value
    return value
end

local function writer_overlaps(address)
    if not stack_watch_address then return false end
    return address <= stack_watch_address + 3 and address + 3 >= stack_watch_address
end

event.on_bus_write(function(address, value, flags)
    if writer_overlaps(address) then
        local writer_pc = register("M68K PC")
        local width = nil
        local width_basis = "unknown_bizhawk_callback_width"
        if writer_pc == 0x60B66 then
            width = 4
            width_basis = "static_MOVE.L_A0_predecrement_evidence"
        elseif value and value > 0xFFFF then
            width = 4
            width_basis = "callback_value_width"
        end
        writer_events[#writer_events + 1] = {
            sequence = sequence, frame = frame, pc = writer_pc, address = address,
            value = value, flags = flags, width = width, width_basis = width_basis,
            before_long = read_long(stack_watch_address),
            instruction_pc = (stack_writer_instruction_60b66 and
                stack_writer_instruction_60b66.frame == frame and
                stack_writer_instruction_60b66.sequence <= sequence) and 0x60B66 or nil
        }
        sequence = sequence + 1
    end
end, "bounded concrete stack writer", "M68K BUS")

local function watched_event(address)
    local kind = watched[address]
    hits[hex(address)] = (hits[hex(address)] or 0) + 1
    if address == 0x60B90 and return_60b90 then return end
    local value = capture(address, kind)
    if address == 0x60B8C and not caller_60b8c then caller_60b8c = value end
    if address == 0x6121A and not target_6121a then target_6121a = value end
    if address == 0x60B90 and caller_60b8c and not return_60b90 then return_60b90 = value end
    if address == 0x60B66 and not stack_writer_instruction_60b66 then stack_writer_instruction_60b66 = value end
    if address == 0x60BCC and not pre_60bcc then
        pre_60bcc = value
        pre_60bcc.p = value.registers.a[8]
        pre_60bcc.stack_long = read_long(pre_60bcc.p)
    elseif address == 0x604BC and pre_60bcc and not callee_entry then
        callee_entry = value
        callee_entry.entry_a7 = value.registers.a[8]
        callee_entry.return_slot_long = read_long(callee_entry.entry_a7)
    elseif address == 0x604E4 and callee_entry and not callee_return then
        callee_return = value
    elseif address == 0x60BD0 and callee_return and not pre_60bd0 then
        pre_60bd0 = value
        pre_60bd0.a7 = value.registers.a[8]
        pre_60bd0.stack_long = read_long(pre_60bd0.a7)
    elseif address == 0x60BD2 and not post_60bd0 then
        post_60bd0 = value
        stop_requested = true
    elseif address == 0x60BFA and not target_60bfa then target_60bfa = value
    elseif address == 0x60C08 and not target_60c08 then target_60c08 = value end
end

for address, _ in pairs(watched) do
    local watched_address = address
    event.on_bus_exec(function() watched_event(watched_address) end, watched_address,
        "bounded stack provenance watch", "M68K BUS")
end

event.onframeend(function() end)
while frame < max_frames and not stop_requested do
    joypad.set(button_map(scenario.inputs[frame]), 1)
    emu.frameadvance()
    frame = frame + 1
end

local function write_event_json(value)
    return '{"sequence":' .. value.sequence .. ',"frame":' .. value.frame ..
        ',"pc":' .. optional_hex(value.pc) .. ',"address":' .. json(hex(value.address)) ..
        ',"value":' .. optional_hex(value.value) .. ',"flags":' .. tostring(value.flags or 0) ..
        ',"width":' .. (value.width or "null") .. ',"width_basis":' .. json(value.width_basis) ..
        ',"instruction_pc":' .. optional_hex(value.instruction_pc) ..
        ',"before_long":' .. optional_hex(value.before_long) .. '}'
end

local writer_events_before_pre = {}
local writer_events_before_pre_total = 0
if pre_60bcc then
    for index = #writer_events, 1, -1 do
        if writer_events[index].sequence < pre_60bcc.sequence then
            writer_events_before_pre_total = writer_events_before_pre_total + 1
            if #writer_events_before_pre < 4 then
                table.insert(writer_events_before_pre, 1, writer_events[index])
            end
        end
    end
end
local writer_values = {}
for _, value in ipairs(writer_events_before_pre) do writer_values[#writer_values + 1] = write_event_json(value) end
local hit_values = {}
for address, count in pairs(hits) do hit_values[#hit_values + 1] = '{"address":' .. json(address) .. ',"count":' .. count .. '}' end
table.sort(hit_values)

output:write("oasis.m68k.re-stack-runtime-provenance.v1\n")
output:write("scenario=" .. scenario.id .. "\nbackend=" .. scenario.backend .. "\n")
output:write("stop_condition=" .. scenario.stop_condition .. "\nframes_executed=" .. frame .. "\n")
for _, value in ipairs(events) do
    output:write(string.format("event seq=%d frame=%d pc=%s kind=%s a7=%s\n",
        value.sequence, value.frame, hex(value.pc), value.kind, hex(value.registers.a[8])))
end
for _, value in ipairs(writer_events_before_pre) do
    output:write(string.format("writer seq=%d frame=%d pc=%s address=%s value=%s width=%s\n",
        value.sequence, value.frame, hex(value.pc), hex(value.address), hex(value.value), value.width or "unknown"))
end
output:close()

local function field_event(name, value)
    return '"' .. name .. '":' .. event_json(value)
end

local p = pre_60bcc and pre_60bcc.p or nil
local entry_a7 = callee_entry and callee_entry.entry_a7 or nil
local consume_a7 = pre_60bd0 and pre_60bd0.a7 or nil
local consume_matches_pre = pre_60bcc and pre_60bd0 and pre_60bcc.stack_long == pre_60bd0.stack_long
local entry_matches_p_minus_four = p and entry_a7 and entry_a7 == p - 4
local consume_matches_p = p and consume_a7 and consume_a7 == p
local post_a0 = post_60bd0 and post_60bd0.registers.a[1] or nil
local post_a7 = post_60bd0 and post_60bd0.registers.a[8] or nil
local caller_return_delta = register_delta(caller_60b8c and caller_60b8c.registers, return_60b90 and return_60b90.registers)
local path = '{"0x60B8C":' .. tostring(caller_60b8c ~= nil) ..
    ',"0x6121A":' .. tostring(target_6121a ~= nil) ..
    ',"0x60B90":' .. tostring(hits["0x00060B90"] ~= nil) ..
    ',"0x60BCC":' .. tostring(pre_60bcc ~= nil) ..
    ',"0x604BC":' .. tostring(callee_entry ~= nil) ..
    ',"0x604E4":' .. tostring(callee_return ~= nil) ..
    ',"0x60BD0":' .. tostring(pre_60bd0 ~= nil) ..
    ',"0x60BFA":' .. tostring(target_60bfa ~= nil) ..
    ',"0x60C08":' .. tostring(target_60c08 ~= nil) .. '}'
report:write('{"schema":"oasis.m68k.re-stack-runtime-provenance.v1","scenario_id":' .. json(scenario.id) ..
    ',"scenario_family":"natural_reach_60b8c_60d4a_v1","variant_id":"start_pulse_120","input_events":' ..
    json(input_override or "scenario_file") .. ',"rom_sha256":' .. json(scenario.rom_sha256) ..
    ',"backend":' .. json(scenario.backend) .. ',"backend_version":' .. json(client.getversion()) ..
    ',"start_state":' .. json(scenario.start_state) .. ',"stop_condition":' .. json(scenario.stop_condition) ..
    ',"frames_executed":' .. frame .. ',"stack_watch_address":' .. optional_hex(stack_watch_address) ..
    ',"stack_writer_instruction_60b66":' .. event_json(stack_writer_instruction_60b66) ..
    ',"writer_event_count_before_pre_60bcc":' .. writer_events_before_pre_total ..
    ',"static_60bcc":{"bytes":"61 00 F8 EE","target":"0x000604BC","return_address":"0x00060BD0"},"static_60bd0_bytes":"20 5F"' ..
    ',"path":' .. path .. ',' .. field_event("caller_60b8c", caller_60b8c) .. ',' ..
    field_event("return_continuation_60b90", return_60b90) ..
    ',"caller_to_return_delta":' .. delta_json(caller_return_delta) ..
    ',"pre_60bcc":{"event":' .. event_json(pre_60bcc) .. ',"p":' .. optional_hex(p) ..
    ',"stack_long":' .. optional_hex(pre_60bcc and pre_60bcc.stack_long) .. '},"callee_entry":{"event":' ..
    event_json(callee_entry) .. ',"entry_a7":' .. optional_hex(entry_a7) ..
    ',"return_slot_long":' .. optional_hex(callee_entry and callee_entry.return_slot_long) ..
    ',"entry_a7_equals_p_minus_4":' .. (entry_matches_p_minus_four == nil and "null" or tostring(entry_matches_p_minus_four)) ..
    '},"callee_return":' .. event_json(callee_return) ..
    ',"consume_60bd0":{"event":' .. event_json(pre_60bd0) .. ',"a7":' .. optional_hex(consume_a7) ..
    ',"stack_long":' .. optional_hex(pre_60bd0 and pre_60bd0.stack_long) ..
    ',"a7_equals_p":' .. (consume_matches_p == nil and "null" or tostring(consume_matches_p)) ..
    ',"matches_pre_60bcc_stack_long":' .. (consume_matches_pre == nil and "null" or tostring(consume_matches_pre)) ..
    '},"post_60bd0":{"event":' .. event_json(post_60bd0) .. ',"a0":' .. optional_hex(post_a0) ..
    ',"a7":' .. optional_hex(post_a7) .. ',"a0_equals_consumed_long":' ..
    ((post_a0 and pre_60bd0 and post_a0 == pre_60bd0.stack_long) and "true" or "null") ..
    ',"a7_equals_p_plus_4":' .. ((post_a7 and p and post_a7 == p + 4) and "true" or "null") ..
    '},"writer_events":[' .. table.concat(writer_values, ',') .. '],"watched_hits":[' .. table.concat(hit_values, ',') ..
    '],"target_60bfa_reached":' .. tostring(target_60bfa ~= nil) .. ',"target_60bfa_event":' ..
    event_json(target_60bfa) .. ',"target_60c08_reached":' .. tostring(target_60c08 ~= nil) ..
    ',"target_60c08_event":' .. event_json(target_60c08) .. ',"deterministic":true}')
report:close()
client.exitCode(0)
