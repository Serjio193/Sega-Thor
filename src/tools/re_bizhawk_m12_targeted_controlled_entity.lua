-- Developer-only targeted M12 controller/entity observer.
-- Worker capture remains disabled; this script emits bounded runtime evidence.

local output_path = assert(os.getenv("OASIS_TARGETED_OUTPUT"), "OASIS_TARGETED_OUTPUT is required")
local state_path = assert(os.getenv("OASIS_TARGETED_STATE"), "OASIS_TARGETED_STATE is required")
local run_id = assert(os.getenv("OASIS_TARGETED_RUN_ID"), "OASIS_TARGETED_RUN_ID is required")
local hold_frames = tonumber(os.getenv("OASIS_TARGETED_HOLD_FRAMES") or "5")
local max_exec = tonumber(os.getenv("OASIS_TARGETED_MAX_EXEC") or "180000")
local max_bus = tonumber(os.getenv("OASIS_TARGETED_MAX_BUS") or "30000")
local ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
local ENTITY_START, ENTITY_END = 0x00FF13CC, 0x00FF13F4
local INPUT_START, INPUT_END = 0x00FF1654, 0x00FF1664
local WINDOW_RANGES = {
    { 0x000029E0, 0x00002A60 },
    { 0x0000A320, 0x0000A3C0 },
}

local function quote(value)
    return '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end

local function number_or_null(value)
    return value == nil and "null" or tostring(value)
end

local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or 0
end

local function native_value(name)
    if not genesis or not genesis[name] then return nil end
    local ok, value = pcall(genesis[name])
    return ok and value or nil
end

local function snapshot()
    local value = { d = {}, a = {}, sr = register("M68K SR"), pc = register("M68K PC") }
    for index = 0, 7 do
        value.d[index] = register("M68K D" .. index) & 0xFFFFFFFF
        value.a[index] = register("M68K A" .. index) & 0xFFFFFFFF
    end
    value.sr = value.sr & 0xFFFF
    value.pc = value.pc & 0xFFFFFFFF
    return value
end

local function snapshot_json(value)
    local result = '{"d":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. tostring(value.d[index])
    end
    result = result .. '],"a":['
    for index = 0, 7 do
        if index > 0 then result = result .. "," end
        result = result .. tostring(value.a[index])
    end
    return result .. '],"sr":' .. tostring(value.sr) .. ',"pc":' .. tostring(value.pc) .. '}'
end

local function opcode_at(address)
    local ok, bytes = pcall(memory.read_bytes_as_array, address, 2, "M68K BUS")
    if not ok or not bytes or #bytes < 2 then return 0 end
    return ((bytes[1] & 0xFF) << 8) | (bytes[2] & 0xFF)
end

local function in_range(address, start_address, end_address)
    return address >= start_address and address < end_address
end

local function in_exec_window(address)
    for _, range in ipairs(WINDOW_RANGES) do
        if in_range(address, range[1], range[2]) then return true end
    end
    return false
end

local function push(list, item, limit)
    if #list < limit then list[#list + 1] = item end
end

local function inputs_json(value)
    if value == "" then return "[]" end
    return "[" .. quote(value) .. "]"
end

local function exec_json(item)
    return '{"observer_instruction_sequence":' .. tostring(item.observer_instruction_sequence) ..
        ',"native_instruction_sequence":' .. number_or_null(item.native_instruction_sequence) ..
        ',"frame":' .. tostring(item.frame) .. ',"pc":' .. tostring(item.pc) ..
        ',"opcode":' .. tostring(item.opcode) .. ',"flags":' .. tostring(item.flags) ..
        ',"next_pc":' .. number_or_null(item.next_pc) ..
        ',"snapshot_id":' .. tostring(item.snapshot_id) .. '}'
end

local function bus_json(item)
    return '{"kind":' .. quote(item.kind) .. ',"frame":' .. tostring(item.frame) ..
        ',"observer_instruction_sequence":' .. tostring(item.observer_instruction_sequence) ..
        ',"native_instruction_sequence":' .. number_or_null(item.native_instruction_sequence) ..
        ',"pc":' .. tostring(item.pc) .. ',"opcode":' .. tostring(item.opcode) ..
        ',"address":' .. tostring(item.address) .. ',"value":' .. tostring(item.value) ..
        ',"flags":' .. tostring(item.flags) .. ',"width_bytes":null' ..
        ',"width_basis":"not_exposed_by_bizhawk_callback"' ..
        ',"snapshot_id":' .. tostring(item.snapshot_id) .. '}'
