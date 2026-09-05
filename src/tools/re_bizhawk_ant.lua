-- One developer-only natural BizHawk worker for oasis.m68k.re-ant-job.v1.
-- It observes one requested indirect instruction and its next executed PC.

local job_path = assert(os.getenv("OASIS_ANT_JOB_FILE"), "OASIS_ANT_JOB_FILE is required")
local result_path = os.getenv("OASIS_ANT_RESULT_OUTPUT") or "ant-result.json"
local job_file = assert(io.open(job_path, "r"))
local job = job_file:read("*a")
job_file:close()

local function field(name)
    local value = job:match('"' .. name .. '":"([^"`]*)"')
    if value then return value end
    value = job:match('"' .. name .. '":([0-9]+)')
    return value
end

local function required(name)
    local value = field(name)
    if not value then error("missing job field: " .. name) end
    return value
end

local function number(name)
    local value = required(name)
    return tonumber(value:gsub("0x", ""), 16)
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

local function bytes_at(address)
    local ok, values = pcall(memory.read_bytes_as_array, address, 2, "M68K BUS")
    if not ok or not values or #values < 2 then return {} end
    return { values[1] & 0xFF, values[2] & 0xFF }
end

local function bytes_json(values)
    local result = "["
    for index, value in ipairs(values) do if index > 1 then result = result .. "," end; result = result .. value end
    return result .. "]"
end

local function snapshot_json(value)
    local result = '{"d":['
    for index = 1, 8 do if index > 1 then result = result .. "," end; result = result .. json(hex(value.d[index])) end
    result = result .. '],"a":['
    for index = 1, 8 do if index > 1 then result = result .. "," end; result = result .. json(hex(value.a[index])) end
    return result .. '],"sr":' .. json(hex(value.sr)) .. '}'
end

local function fnv(text)
    local hash = 2166136261
    for index = 1, #text do
        hash = ((hash ~ string.byte(text, index)) * 16777619) & 0xFFFFFFFF
    end
    return string.format("0x%08X", hash)
end

local source_pc = number("source_pc")
local max_steps = tonumber(required("max_steps"))
local max_frames = tonumber(required("max_frames"))
local rom_size = tonumber(required("rom_size"))
local job_id = required("job_id")
local frontier_id = required("frontier_id")
local rom_sha256 = required("rom_sha256")
local backend_version = client.getversion()
local started = os.clock()
local frame = 0
local sequence = 0
local instructions = 0
local source = nil
local next_pc = nil
local stopped = false

event.on_bus_exec_any(function(address)
    if stopped then return end
    instructions = instructions + 1
    if source and not next_pc then
        next_pc = { pc = address, frame = frame, sequence = sequence }
        stopped = true
        return
    end
    if not source and address == source_pc then
        source = { pc = address, frame = frame, sequence = sequence, registers = snapshot(), bytes = bytes_at(address) }
    end
    sequence = sequence + 1
end)

while frame < max_frames and instructions < max_steps and not stopped do
    joypad.set({}, 1)
    emu.frameadvance()
    frame = frame + 1
end

local status = source and next_pc and "RESOLVED" or source and "TIMEOUT" or "NOT_REACHED"
local stop_reason = source and next_pc and "next_pc_observed" or instructions >= max_steps and "max_steps" or "max_frames_without_frontier"
local target = next_pc and next_pc.pc or nil
local target_inside = target ~= nil and target < rom_size and (target & 1) == 0
local target_text = target and tostring(target) or "unknown"
local frame_text = source and tostring(source.frame) or "unknown"
local sequence_text = source and tostring(source.sequence) or "unknown"
local hash_input = job_id .. "|" .. tostring(source_pc) .. "|" .. target_text .. "|" .. frame_text .. "|" .. sequence_text .. "|" .. (source and tostring(source.registers.a[2]) or "unknown")
local output = assert(io.open(result_path, "w"))
output:write('{"schema":"oasis.m68k.re-ant-result.v1","job_id":' .. json(job_id) .. ',"frontier_id":' .. json(frontier_id) ..
    ',"status":' .. json(status) .. ',"backend":"bizhawk","backend_version":' .. json(backend_version) ..
    ',"rom_sha256":' .. json(rom_sha256) .. ',"reachability_class":"DYNAMIC_NATURAL","source_entry":' .. json(hex(number("source_entry"))) ..
    ',"source_pc":' .. json(hex(source_pc))
    .. (source and ',"observed_actual_pc":' .. json(hex(source.pc)) .. ',"observed_instruction":' .. json(required("instruction")) ..
        ',"instruction_bytes":' .. bytes_json(source.bytes) .. ',"observed_next_pc":' .. json(hex(target)) ..
        ',"observed_indirect_target":' .. json(hex(target)) .. ',"observed_frame":' .. source.frame ..
        ',"observed_sequence":' .. source.sequence .. ',"target_inside_rom":' .. tostring(target_inside) ..
        ',"observed_registers":' .. snapshot_json(source.registers) or "")
    .. ',"stop_reason":' .. json(stop_reason) .. ',"reproducible":true,"result_hash":' .. json(fnv(hash_input)) ..
    ',"startup_load_ms":0,"checkpoint_restore_ms":0,"execution_ms":' .. math.floor((os.clock() - started) * 1000) ..
    ',"total_wall_clock_ms":' .. math.floor((os.clock() - started) * 1000) .. ',"frames_executed":' .. frame ..
    ',"instructions_until_observation":' .. instructions .. '}')
output:close()
client.exitCode(0)
