-- Developer-only bounded natural reachability probe.
-- It never writes emulator memory or registers and starts from ROM reset.

local scenario_path = os.getenv("OASIS_SCENARIO_FILE")
local trace_path = os.getenv("OASIS_NATURAL_TRACE_OUTPUT") or "natural-trace.txt"
local report_path = os.getenv("OASIS_NATURAL_REPORT_OUTPUT") or "natural-report.json"
local input_override = os.getenv("OASIS_INPUT_EVENTS")

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
local frame = 0
local target_hit = false
local target_frame = nil
local target_sequence = nil
local sequence = 0
local previous_pc = nil
local ring = {}
local trace_events = {}
local first_events = {}
local writes = {}
local entry = nil
local target_hits = {}

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
    local start = stack - 0x20
    if start < 0 then return nil end
    local ok, bytes = pcall(memory.read_bytes_as_array, start, 0x60, "M68K BUS")
    if not ok or not bytes then return nil end
    local result = {}
    for index = 1, #bytes do result[index] = bytes[index] end
    return { start = start, bytes = result }
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
    ring[#ring + 1] = event
    if #ring > 128 then table.remove(ring, 1) end
    if #first_events < 512 then first_events[#first_events + 1] = event end
    previous_pc = pc
    return prior
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
    if not target_hit then sample_pc(register("M68K PC") or 0) end
end)

local function target_event(address)
    target_hits[address] = (target_hits[address] or 0) + 1
    if not target_hit and address == primary_target then
        target_hit = true
        target_frame = frame
        target_sequence = sequence
        local prior_pc = previous_pc
        entry = { pc = address, previous_pc = prior_pc, registers = snapshot(), stack = stack_window(register("M68K A7") or register("M68K SP") or 0) }
        trace_events = {}
        for _, item in ipairs(ring) do trace_events[#trace_events + 1] = item end
        trace_events[#trace_events + 1] = { seq = sequence, pc = address, registers = entry.registers }
        sequence = sequence + 1
    end
end

for _, target in ipairs(scenario.targets) do
    local watched_target = target
    event.on_bus_exec(function() target_event(watched_target) end, watched_target, "natural target watch", "M68K BUS")
end

while frame < max_frames and not target_hit do
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

local targets = {}
for _, target in ipairs(scenario.targets) do targets[#targets + 1] = json(hex(target)) end
local target_report = entry and ('{"pc":' .. json(hex(entry.pc)) .. ',"previous_observed_pc":' .. json(hex(entry.previous_pc)) .. ',"registers":' .. snapshot_json(entry.registers) .. ',"stack_window":' .. (entry.stack and ('{"start":' .. json(hex(entry.stack.start)) .. ',"bytes":' .. write_json_array(entry.stack.bytes, function(value) return json(string.format("0x%02X", value)) end) .. '}') or 'null') .. '}') or 'null'
local hit_report = {}
for _, target in ipairs(scenario.targets) do hit_report[#hit_report + 1] = '{"address":' .. json(hex(target)) .. ',"count":' .. (target_hits[target] or 0) .. '}' end
report:write('{"schema":"oasis.m68k.natural-reach.v1","scenario_id":' .. json(scenario.id) .. ',"rom_sha256":' .. json(scenario.rom_sha256) .. ',"backend":' .. json(scenario.backend) .. ',"start_state":' .. json(scenario.start_state) .. ',"stop_condition":' .. json(scenario.stop_condition) .. ',"coverage_mode":"frame_boundary_samples_plus_exact_target_hooks","frames_executed":' .. frame .. ',"target_addresses":[' .. table.concat(targets, ',') .. '],"target_hits":[' .. table.concat(hit_report, ',') .. '],"target_reached":' .. tostring(target_hit) .. ',"target_frame":' .. (target_frame or 'null') .. ',"target_sequence":' .. (target_sequence or 'null') .. ',"entry":' .. target_report .. ',"previous_pcs":' .. write_json_array(ring, function(value) return json(hex(value.pc)) end) .. ',"writes":' .. write_json_array(writes, function(value) return '{"frame":' .. value.frame .. ',"pc":' .. json(hex(value.pc)) .. ',"address":' .. json(hex(value.address)) .. ',"value":' .. json(hex(value.value)) .. ',"flags":' .. tostring(value.flags or 0) .. '}' end) .. '}')
report:close()
client.exitCode(0)