end

local function boundary_json(item)
    return '{"run_id":' .. quote(run_id) .. ',"epoch":' .. number_or_null(item.epoch) ..
        ',"frame":' .. tostring(item.frame) .. ',"stream_sequence":' ..
        number_or_null(item.stream_sequence) .. ',"kind":"FRAME_END"}'
end

local function transition_json(item)
    return '{"index":' .. tostring(item.index) .. ',"name":' .. quote(item.name) ..
        ',"buttons":' .. inputs_json(item.buttons) .. ',"apply_frame":' ..
        tostring(item.apply_frame) .. ',"first_end_frame":' .. tostring(item.first_end_frame) ..
        ',"last_end_frame":' .. tostring(item.last_end_frame) .. '}'
end

local function poll_json(item)
    return '{"frame":' .. tostring(item.frame) .. ',"poll_index":' ..
        tostring(item.poll_index) .. ',"buttons":' .. inputs_json(item.buttons) ..
        ',"native_instruction_sequence":' .. number_or_null(item.native_instruction_sequence) .. '}'
end

assert(savestate.load(state_path, true), "targeted savestate load failed")
local initial_frame = emu.framecount()
if genesis and genesis.live_forward_enable then
    assert(genesis.live_forward_enable(false), "could not keep Worker recorder disabled")
end

local frame_boundaries, transitions, input_polls = {}, {}, {}
local executions, bus_events, snapshots = {}, {}, {}
local observer_instruction_sequence, poll_index = 0, 0
local snapshot_id = 0
local current_instruction = nil
local current_buttons = ""
local last_watched_execution = nil
local bus_counts = {}

local function capture_snapshot()
    snapshot_id = snapshot_id + 1
    snapshots[snapshot_id] = snapshot()
    return snapshot_id
end

local function on_watched_exec(watched_pc, address, value, flags)
    observer_instruction_sequence = observer_instruction_sequence + 1
    if last_watched_execution then last_watched_execution.next_pc = address end
    local native_sequence = native_value("live_forward_instruction_sequence")
    local item = {
        observer_instruction_sequence = observer_instruction_sequence,
        native_instruction_sequence = native_sequence,
        frame = emu.framecount(), pc = watched_pc, opcode = value or 0,
        flags = flags or 0, snapshot_id = capture_snapshot(),
    }
    item.opcode = opcode_at(watched_pc)
    push(executions, item, max_exec)
    last_watched_execution = item
    current_instruction = item
end

for address = 0x000029E0, 0x00002A60 - 1 do
    local watched_pc = address
    event.on_bus_exec(function(exec_address, value, flags)
        on_watched_exec(watched_pc, exec_address, value, flags)
    end, watched_pc, "M12 targeted input PC window", "M68K BUS")
end
for address = 0x0000A320, 0x0000A3C0 - 1 do
    local watched_pc = address
    event.on_bus_exec(function(exec_address, value, flags)
        on_watched_exec(watched_pc, exec_address, value, flags)
    end, watched_pc, "M12 targeted entity PC window", "M68K BUS")
end

local function on_bus(kind, address, value, flags)
    if #bus_events >= max_bus then return end
    bus_counts[address] = (bus_counts[address] or 0) + 1
    local address_limit = 16
    if address == 0x00FF1654 then address_limit = 96 end
    if address == 0x00A10003 or address == 0x00A10005 or
        in_range(address, INPUT_START, INPUT_END) or
        in_range(address, ENTITY_START, ENTITY_END) then address_limit = 10000 end
    if bus_counts[address] > address_limit then return end
    local instruction = {
        observer_instruction_sequence = observer_instruction_sequence,
        native_instruction_sequence = native_value("live_forward_instruction_sequence"),
        frame = emu.framecount(), pc = register("M68K PC"), opcode = 0,
        flags = 0, snapshot_id = capture_snapshot(),
    }
    if current_instruction and instruction.pc == current_instruction.pc then
        instruction.opcode = current_instruction.opcode
    else
        instruction.opcode = opcode_at(instruction.pc)
    end
    push(bus_events, {
        kind = kind, frame = emu.framecount(),
        observer_instruction_sequence = instruction.observer_instruction_sequence,
        native_instruction_sequence = instruction.native_instruction_sequence,
        pc = instruction.pc, opcode = instruction.opcode,
        address = address, value = value or 0, flags = flags or 0,
        snapshot_id = instruction.snapshot_id,
    }, max_bus)
end

local watched_memory = { 0x00A10003, 0x00A10005 }
for address = INPUT_START, INPUT_END - 1 do watched_memory[#watched_memory + 1] = address end
for address = ENTITY_START, ENTITY_END - 1 do watched_memory[#watched_memory + 1] = address end
for address = 0x00FF1600, 0x00FF1700 - 1 do watched_memory[#watched_memory + 1] = address end
local registered_memory = {}
for _, address in ipairs(watched_memory) do
    if not registered_memory[address] then
        registered_memory[address] = true
        local watched_address = address
        event.on_bus_read(function(bus_address, value, flags)
            on_bus("READ", watched_address, value, flags)
        end, watched_address, "M12 targeted controlled entity reads", "M68K BUS")
        event.on_bus_write(function(bus_address, value, flags)
            on_bus("WRITE", watched_address, value, flags)
        end, watched_address, "M12 targeted controlled entity writes", "M68K BUS")
    end
end

event.oninputpoll(function()
    poll_index = poll_index + 1
    push(input_polls, { frame = emu.framecount(), poll_index = poll_index,
        buttons = current_buttons,
        native_instruction_sequence = native_value("live_forward_instruction_sequence") }, max_bus)
end, "M12 targeted input poll")

event.onframeend(function()
    push(frame_boundaries, { epoch = native_value("live_forward_epoch"),
        frame = emu.framecount(), stream_sequence = native_value("live_forward_stream_sequence") },
        max_bus)
end, "M12 targeted frame boundary")

local sequence = {
    { "NEUTRAL", "" }, { "RIGHT", "Right" }, { "NEUTRAL", "" },
    { "LEFT", "Left" }, { "NEUTRAL", "" }, { "UP", "Up" },
    { "NEUTRAL", "" }, { "DOWN", "Down" }, { "NEUTRAL", "" },
    { "ACTION_BUTTON", "A" }, { "NEUTRAL", "" },
}
for index, phase in ipairs(sequence) do
    local apply_frame = emu.framecount()
    current_buttons = phase[2]
    local transition = { index = index, name = phase[1], buttons = phase[2],
        apply_frame = apply_frame, first_end_frame = apply_frame + 1,
        last_end_frame = apply_frame + hold_frames }
    push(transitions, transition, #sequence)
    local button_table = {}
    if phase[2] ~= "" then button_table[phase[2]] = true end
    for _ = 1, hold_frames do
        joypad.set(button_table, 1)
        emu.frameadvance()
    end
end

local output = assert(io.open(output_path, "w"))
output:write('{"schema":"oasis.m12.targeted-controlled-entity-capture.v1",')
output:write('"architecture":"DEVELOPER_ONLY_TARGETED_OBSERVER_WORKER_DISABLED",')
output:write('"run_id":', quote(run_id), ',"canonical_rom_sha256":', quote(ROM_SHA256),
    ',"state_path":', quote(state_path), ',"initial_frame":', tostring(initial_frame),
    ',"start_frame":', tostring(initial_frame), ',"end_frame":', tostring(emu.framecount()),
    ',"hold_frames":', tostring(hold_frames), ',"worker_disabled":true,')
output:write('"frame_boundaries":[')
local values = {}
for _, item in ipairs(frame_boundaries) do values[#values + 1] = boundary_json(item) end
output:write(table.concat(values, ","), '],"transitions":[')
values = {}
for _, item in ipairs(transitions) do values[#values + 1] = transition_json(item) end
output:write(table.concat(values, ","), '],"input_polls":[')
values = {}
for _, item in ipairs(input_polls) do values[#values + 1] = poll_json(item) end
output:write(table.concat(values, ","), '],"executions":[')
values = {}
for _, item in ipairs(executions) do values[#values + 1] = exec_json(item) end
output:write(table.concat(values, ","), '],"bus_events":[')
values = {}
for _, item in ipairs(bus_events) do values[#values + 1] = bus_json(item) end
output:write(table.concat(values, ","), '],"snapshots":[')
values = {}
for index = 1, #snapshots do
    values[#values + 1] = '{"snapshot_id":' .. tostring(index) ..
        ',"registers":' .. snapshot_json(snapshots[index]) .. '}'
end
output:write(table.concat(values, ","), '],"limits":{"max_exec":', tostring(max_exec),
    ',"max_bus":', tostring(max_bus), '},"state_writes_emitted":false}')
output:close()
client.exitCode(0)
client.exit()
